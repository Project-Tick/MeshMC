#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
"""Driver for the ``setup-kf6`` composite action.

Builds KDE Frameworks 6 from the official release tarballs against an already
installed Qt 6 (for example one installed by aqtinstall), in dependency order.

Sub-commands (each is one workflow step, they share ``<work-dir>/plan.json``):

    plan         resolve the selection, download + verify tarballs, analyse the
                 dependency tree, group frameworks into tiers, compute cache keys
    sysdeps      install the system packages the planned frameworks need
    build        build one tier (or restore it from its cache delta)
    finish       export the environment, write the summary, set outputs

Dependency analysis
-------------------
KDE publishes the dependency data its own CI uses inside every release
tarball, so nothing here is hard-coded per framework:

* ``.kde-ci.yml``    build-time dependencies, per platform (Linux/Windows/macOS)
* ``metainfo.yaml``  declared tier + supported platforms

The tier of a framework (the unit that is cached) is the tier KDE declares,
raised to the tier of anything it depends on if the declaration is missing or
inconsistent. Inside a tier, frameworks are ordered by dependency depth, so
(tier, depth, name) is always a valid topological build order.

Inputs arrive as JSON in ``$KF6_INPUTS`` (``${{ toJSON(inputs) }}``), see
``action.yml`` for their documentation. The script has no third party
dependencies and runs on the Linux, macOS and Windows GitHub runners.
"""

from __future__ import annotations

import concurrent.futures
import contextlib
import difflib
import hashlib
import json
import os
import platform
import re
import shlex
import shutil
import subprocess
import sys
import tarfile
import time
import urllib.error
import urllib.request
from dataclasses import asdict, dataclass
from pathlib import Path

# Tiers are cached separately. KDE declares tiers 1..3 (frameworkintegration
# declares 4); anything deeper is folded into the last group, which is safe
# because order inside a group is still topological.
MAX_GROUP = 4

# Part of every cache key, see cmd_plan. Bump it when cached data must not be reused.
CACHE_FORMAT = 3

# Release tarballs that are not CMake projects.
NOT_BUILDABLE = {"kapidox"}

# Fixes for upstream bugs that stop a framework from building on some platform, applied to the
# unpacked sources: (framework, file relative to the source root, old text, new text).
# Each is idempotent. If neither the old nor the new text is found, upstream changed the code
# (most likely fixed it) and a warning says the patch should be reviewed.
SOURCE_PATCHES = [
    # solid: the MSVC branch calls __cpuid() unconditionally, an x86 intrinsic that does not exist
    # for ARM64 ("error C3861: '__cpuid': identifier not found" on Windows ARM64). `features` starts
    # at 0, so skipping the branch just reports no x86 instruction sets, which is right on ARM.
    ("solid",
     "src/solid/devices/backends/shared/cpufeatures.cpp",
     "#elif defined(_MSC_VER)\n    int array[4], ft = 1;",
     "#elif defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))\n    int array[4], ft = 1;"),
]

ECM = "extra-cmake-modules"
ALIASES = {"ecm": ECM}

PLATFORM_ALIASES = {
    "Linux": {"Linux"},
    "macOS": {"macOS", "MacOSX", "Mac OS X"},
    "Windows": {"Windows"},
}

# Defaults every framework is configured with. User supplied ``cmake-args``
# come afterwards, so they can override anything here.
BASE_CMAKE_DEFS = [
    "-DBUILD_TESTING=OFF",
    "-DBUILD_QCH=OFF",
    "-DBUILD_EXAMPLES=OFF",
    "-DBUILD_PYTHON_BINDINGS=OFF",
    # Keep plugins/QML inside our prefix instead of writing into the Qt install.
    "-DKDE_INSTALL_USE_QT_SYS_PATHS=OFF",
]

# Per-framework defaults (before user supplied ``framework-cmake-args``).
FRAMEWORK_CMAKE_DEFS = {
    # 24x24 icon generation needs Python + the lxml module; the 22px/32px
    # icons are shipped pre-rendered so nothing essential is lost.
    "breeze-icons": ["-DWITH_ICON_GENERATION=OFF"],
    # The secret service daemons need Qca, which distro/Homebrew packages build against
    # *their* Qt, not the one used here, and a daemon is not something a bundled prefix
    # can run anyway. The KF6Wallet client library is unaffected.
    "kwallet": ["-DBUILD_KSECRETD=OFF", "-DBUILD_KWALLETD=OFF"],
}

# Optional features that need a Qt module. Switched off when that module is not
# installed, so a base aqtinstall Qt works without extra modules:
# framework -> (CMake option, Qt module).
QT_MODULE_FEATURES = {
    "ktextwidgets": ("WITH_TEXT_TO_SPEECH", "TextToSpeech"),  # qtspeech
    "prison": ("WITH_MULTIMEDIA", "Multimedia"),  # qtmultimedia
}

# System packages by platform. "*" is always installed, other keys only when
# the framework is part of the plan. Names are apt / brew / choco packages.
_X11 = [
    "libx11-dev", "libx11-xcb-dev", "libxcb1-dev", "libxcb-keysyms1-dev",
    "libxcb-xkb-dev", "libxcb-icccm4-dev", "libxcb-image0-dev", "libxcb-util-dev",
    "libxcb-res0-dev", "libxcb-record0-dev", "libxcb-screensaver0-dev",
    "libxcb-sync-dev", "libxcb-randr0-dev", "libxcb-xtest0-dev",
    "libxkbcommon-x11-dev", "libxext-dev", "libxfixes-dev", "libxrender-dev",
]
_WAYLAND = ["libwayland-dev", "wayland-protocols", "plasma-wayland-protocols"]
_XML = ["libxml2-dev", "libxslt1-dev"]
SYSTEM_DEPS = {
    "Linux": {
        "*": ["gperf", "gettext", "bison", "flex", "pkg-config", "perl",
              "libgl1-mesa-dev", "libxkbcommon-dev"],
        "kcoreaddons": ["libmount-dev", "libudev-dev"],
        "solid": ["libudev-dev", "libmount-dev"],
        "kwindowsystem": _X11 + _WAYLAND,
        "kguiaddons": _X11 + _WAYLAND,
        "kidletime": _X11 + _WAYLAND,
        "kglobalaccel": _X11,
        "kcrash": _X11,
        "kdesu": _X11 + ["libutempter-dev"],
        "kstatusnotifieritem": _X11,
        "qqc2-desktop-style": _X11,
        "kpty": ["libutempter-dev"],
        "knotifications": ["libcanberra-dev"],
        "kdoctools": _XML + ["docbook-xml", "docbook-xsl", "libxml2-utils", "xsltproc", "liburi-perl"],
        "kio": _XML + ["libacl1-dev", "libmount-dev"],
        "kcalendarcore": ["libical-dev"],
        "kdnssd": ["libavahi-client-dev"],
        "kfilemetadata": ["libattr1-dev"],
        "baloo": ["liblmdb-dev"],
        "networkmanager-qt": ["libnm-dev"],
        "modemmanager-qt": ["libmm-glib-dev"],
        "prison": ["libqrencode-dev", "libdmtx-dev", "libzxing-dev"],
        # Deliberately absent: libqca-qt6-dev, libpolkit-qt6-1-dev, ... Packages that
        # link Qt are built against the distro's Qt, which must not be mixed with the
        # aqtinstall Qt used here. KAuth falls back to its own backends without polkit.
    },
    "macOS": {
        "*": ["gettext", "gperf", "bison", "flex", "pkgconf"],
        "kcalendarcore": ["libical"],
        # libxml2/libxslt: Homebrew's, not the SDK's. On runners with both Xcode and the Command Line
        # Tools, CMake finds them in the CLT SDK, which adds `-I<CLT SDK>/usr/include` next to Xcode's
        # libc++ and breaks every C++ standard header ("<cmath> tried including <math.h> but didn't
        # find libc++'s <math.h>"). Homebrew's headers live outside any SDK, so nothing clashes.
        # kdoctools' own CMake module knows Homebrew's DocBook layout (opt/docbook/docbook/xml/4.5).
        "kdoctools": ["libxml2", "libxslt", "docbook", "docbook-xsl"],
        "kio": ["libxml2", "libxslt"],
        "prison": ["qrencode", "libdmtx", "zxing-cpp"],
    },
    "Windows": {
        "*": ["gperf", "winflexbison3", "gettext"],
    },
}


# --------------------------------------------------------------------------
# small helpers
# --------------------------------------------------------------------------

IN_CI = os.environ.get("GITHUB_ACTIONS") == "true"


def log(msg: str = "") -> None:
    print(msg, flush=True)


def notice(msg: str) -> None:
    log(f"::notice::{msg}" if IN_CI else f"notice: {msg}")


def warning(msg: str) -> None:
    log(f"::warning::{msg}" if IN_CI else f"warning: {msg}")


def die(msg: str) -> None:
    log(f"::error::{msg}" if IN_CI else f"error: {msg}")
    sys.exit(1)


@contextlib.contextmanager
def group(title: str):
    log(f"::group::{title}" if IN_CI else f"--- {title}")
    try:
        yield
    finally:
        if IN_CI:
            log("::endgroup::")


def append_file(env_var: str, text: str) -> None:
    target = os.environ.get(env_var)
    if not target:
        log(f"[{env_var}] {text.rstrip()}")
        return
    with open(target, "a", encoding="utf-8") as fh:
        fh.write(text if text.endswith("\n") else text + "\n")


def set_output(key: str, value: str) -> None:
    append_file("GITHUB_OUTPUT", f"{key}={value}")


def truthy(value) -> bool:
    return str(value).strip().lower() in {"1", "true", "yes", "on"}


def current_platform() -> str:
    override = os.environ.get("KF6_PLATFORM_OVERRIDE")  # for testing the planner
    if override:
        return override
    return {"Linux": "Linux", "Darwin": "macOS", "Windows": "Windows"}.get(
        platform.system(), platform.system()
    )


def normalized_arch() -> str:
    machine = platform.machine().lower()
    return {"amd64": "x86_64", "aarch64": "arm64"}.get(machine, machine)


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with open(path, "rb") as fh:
        for chunk in iter(lambda: fh.read(1 << 20), b""):
            digest.update(chunk)
    return digest.hexdigest()


def http_get(url: str, retries: int = 4, fatal: bool = True) -> bytes | None:
    last: Exception | None = None
    for attempt in range(retries):
        try:
            req = urllib.request.Request(url, headers={"User-Agent": "setup-kf6"})
            with urllib.request.urlopen(req, timeout=120) as resp:
                return resp.read()
        except (urllib.error.URLError, TimeoutError, ConnectionError) as exc:
            last = exc
            time.sleep(2 * (attempt + 1))
    if fatal:
        die(f"Could not download {url}: {last}")
    return None


def split_list(value: str) -> list[str]:
    return [t for t in re.split(r"[\s,;]+", value or "") if t]


# --------------------------------------------------------------------------
# configuration
# --------------------------------------------------------------------------


class Config:
    """Typed view over the action inputs."""

    def __init__(self) -> None:
        raw = os.environ.get("KF6_INPUTS", "{}")
        self.raw: dict = json.loads(raw)
        self.version = self.get("version", "6.31.0")
        self.frameworks = self.get("frameworks")
        self.exclude = split_list(self.get("exclude"))
        self.build_type = self.get("build-type", "Release")
        self.linkage = self.get("linkage", "shared").lower()
        self.cmake_args = self.get("cmake-args")
        self.framework_cmake_args = self.get("framework-cmake-args")
        self.osx_architectures = self.get("osx-architectures")
        self.generator = self.get("generator", "Ninja")
        self.cache = truthy(self.get("cache", "true"))
        self.cache_key_suffix = self.get("cache-key-suffix")
        self.system_deps = truthy(self.get("system-deps", "true"))
        self.extra_system_packages = split_list(self.get("extra-system-packages"))
        self.keep_going = truthy(self.get("keep-going", "false"))
        self.ignore_failures = truthy(self.get("ignore-failures", "false"))
        self.export_env = truthy(self.get("export-env", "true"))
        self.verbose = truthy(self.get("verbose", "false"))
        self.base_url = self.get("base-url", "https://download.kde.org/stable/frameworks").rstrip("/")
        self.jobs = int(self.get("jobs", "0") or 0) or (os.cpu_count() or 2)
        self.qt_prefix_input = self.get("qt-prefix")
        self.qt_host_path = self.get("qt-host-path")

        work = self.get("work-dir") or os.path.join(
            os.environ.get("RUNNER_TEMP", str(Path.home() / ".cache")), "setup-kf6"
        )
        self.work = Path(work).resolve()
        prefix = self.get("install-prefix") or str(self.work.parent / "kf6")
        self.prefix = Path(prefix).resolve()

        if self.linkage not in {"shared", "static"}:
            die(f"linkage must be 'shared' or 'static', got '{self.linkage}'")

    def get(self, name: str, default: str = "") -> str:
        value = self.raw.get(name)
        return default if value is None or str(value).strip() == "" else str(value).strip()


# --------------------------------------------------------------------------
# release metadata
# --------------------------------------------------------------------------


@dataclass
class Framework:
    name: str
    tarball: str
    sha256: str
    src: str  # extracted source directory
    declared_tier: int | None
    platforms: list[str]
    deps: list[str]  # hard: find_package(... REQUIRED) in CMake (incl. ECM). Pulled in, failure blocks us.
    soft_deps: list[str]  # optional in CMake, or only named by .kde-ci.yml. Ordering only.
    external: list[str]  # non-framework KDE dependencies named by .kde-ci.yml
    buildable: bool
    cmakename: str = ""  # e.g. KF6KIO
    fingerprint: str = ""  # identifies how this framework is built (see cmd_plan)
    tier: int = 0  # computed
    group: int = 0  # cache group, 1..MAX_GROUP

    def supports(self, plat: str) -> bool:
        wanted = PLATFORM_ALIASES.get(plat, {plat})
        return any(p == "All" or p in wanted for p in self.platforms)


def parse_metainfo(text: str) -> tuple[int | None, list[str]]:
    tier = None
    m = re.search(r"^tier:\s*(\d+)", text, re.M)
    if m:
        tier = int(m.group(1))
    platforms: list[str] = []
    in_platforms = False
    for line in text.splitlines():
        if re.match(r"^platforms:\s*$", line):
            in_platforms = True
            continue
        if in_platforms:
            if line and not line[0].isspace():
                break
            m = re.match(r"^\s*-\s*name:\s*(.+?)\s*$", line)
            if m:
                platforms.append(m.group(1).strip("'\""))
    return tier, platforms


def parse_kde_ci(text: str, plat: str) -> tuple[list[str], list[str]]:
    """Return (framework deps, other KDE deps) that apply to ``plat``.

    Understands the subset of YAML KDE uses for the ``Dependencies`` block::

        Dependencies:
         - 'on': ['Linux', 'Windows']
           'require':
            'frameworks/kconfig': '@same'
    """
    wanted = PLATFORM_ALIASES.get(plat, {plat})
    frameworks: list[str] = []
    external: list[str] = []
    in_deps = False
    applies = False
    in_require = False
    for raw in text.splitlines():
        line = re.sub(r"\s+#.*$", "", raw).rstrip()
        stripped = line.strip()
        if not stripped or stripped.startswith("#"):
            continue
        if not line[0].isspace():  # top level key
            in_deps = stripped.startswith("Dependencies:")
            in_require = False
            applies = False
            continue
        if not in_deps:
            continue
        m = re.match(r"-\s*['\"]?on['\"]?\s*:\s*\[(.*)\]\s*$", stripped)
        if m:
            targets = {t.strip().strip("'\"") for t in m.group(1).split(",") if t.strip()}
            applies = bool(targets & (wanted | {"@all", "All"}))
            in_require = False
            continue
        if re.match(r"['\"]?require['\"]?\s*:\s*$", stripped):
            in_require = True
            continue
        m = re.match(r"['\"]([^'\"]+)['\"]\s*:\s*['\"][^'\"]*['\"]\s*$", stripped)
        if m and in_require:
            if applies:
                dep = m.group(1)
                (frameworks if dep.startswith("frameworks/") else external).append(dep)
            continue
        warning(f"unrecognised line in .kde-ci.yml Dependencies block: {stripped!r}")
    return [d.split("/", 1)[1] for d in frameworks], external


# Directories whose CMake projects are not part of the framework itself.
SKIP_DIRS = {"autotests", "tests", "test", "examples", "example", "demo", "demos", "templates", "po", "poqm", "doc", "docs"}
# find_package() keywords that are not component names.
_FP_KEYWORDS = {
    "REQUIRED", "QUIET", "CONFIG", "NO_MODULE", "MODULE", "EXACT", "GLOBAL", "NO_POLICY_SCOPE",
    "NO_DEFAULT_PATH", "CMAKE_FIND_ROOT_PATH_BOTH", "ONLY_CMAKE_FIND_ROOT_PATH",
}
_FIND_PACKAGE = re.compile(r"\bfind_package\s*\(([^)]*)\)", re.I)


def scan_cmake_kf_deps(src: Path) -> tuple[set[str], set[str]]:
    """KF6 packages (without the ``KF6`` prefix) the CMake project asks for.

    Returns ``(required, optional)``. Understands both forms::

        find_package(KF6Config 6.31.0 REQUIRED)
        find_package(KF6 6.31.0 REQUIRED COMPONENTS CoreAddons I18n OPTIONAL_COMPONENTS Foo)

    The CMake files are the source of truth for what a framework really needs:
    ``.kde-ci.yml`` only lists what KDE's CI exercises per platform, which is
    sometimes more (optional documentation tooling) and sometimes less (it has
    no macOS entry for kcmutils although it requires KIO there too).
    """
    required: set[str] = set()
    optional: set[str] = set()
    for root, dirs, files in os.walk(src):
        dirs[:] = [d for d in dirs if d.lower() not in SKIP_DIRS and not d.startswith(".")]
        if "CMakeLists.txt" not in files:
            continue
        text = (Path(root) / "CMakeLists.txt").read_text(encoding="utf-8", errors="replace")
        text = re.sub(r"(?m)#.*$", "", text)
        for m in _FIND_PACKAGE.finditer(text):
            tokens = m.group(1).split()
            if not tokens:
                continue
            name, is_required = tokens[0], "REQUIRED" in tokens
            if name == "KF6":
                mode = None
                for tok in tokens[1:]:
                    if tok == "COMPONENTS":
                        mode = "components"
                    elif tok == "OPTIONAL_COMPONENTS":
                        mode = "optional"
                    elif tok in _FP_KEYWORDS or "$" in tok or tok[0].isdigit():
                        continue
                    elif mode == "components":
                        (required if is_required else optional).add(tok)
                    elif mode == "optional":
                        optional.add(tok)
            elif name.startswith("KF6") and len(name) > 3 and "$" not in name:
                (required if is_required else optional).add(name[3:])
    return required, optional


# CMake package names that do not follow the "KF6" + repository name pattern:
# KF6Su is kdesu, and some frameworks ship extra find_package()-able sub-libraries.
PACKAGE_ALIASES = {
    "su": "kdesu",
    "newstuffcore": "knewstuff",
    "newstuffwidgets": "knewstuff",
    "kirigamiplatform": "kirigami",
}


class Releases:
    """Download, verify, extract and parse framework release tarballs."""

    def __init__(self, cfg: Config, plat: str) -> None:
        self.cfg = cfg
        self.plat = plat
        self.version = cfg.version
        self.dir_url = ""
        self.catalog: dict[str, str] = {}
        self._fw: dict[str, Framework] = {}
        self.downloads = cfg.work / "downloads"
        self.sources = cfg.work / "src"
        self.downloads.mkdir(parents=True, exist_ok=True)
        self.sources.mkdir(parents=True, exist_ok=True)
        self._load_catalog()

    def _load_catalog(self) -> None:
        base = self.cfg.base_url
        version = self.version
        if version.lower() == "latest":
            index = http_get(base + "/").decode("utf-8", "replace")
            series = sorted(
                {tuple(map(int, m.groups())) for m in re.finditer(r'href="(\d+)\.(\d+)/"', index)}
            )
            if not series:
                die(f"Could not find any release series at {base}/")
            version = "%d.%d.0" % series[-1]
        major_minor = ".".join(version.split(".")[:2])
        self.dir_url = f"{base}/{major_minor}"
        index = http_get(self.dir_url + "/").decode("utf-8", "replace")
        found: dict[str, set[str]] = {}
        for m in re.finditer(r'href="([A-Za-z0-9._+-]+?)-(\d+\.\d+\.\d+)\.tar\.xz"', index):
            found.setdefault(m.group(2), set()).add(m.group(1))
        if self.version.lower() == "latest":
            version = max(found, key=lambda v: (len(found[v]), tuple(map(int, v.split(".")))), default="")
        if version not in found:
            avail = ", ".join(sorted(found)) or "none"
            die(f"KDE Frameworks {version} not found at {self.dir_url}/ (available there: {avail})")
        self.version = version
        self.catalog = {n: f"{n}-{version}.tar.xz" for n in sorted(found[version])}

    def resolve_name(self, token: str) -> str:
        """Map ``kio``, ``KIO``, ``KF6KIO``, ``KF6::KIO``, ``Config``, ``ecm`` ... to a repo name."""

        def norm(s: str) -> str:
            return re.sub(r"[^a-z0-9]", "", s.lower())

        index = {norm(n): n for n in self.catalog}
        for alias, target in ALIASES.items():
            index.setdefault(alias, target)
        stripped = re.sub(r"^kf6(::)?", "", token, flags=re.I)
        for candidate in (norm(token), norm(stripped), "k" + norm(stripped)):
            if candidate in index:
                return index[candidate]
        close = difflib.get_close_matches(norm(stripped), list(index), n=3)
        hint = f" Did you mean: {', '.join(index[c] for c in close)}?" if close else ""
        die(f"Unknown framework '{token}' for KDE Frameworks {self.version}.{hint}")
        raise AssertionError

    def get(self, name: str) -> Framework:
        if name in self._fw:
            return self._fw[name]
        tarball = self.catalog[name]
        path = self.downloads / tarball
        url = f"{self.dir_url}/{tarball}"
        if not path.exists():
            part = path.with_suffix(".part")
            part.write_bytes(http_get(url))
            part.replace(path)
        digest = sha256_file(path)
        published = http_get(url + ".sha256", retries=2, fatal=False)
        if published:
            expected = published.decode().split()[0].lower()
            if expected != digest:
                path.unlink(missing_ok=True)
                die(f"SHA-256 mismatch for {tarball}: expected {expected}, got {digest}")
        else:
            warning(f"no .sha256 published for {tarball}; skipping verification")

        root = f"{name}-{self.version}"
        src = self.sources / root
        if not (src / ".extracted").exists():
            shutil.rmtree(src, ignore_errors=True)
            extract_tarball(path, self.sources, pseudo_svg_links=(name == "breeze-icons"))
            (src / ".extracted").write_text("ok")
        apply_source_patches(name, src)

        meta = src / "metainfo.yaml"
        meta_text = meta.read_text(encoding="utf-8") if meta.exists() else ""
        tier, platforms = parse_metainfo(meta_text)
        m = re.search(r"^cmakename:\s*(\S+)", meta_text, re.M)
        cmakename = m.group(1) if m else ""
        ci = src / ".kde-ci.yml"
        ci_deps, external = parse_kde_ci(ci.read_text(encoding="utf-8"), self.plat) if ci.exists() else ([], [])

        required, optional = scan_cmake_kf_deps(src) if (src / "CMakeLists.txt").exists() else (set(), set())
        hard: set[str] = set()
        for pkg in sorted(required):
            dep = self.package_to_repo(pkg)
            if dep:
                hard.add(dep)
            else:
                log(f"  note: {name} requires KF6{pkg}, which is not a KDE Frameworks release tarball (assumed provided)")
        soft = {self.package_to_repo(p) for p in optional} | set(ci_deps)
        if name != ECM:
            hard.add(ECM)
        soft -= hard | {name, None}
        hard.discard(name)
        fw = Framework(
            name=name,
            tarball=tarball,
            sha256=digest,
            src=str(src),
            declared_tier=tier,
            platforms=platforms or ["All"],
            deps=sorted(hard, key=lambda d: (d != ECM, d)),
            soft_deps=sorted(soft),
            external=sorted(set(external)),
            buildable=(src / "CMakeLists.txt").exists() and name not in NOT_BUILDABLE,
            cmakename=cmakename,
        )
        self._fw[name] = fw
        return fw

    def package_to_repo(self, pkg: str) -> str | None:
        """``Config`` / ``KIO`` / ``DocTools`` (from find_package(KF6<pkg>)) -> ``kconfig`` / ``kio`` / ``kdoctools``."""

        def norm(s: str) -> str:
            return re.sub(r"[^a-z0-9]", "", s.lower())

        key = norm(pkg)
        for fw in list(self._fw.values()):  # exact answers from metainfo.yaml's cmakename where known
            if fw.cmakename and norm(fw.cmakename[3:] if fw.cmakename.startswith("KF6") else fw.cmakename) == key:
                return fw.name
        index = {norm(n): n for n in self.catalog}
        for candidate in (key, "k" + key):
            if candidate in index:
                return index[candidate]
        return PACKAGE_ALIASES.get(key) if PACKAGE_ALIASES.get(key) in self.catalog else None

    def load_all(self) -> None:
        with concurrent.futures.ThreadPoolExecutor(max_workers=8) as pool:
            list(pool.map(self.get, list(self.catalog)))


def apply_source_patches(name: str, src: Path) -> None:
    for fw, relpath, old, new in SOURCE_PATCHES:
        if fw != name:
            continue
        target = src / relpath
        if not target.exists():
            warning(f"source patch for {name}: {relpath} does not exist any more, review SOURCE_PATCHES")
            continue
        # Newline-insensitive: the sources may have been unpacked with CRLF line endings.
        text = target.read_text(encoding="utf-8")
        if new in text:
            continue  # already applied
        if old not in text:
            warning(f"source patch for {name}: expected code not found in {relpath} "
                    "(changed upstream?), review SOURCE_PATCHES")
            continue
        target.write_text(text.replace(old, new, 1), encoding="utf-8", newline="")
        log(f"  patched {name}: {relpath}")


def materialize_symlinks(links: list[tuple[Path, str]], pseudo_svg_links: bool = False) -> None:
    """Windows: replace the symlinks of a tarball by something that works there.

    Real symlinks are unreliable on Windows (they need a privilege, and some tools mishandle them).

    * ``pseudo_svg_links``: every link (files and directories, like Git does) becomes what Git
      for Windows checks symlinks out as without symlink support: a small file holding just the
      target's name. breeze-icons' tools are written for exactly that. Its icon generator skips
      such files, and ``qrcAlias`` resolves ``*.svg`` ones as aliases. Neither works with real
      links (the generator turns them into bogus ``.lnk`` shortcuts) nor with copies (``qrcAlias``
      rejects the duplicated icons, ``places/16@3x`` is a link to the directory ``16``).
    * otherwise a link is replaced by a copy of what it points to (links to links are resolved
      by retrying until what they need exists).
    """
    pending: list[tuple[Path, str]] = []
    for link, target in links:
        if pseudo_svg_links:
            link.parent.mkdir(parents=True, exist_ok=True)
            link.write_text(target, encoding="utf-8")
        else:
            pending.append((link, target))
    for _ in range(8):
        still: list[tuple[Path, str]] = []
        for link, target in pending:
            resolved = link.parent / target
            if not resolved.exists():
                still.append((link, target))
                continue
            if resolved.is_dir():
                shutil.copytree(resolved, link, dirs_exist_ok=True)
            else:
                link.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(resolved, link)
        pending = still
        if not pending:
            return
    for link, target in pending:
        warning(f"dangling link {link} -> {target} (skipped)")


def extract_tarball(path: Path, dest: Path, windows: bool | None = None, pseudo_svg_links: bool = False) -> None:
    windows = (os.name == "nt") if windows is None else windows
    links: list[tuple[Path, str]] = []
    with tarfile.open(path) as tar:
        for member in tar:
            if windows and member.issym():
                links.append((dest / member.name, member.linkname))
                continue
            try:
                try:
                    tar.extract(member, dest, filter="data")
                except TypeError:  # Python < 3.12 without the filter backport
                    tar.extract(member, dest)
            except (OSError, tarfile.TarError) as exc:
                if member.issym() or member.islnk():
                    warning(f"could not create link {member.name}: {exc}")
                else:
                    die(f"could not extract {member.name} from {path.name}: {exc}")
    if links:
        materialize_symlinks(links, pseudo_svg_links)


# --------------------------------------------------------------------------
# planning
# --------------------------------------------------------------------------


def resolve_selection(rel: Releases, cfg: Config) -> list[Framework]:
    plat = rel.plat
    explicit = split_list(cfg.frameworks)
    select_all = not explicit or any(t.lower() == "all" for t in explicit)
    exclude = {rel.resolve_name(t) for t in cfg.exclude}

    if select_all:
        rel.load_all()
        for fw in rel._fw.values():
            demote_unavailable_deps(rel, fw)
        chosen = {
            n: fw
            for n, fw in rel._fw.items()
            if fw.buildable and fw.supports(plat) and n not in exclude
        }
        for n, fw in sorted(rel._fw.items()):
            if n in chosen:
                continue
            why = (
                "excluded" if n in exclude else
                "not a CMake project" if not fw.buildable else
                f"not supported on {plat}"
            )
            log(f"  skip {n}: {why}")
        # Drop frameworks whose dependencies are not available.
        changed = True
        while changed:
            changed = False
            for n in list(chosen):
                missing = [d for d in chosen[n].deps if d not in chosen]
                if missing:
                    log(f"  skip {n}: needs {', '.join(missing)} which is not built")
                    del chosen[n]
                    changed = True
        return list(chosen.values())

    wanted = [rel.resolve_name(t) for t in explicit]
    picked: dict[str, Framework] = {}
    queue = list(wanted)
    required_by: dict[str, str] = {}
    while queue:
        name = queue.pop(0)
        if name in picked:
            continue
        who = f" (needed by {required_by[name]})" if name in required_by else ""
        if name not in rel.catalog:
            die(f"'{name}'{who} is not part of KDE Frameworks {rel.version}")
        fw = rel.get(name)
        if name in exclude:
            die(f"'{name}'{who} is excluded but required")
        if not fw.buildable:
            die(f"'{name}'{who} is not a CMake project and cannot be built")
        if not fw.supports(plat):
            die(f"'{name}'{who} is not supported on {plat} (supported: {', '.join(fw.platforms)})")
        demote_unavailable_deps(rel, fw)
        picked[name] = fw
        # Only hard dependencies are pulled in; optional ones (docs tooling, ...) are
        # used only if they were asked for too.
        for dep in fw.deps:
            required_by.setdefault(dep, name)
            queue.append(dep)
    return list(picked.values())


def demote_unavailable_deps(rel: Releases, fw: Framework) -> None:
    """A REQUIRED dependency that cannot exist on this platform is almost always
    guarded by a platform check in the CMake file; treat it as optional."""
    keep: list[str] = []
    for dep in fw.deps:
        target = rel.get(dep) if dep in rel.catalog else None
        if target is None or (target.buildable and target.supports(rel.plat)):
            keep.append(dep)
        else:
            log(f"  note: {fw.name} lists {dep} as required, but it is not available on {rel.plat}; treating it as optional")
            fw.soft_deps = sorted(set(fw.soft_deps) | {dep})
    fw.deps = keep


def assign_tiers(fws: list[Framework]) -> None:
    """Compute ``group`` (the tier, used for caching) and ``tier`` (build order).

    * ``group`` follows the tier KDE declares, but is never lower than the group
      of anything the framework depends on, so every dependency sits in the same
      or an earlier group. KDE's own "Tier 3" frameworks depend on each other
      (kio -> kiconthemes), so same-group dependencies are normal.
    * ``tier`` is the dependency depth (ECM = 0). Sorting by (group, tier) is a
      valid topological order, including inside a group.
    """
    by_name = {f.name: f for f in fws}
    depth: dict[str, int] = {}
    grp: dict[str, int] = {}
    visiting: list[str] = []

    def visit(name: str) -> None:
        if name in depth:
            return
        if name in visiting:
            die("dependency cycle: " + " -> ".join(visiting[visiting.index(name):] + [name]))
        visiting.append(name)
        fw = by_name[name]
        deps = [d for d in fw.deps if d in by_name]
        for d in deps:
            visit(d)
        # Optional dependencies only constrain the order if they are built too,
        # and are dropped rather than failing if they would form a cycle.
        for d in fw.soft_deps:
            if d in by_name and d not in deps:
                if d in visiting:
                    log(f"  note: ignoring optional dependency {name} -> {d} (would form a cycle)")
                    continue
                visit(d)
                deps.append(d)
        if name == ECM:
            depth[name], grp[name] = 0, 1
        else:
            depth[name] = 1 + max([depth[d] for d in deps] + [0])
            grp[name] = max([fw.declared_tier or 1] + [grp[d] for d in deps])
        visiting.pop()

    for fw in fws:
        visit(fw.name)
        fw.tier = depth[fw.name]
        fw.group = max(1, min(grp[fw.name], MAX_GROUP))


def detect_qt(cfg: Config) -> dict:
    candidates: list[str] = [cfg.qt_prefix_input, os.environ.get("QT_ROOT_DIR", "")]
    qt6_dir = os.environ.get("Qt6_DIR", "")
    if qt6_dir:
        candidates.append(str(Path(qt6_dir).parents[2]))  # <qt>/lib/cmake/Qt6
    candidates.append(os.environ.get("QTDIR", ""))
    for cand in candidates:
        if not cand:
            continue
        root = Path(cand)
        cmake_dir = root / "lib" / "cmake" / "Qt6Core"
        if (cmake_dir / "Qt6CoreConfig.cmake").exists():
            version = "unknown"
            # Newer Qt keeps the version in ...ConfigVersionImpl.cmake, older in ...ConfigVersion.cmake.
            for fname in ("Qt6CoreConfigVersionImpl.cmake", "Qt6CoreConfigVersion.cmake"):
                f = cmake_dir / fname
                m = re.search(r'set\(PACKAGE_VERSION\s+"([0-9][^"\s]*)"', f.read_text()) if f.exists() else None
                if m:
                    version = m.group(1)
                    break
            return {"prefix": str(root.resolve()), "version": version, "name": root.name}
    die(
        "Could not find a Qt 6 installation. Install Qt first (e.g. jurplel/install-qt-action) "
        "or pass the 'qt-prefix' input. Looked at: "
        + (", ".join(c for c in candidates if c) or "QT_ROOT_DIR, Qt6_DIR, QTDIR (none set)")
    )
    raise AssertionError


def user_cmake_args(cfg: Config) -> list[str]:
    args: list[str] = []
    for line in cfg.cmake_args.splitlines():
        args += shlex.split(line, posix=True)
    return args


def per_framework_args(cfg: Config) -> dict[str, list[str]]:
    out: dict[str, list[str]] = {}
    for line in cfg.framework_cmake_args.splitlines():
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        name, sep, rest = line.partition(":")
        if not sep:
            die(f"framework-cmake-args: expected '<framework>: <args>', got '{line}'")
        out.setdefault(name.strip(), []).extend(shlex.split(rest, posix=True))
    # Set by the "Install DocBook (Windows)" step of this action (via GITHUB_ENV). It cannot come in
    # through the `framework-cmake-args` input: the caller's expression is evaluated before the
    # action runs, when the variable doesn't exist yet.
    extra = os.environ.get("KF6_KDOCTOOLS_ARGS", "").strip()
    if extra:
        out.setdefault("kdoctools", []).extend(shlex.split(extra, posix=True))
    return out


def cmd_plan(cfg: Config) -> None:
    plat = current_platform()
    cfg.work.mkdir(parents=True, exist_ok=True)
    qt = detect_qt(cfg)

    with group("Resolving KDE Frameworks release"):
        rel = Releases(cfg, plat)
        log(f"KDE Frameworks {rel.version} for {plat}/{normalized_arch()}, Qt {qt['version']} ({qt['prefix']})")
        fws = resolve_selection(rel, cfg)
        if not fws:
            die("Nothing to build: the selection is empty for this platform.")
        assign_tiers(fws)
        # Validate user per-framework arguments early.
        names = {f.name for f in fws}
        for n in per_framework_args(cfg):
            if n not in rel.catalog:
                die(f"framework-cmake-args: unknown framework '{n}'")
            if n not in names:
                warning(f"framework-cmake-args given for '{n}', which is not part of the plan")
        fws.sort(key=lambda f: (f.group, f.tier, f.name))

    # ---- cache keys: each tier's key covers everything built up to it
    ctx = {
        "kf": rel.version,
        "os": plat,
        "arch": normalized_arch(),
        "image": os.environ.get("ImageOS", ""),
        "qt": qt["version"],
        "qt_name": qt["name"],
        "build_type": cfg.build_type,
        "linkage": cfg.linkage,
        "generator": cfg.generator,
        # macOS install names and RPATHs embed the absolute prefix.
        "prefix": cfg.prefix.as_posix(),
        "osx_arch": cfg.osx_architectures or os.environ.get("CMAKE_OSX_ARCHITECTURES", ""),
        "args": user_cmake_args(cfg),  # per-framework arguments are part of that framework's entry below
        "suffix": cfg.cache_key_suffix,
        # Bumped by hand when the layout or meaning of cached data changes. Deliberately not a
        # hash of this file: editing the script must not throw away hours of cached builds. What a
        # framework was built with is covered by its fingerprint instead.
        "cache_format": CACHE_FORMAT,
    }
    key_base = f"kf6-{rel.version}-{plat}-{normalized_arch()}-qt{qt['version']}-{cfg.build_type}-{cfg.linkage}"

    # A framework's fingerprint says how it is built: its tarball, its own arguments and the
    # fingerprints of everything it depends on (hard or soft, since optional features are
    # picked up when the dependency is there). `fws` is in build order, so dependencies come first.
    fw_args = per_framework_args(cfg)
    fingerprints: dict[str, str] = {}
    for f in fws:
        parts = [f.name, f.sha256, fw_args.get(f.name, []),
                 [p[1:] for p in SOURCE_PATCHES if p[0] == f.name],  # patched sources are different sources
                 sorted(fingerprints[d] for d in f.deps + f.soft_deps if d in fingerprints)]
        f.fingerprint = hashlib.sha256(json.dumps(parts).encode()).hexdigest()[:16]
        fingerprints[f.name] = f.fingerprint

    # Cache keys. The part before the final hash only describes the environment, so a cache from an
    # earlier run of the same tier can always be found (restore-keys) and is then used framework by
    # framework: only those whose fingerprint still matches are trusted, the rest is rebuilt. The
    # exact key additionally covers every framework up to this tier and finds a finished tier directly.
    ctx_hash = hashlib.sha256(json.dumps(ctx, sort_keys=True).encode()).hexdigest()[:12]
    keys: dict[int, str] = {}
    restore_prefix: dict[int, str] = {}
    for g in range(1, MAX_GROUP + 1):
        upto = [(f.name, f.fingerprint) for f in fws if f.group <= g]
        chain = hashlib.sha256(json.dumps(upto).encode()).hexdigest()[:12]
        restore_prefix[g] = f"{key_base}-t{g}-{ctx_hash}-"
        keys[g] = f"{restore_prefix[g]}{chain}"

    plan = {
        "version": rel.version,
        "platform": plat,
        "qt": qt,
        "prefix": str(cfg.prefix),
        "work": str(cfg.work),
        "key_base": key_base,
        "keys": keys,
        "restore": restore_prefix,
        "frameworks": [asdict(f) for f in fws],
    }
    (cfg.work / "plan.json").write_text(json.dumps(plan, indent=2))
    (cfg.work / "status.json").write_text("{}")

    # ---- report
    with group("Build plan"):
        for g in range(1, MAX_GROUP + 1):
            members = [f for f in fws if f.group == g]
            if members:
                log(f"Tier {g}: " + ", ".join(f.name for f in members))
        for f in fws:
            if f.external:
                log(f"  note: {f.name} also lists non-framework KDE dependencies: {', '.join(f.external)}")

    lines = [f"### KDE Frameworks {rel.version} build plan ({plat}/{normalized_arch()}, Qt {qt['version']})", "",
             "| Tier | Frameworks |", "|---|---|"]
    for g in range(1, MAX_GROUP + 1):
        members = [f.name for f in fws if f.group == g]
        if members:
            lines.append(f"| {g} | {', '.join(members)} |")
    append_file("GITHUB_STEP_SUMMARY", "\n".join(lines) + "\n")

    for g in range(1, MAX_GROUP + 1):
        set_output(f"count-{g}", str(sum(1 for f in fws if f.group == g)))
        set_output(f"key-{g}", keys[g])
        set_output(f"restore-{g}", restore_prefix[g])
    set_output("work-dir", str(cfg.work))
    set_output("key-base", key_base)
    set_output("prefix", str(cfg.prefix))
    set_output("frameworks", ",".join(f.name for f in fws))
    set_output("kf-version", rel.version)
    set_output("qt-version", qt["version"])


# --------------------------------------------------------------------------
# state shared between steps
# --------------------------------------------------------------------------


def load_plan(cfg: Config) -> dict:
    path = cfg.work / "plan.json"
    if not path.exists():
        die(f"{path} not found: the 'plan' step has not run")
    return json.loads(path.read_text())


def load_status(cfg: Config) -> dict:
    path = cfg.work / "status.json"
    return json.loads(path.read_text()) if path.exists() else {}


def save_status(cfg: Config, status: dict) -> None:
    (cfg.work / "status.json").write_text(json.dumps(status, indent=2))


# --------------------------------------------------------------------------
# system packages
# --------------------------------------------------------------------------


def run_quiet(cmd: list[str], **kw) -> subprocess.CompletedProcess:
    return subprocess.run(cmd, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, **kw)


def cmd_sysdeps(cfg: Config) -> None:
    plan = load_plan(cfg)
    plat = plan["platform"]
    table = SYSTEM_DEPS.get(plat, {})
    packages: list[str] = list(table.get("*", []))
    for fw in plan["frameworks"]:
        packages += table.get(fw["name"], [])
    packages += cfg.extra_system_packages
    packages = list(dict.fromkeys(packages))
    sysdeps_state: dict[str, list[str]] = {"path": [], "prefix": []}

    with group(f"Installing system packages ({plat}): {' '.join(packages)}"):
        if plat == "Linux":
            sudo = [] if os.geteuid() == 0 else ["sudo"]
            env = dict(os.environ, DEBIAN_FRONTEND="noninteractive")
            subprocess.run(sudo + ["apt-get", "-y", "update"], env=env, check=False)
            base = sudo + ["apt-get", "-y", "install", "--no-install-recommends"]
            if subprocess.run(base + packages, env=env).returncode != 0:
                warning("bulk apt-get install failed, retrying package by package")
                for pkg in packages:
                    if subprocess.run(base + [pkg], env=env).returncode != 0:
                        warning(f"apt package '{pkg}' is unavailable; frameworks needing it may fail or lose features")
        elif plat == "macOS":
            env = dict(os.environ, HOMEBREW_NO_AUTO_UPDATE="1", HOMEBREW_NO_INSTALL_CLEANUP="1")
            for pkg in packages:
                if subprocess.run(["brew", "install", pkg], env=env).returncode != 0:
                    warning(f"brew package '{pkg}' could not be installed")
                    continue
                res = run_quiet(["brew", "--prefix", pkg], env=env)
                prefix = res.stdout.strip().splitlines()[-1] if res.returncode == 0 and res.stdout.strip() else ""
                if prefix and Path(prefix).is_dir():
                    # Many of these are keg-only: neither bin/ nor the libraries are
                    # reachable without being named explicitly.
                    sysdeps_state["prefix"].append(prefix)
                    if (Path(prefix) / "bin").is_dir():
                        sysdeps_state["path"].append(str(Path(prefix) / "bin"))
        elif plat == "Windows":
            for pkg in packages:
                res = subprocess.run(["choco", "install", pkg, "-y", "--no-progress", "--allow-empty-checksums"])
                if res.returncode not in (0, 1641, 3010):
                    warning(f"choco package '{pkg}' could not be installed")
        else:
            warning(f"no system package installer known for {plat}")

    (cfg.work / "sysdeps.json").write_text(json.dumps(sysdeps_state))
    if IN_CI:
        for p in sysdeps_state["path"]:
            append_file("GITHUB_PATH", p)


def load_sysdeps(cfg: Config) -> dict:
    path = cfg.work / "sysdeps.json"
    return json.loads(path.read_text()) if path.exists() else {"path": [], "prefix": []}


# --------------------------------------------------------------------------
# building
# --------------------------------------------------------------------------


def snapshot(prefix: Path) -> dict[str, tuple[int, int]]:
    snap: dict[str, tuple[int, int]] = {}
    if not prefix.exists():
        return snap
    stack = [prefix]
    while stack:
        current = stack.pop()
        with os.scandir(current) as it:
            for entry in it:
                if entry.is_dir(follow_symlinks=False):
                    stack.append(Path(entry.path))
                else:
                    st = entry.stat(follow_symlinks=False)
                    snap[os.path.relpath(entry.path, prefix)] = (st.st_size, st.st_mtime_ns)
    return snap


def copy_entry(src: Path, dst: Path) -> None:
    dst.parent.mkdir(parents=True, exist_ok=True)
    if os.path.lexists(dst):
        dst.unlink()
    if src.is_symlink():
        os.symlink(os.readlink(src), dst)
    else:
        shutil.copy2(src, dst)


MANIFEST = ".manifest.json"


def read_manifest(delta: Path) -> dict | None:
    path = delta / MANIFEST
    try:
        return json.loads(path.read_text())
    except (OSError, ValueError):
        return None


def write_delta(prefix: Path, before: dict, delta: Path, merge: bool = False) -> int:
    """Copy what changed in ``prefix`` since ``before`` into ``delta``.

    With ``merge`` the existing delta (a partial tier restored from cache) is kept.
    """
    if not merge:
        shutil.rmtree(delta, ignore_errors=True)
    delta.mkdir(parents=True, exist_ok=True)
    after = snapshot(prefix)
    changed = [rel for rel, meta in after.items() if before.get(rel) != meta]
    for rel in changed:
        copy_entry(prefix / rel, delta / rel)
    return len(changed)


def apply_delta(delta: Path, prefix: Path) -> int:
    count = 0
    for root, _dirs, files in os.walk(delta):
        for name in files:
            src = Path(root) / name
            rel = src.relative_to(delta)
            if str(rel) in {MANIFEST, ".complete"}:
                continue
            copy_entry(src, prefix / rel)
            count += 1
    return count


def build_env(cfg: Config, sysdeps: dict) -> dict:
    env = dict(os.environ)
    if sysdeps["path"]:
        env["PATH"] = os.pathsep.join(sysdeps["path"] + [env.get("PATH", "")])
    if current_platform() == "Windows":
        if not shutil.which("cl") and not env.get("CXX"):
            die("No MSVC compiler on PATH. Run ilammy/msvc-dev-cmd (or open a VS developer shell) before this action.")
        # Windows has no RPATH: programs run during the build (meinproc6, ...) find the DLLs of the
        # frameworks installed so far only through PATH. Without this they exit with 0xC0000135.
        env["PATH"] = os.pathsep.join([str(cfg.prefix / "bin"), env.get("PATH", "")])
    return env


def tail(path: Path, lines: int = 80) -> str:
    try:
        return "\n".join(path.read_text(errors="replace").splitlines()[-lines:])
    except OSError:
        return ""


def error_context(path: Path) -> str:
    """The CMake / compiler error lines, so the cause is visible without scrolling."""
    try:
        text = path.read_text(errors="replace").splitlines()
    except OSError:
        return ""
    picked: list[str] = []
    for i, line in enumerate(text):
        # Harmless: the aqtinstall Qt's QML plugin metadata mentions a module that is not
        # installed, and Vulkan headers are optional.
        if "Qt6TaskTree" in line or "WrapVulkanHeaders" in line:
            continue
        if re.search(r"CMake Error|FAILED:|error:|fatal error|Could NOT find|Could not find", line):
            picked += text[i:i + 4]
            if len(picked) > 60:
                break
    return "\n".join(picked)


def missing_qt_module_hint(path: Path) -> str:
    """Name the aqtinstall module when a configure failed for a missing Qt component."""
    try:
        text = path.read_text(errors="replace")
    except OSError:
        return ""
    modules = sorted(set(re.findall(r'Failed to find required Qt component "(\w+)"', text)))
    if not modules:
        return ""
    special = {"TextToSpeech": "qtspeech"}  # aqtinstall name differs from the Qt module name
    names = ", ".join(f"{m} (aqtinstall module: {special.get(m, 'qt' + m.lower())})" for m in modules)
    return f"Missing Qt module(s): {names}. Install them with the Qt installer (e.g. `modules:` of install-qt-action)."


def run_logged(cfg: Config, cmd: list[str], logfile: Path, env: dict) -> int:
    logfile.parent.mkdir(parents=True, exist_ok=True)
    with open(logfile, "w", encoding="utf-8", errors="replace") as fh:
        fh.write("$ " + " ".join(shlex.quote(c) for c in cmd) + "\n\n")
        fh.flush()
        proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, env=env, text=True,
                                encoding="utf-8", errors="replace")
        assert proc.stdout is not None
        for line in proc.stdout:
            fh.write(line)
            if cfg.verbose:
                sys.stdout.write(line)
        return proc.wait()


def pkg_config_has(module: str) -> bool:
    exe = shutil.which("pkg-config") or shutil.which("pkgconf")
    if not exe:
        return False
    try:
        return subprocess.run([exe, "--exists", module], stdout=subprocess.DEVNULL,
                              stderr=subprocess.DEVNULL, timeout=30).returncode == 0
    except (OSError, subprocess.SubprocessError):
        return False


def cmake_defs(cfg: Config, plan: dict, sysdeps: dict, fw: dict) -> list[str]:
    prefix = Path(plan["prefix"]).as_posix()
    # Prefixes the caller already exported (e.g. a vcpkg tree or tools built by an earlier
    # step) are honoured by CMake anyway; listing them makes that visible and explicit.
    inherited = [p for p in os.environ.get("CMAKE_PREFIX_PATH", "").split(os.pathsep) if p]
    prefix_path = ";".join(dict.fromkeys(
        [prefix, Path(plan["qt"]["prefix"]).as_posix()]
        + [Path(p).as_posix() for p in sysdeps["prefix"]]
        + [Path(p).as_posix() for p in inherited]
    ))
    defs = [
        f"-DCMAKE_BUILD_TYPE={cfg.build_type}",
        f"-DCMAKE_INSTALL_PREFIX={prefix}",
        f"-DCMAKE_PREFIX_PATH={prefix_path}",
        f"-DBUILD_SHARED_LIBS={'ON' if cfg.linkage == 'shared' else 'OFF'}",
    ]
    if cfg.qt_host_path:
        defs.append(f"-DQT_HOST_PATH={Path(cfg.qt_host_path).as_posix()}")
    if cfg.osx_architectures:
        defs.append(f"-DCMAKE_OSX_ARCHITECTURES={cfg.osx_architectures}")
    defs += BASE_CMAKE_DEFS
    defs += FRAMEWORK_CMAKE_DEFS.get(fw["name"], [])
    feature = QT_MODULE_FEATURES.get(fw["name"])
    if feature and not (Path(plan["qt"]["prefix"]) / "lib" / "cmake" / f"Qt6{feature[1]}").is_dir():
        log(f"  {fw['name']}: Qt{feature[1]} is not installed, building with -D{feature[0]}=OFF")
        defs.append(f"-D{feature[0]}=OFF")
    if fw["name"] == "karchive" and not pkg_config_has("libzstd"):
        # karchive finds zstd only through pkg-config and treats it as REQUIRED by default.
        log("  karchive: libzstd is not visible to pkg-config, building with -DWITH_LIBZSTD=OFF")
        defs.append("-DWITH_LIBZSTD=OFF")
    if current_platform() == "Windows":
        # ECM's app template packaging runs the first `tar` it finds. On the runners that is
        # Git's GNU tar, which reads "D:/x" as host "D", so point it at Windows' own bsdtar
        # (it then falls back to `cmake -E tar`).
        win_tar = Path(os.environ.get("SystemRoot", r"C:\Windows")) / "System32" / "tar.exe"
        if win_tar.exists():
            defs.append(f"-D_tar_executable={win_tar.as_posix()}")
    defs += user_cmake_args(cfg)
    defs += per_framework_args(cfg).get(fw["name"], [])
    return defs


def build_one(cfg: Config, plan: dict, sysdeps: dict, fw: dict, env: dict) -> bool:
    name = fw["name"]
    src = Path(fw["src"])
    build_dir = cfg.work / "build" / name
    logs = cfg.work / "logs"
    shutil.rmtree(build_dir, ignore_errors=True)

    steps = [
        ("configure", ["cmake", "-S", str(src), "-B", str(build_dir), "-G", cfg.generator]
         + cmake_defs(cfg, plan, sysdeps, fw)),
        ("build", ["cmake", "--build", str(build_dir), "--parallel", str(cfg.jobs)]),
        ("install", ["cmake", "--install", str(build_dir)]),
    ]
    for label, cmd in steps:
        logfile = logs / f"{name}-{label}.log"
        rc = run_logged(cfg, cmd, logfile, env)
        if rc != 0:
            with group(f"{name}: {label} FAILED (exit {rc}) - log: {logfile}"):
                ctx = error_context(logfile)
                hint = missing_qt_module_hint(logfile)
                if hint:
                    log("HINT: " + hint)
                if ctx:
                    log("---- errors ----\n" + ctx + "\n---- end of errors ----")
                log("---- last lines of the log ----\n" + tail(logfile))
            if hint:
                log(f"::error title=KDE Frameworks: {name} needs another Qt module::{hint}" if IN_CI else f"error: {hint}")
            log(f"::error title=KDE Frameworks: {name} {label} failed::see the '{name}: {label} FAILED' group above"
                if IN_CI else f"error: {name} {label} failed")
            return False
    shutil.rmtree(build_dir, ignore_errors=True)  # keep the runner's disk free
    return True


def cmd_build(cfg: Config, tier: int) -> None:
    plan = load_plan(cfg)
    status = load_status(cfg)
    sysdeps = load_sysdeps(cfg)
    prefix = Path(plan["prefix"])
    delta = cfg.work / "delta" / f"tier{tier}"
    members = [f for f in plan["frameworks"] if f["group"] == tier]
    prefix.mkdir(parents=True, exist_ok=True)
    set_output("save", "false")

    if not members:
        return

    # A restored cache holds a manifest listing what it contains: either the whole tier,
    # or (after an earlier run in which something failed) the frameworks that did build.
    already: set[str] = set()
    manifest = read_manifest(delta)
    if manifest:
        count = apply_delta(delta, prefix)
        # The cache may come from a run with different settings (restore-keys). Only trust a
        # framework if it was built exactly the way it would be built now.
        cached = manifest.get("frameworks", {})
        already = {m["name"] for m in members if cached.get(m["name"]) == m["fingerprint"]}
        stale = sorted(m["name"] for m in members if m["name"] in cached and m["name"] not in already)
        for fw in members:
            if fw["name"] in already:
                status[fw["name"]] = {"state": "cached", "seconds": 0}
        save_status(cfg, status)
        if stale:
            log(f"Tier {tier}: rebuilding {', '.join(stale)} (built earlier with different settings)")
        if len(already) == len(members):
            notice(f"Tier {tier}: restored {len(members)} framework(s) from cache ({count} files)")
            return
        notice(f"Tier {tier}: resuming, {len(already)}/{len(members)} framework(s) restored from cache ({count} files)")
    else:
        shutil.rmtree(delta, ignore_errors=True)  # an incomplete leftover

    env = build_env(cfg, sysdeps)
    before = snapshot(prefix)  # after restoring, so the new delta only holds what we add
    failed = False
    built_now: list[str] = []
    t_tier = time.time()
    todo = [m for m in members if m["name"] not in already]
    log(f"Tier {tier}: building {len(todo)} framework(s): {', '.join(m['name'] for m in todo)}")
    for fw in todo:
        name = fw["name"]
        blockers = [d for d in fw["deps"] if status.get(d, {}).get("state") in {"failed", "skipped"}]
        if blockers:
            status[name] = {"state": "skipped", "seconds": 0, "reason": f"depends on failed {', '.join(blockers)}"}
            log(f"  - {name}: skipped (depends on failed {', '.join(blockers)})")
            failed = True
            continue
        start = time.time()
        ok = build_one(cfg, plan, sysdeps, fw, env)
        seconds = round(time.time() - start)
        status[name] = {"state": "built" if ok else "failed", "seconds": seconds}
        save_status(cfg, status)
        log(f"  {'✔' if ok else '✘'} {name} ({seconds}s)")
        if ok:
            built_now.append(name)
        else:
            failed = True
            if not cfg.keep_going:
                break

    save_status(cfg, status)
    log(f"Tier {tier} finished in {round(time.time() - t_tier)}s")
    # Save what was built even if something failed, so the next run does not start from
    # scratch (a tier can take half an hour). A fully built tier gets the exact key and is
    # reused as a whole; a partial one gets a unique "-partial-" key that later runs resume from.
    if cfg.cache and built_now:
        files = write_delta(prefix, before, delta, merge=bool(manifest))
        complete = len(already) + len(built_now) == len(members)
        fingerprints = {m["name"]: m["fingerprint"] for m in members if m["name"] in already | set(built_now)}
        (delta / MANIFEST).write_text(json.dumps({"frameworks": fingerprints, "complete": complete}))
        key = plan["keys"][str(tier)]
        if not complete:
            run = f"{os.environ.get('GITHUB_RUN_ID', int(time.time()))}-{os.environ.get('GITHUB_RUN_ATTEMPT', '1')}"
            key = f"{plan['restore'][str(tier)]}partial-{run}"
        log(f"Tier {tier}: cache {'complete' if complete else 'partial'}, {files} new file(s), key {key}")
        set_output("save", "true")
        set_output("save-key", key)
    if failed and not cfg.keep_going:
        sys.exit(1)
    # With keep-going the failure is reported by the 'finish' step, so later
    # tiers can still show everything that is broken in one run.


# --------------------------------------------------------------------------
# finishing
# --------------------------------------------------------------------------


def lib_subdirs(prefix: Path, leaf: str) -> list[str]:
    found: list[str] = []
    for libdir in [prefix / "lib", prefix / "lib64"] + sorted((prefix / "lib").glob("*-linux-gnu")):
        cand = libdir / leaf
        if cand.is_dir():
            found.append(str(cand))
    return found


def join_paths(new: list[str], existing: str) -> str:
    return os.pathsep.join([p for p in new + [existing] if p])


def cmd_finish(cfg: Config) -> None:
    if not (cfg.work / "plan.json").exists():
        return  # planning itself failed; the error was already reported
    plan = load_plan(cfg)
    status = load_status(cfg)
    prefix = Path(plan["prefix"])
    sysdeps = load_sysdeps(cfg)

    rows = ["", "| Framework | Tier | Result | Time |", "|---|---|---|---|"]
    icons = {"built": "✅ built", "cached": "📦 cached", "failed": "❌ failed", "skipped": "⏭️ skipped"}
    failures: list[str] = []
    for fw in plan["frameworks"]:
        st = status.get(fw["name"], {"state": "not run", "seconds": 0})
        if st["state"] in {"failed", "skipped", "not run"}:
            failures.append(fw["name"])
        note = f" ({st['reason']})" if st.get("reason") else ""
        seconds = f"{st['seconds']}s" if st["seconds"] else ""
        rows.append(f"| {fw['name']} | {fw['group']} | {icons.get(st['state'], st['state'])}{note} | {seconds} |")
    append_file("GITHUB_STEP_SUMMARY", "\n".join(rows) + "\n")

    all_cached = all(status.get(f["name"], {}).get("state") == "cached" for f in plan["frameworks"])
    set_output("prefix", str(prefix))
    set_output("cache-hit", "true" if all_cached else "false")
    set_output("failed", ",".join(failures))

    if failures:
        message = "KDE Frameworks failed/skipped: " + ", ".join(failures)
        if cfg.ignore_failures:
            warning(message)
        else:
            die(message)

    if cfg.export_env:
        export: dict[str, str] = {}
        export["KF6_PREFIX"] = str(prefix)
        export["CMAKE_PREFIX_PATH"] = join_paths(
            [str(prefix)] + sysdeps["prefix"], os.environ.get("CMAKE_PREFIX_PATH", "")
        )
        plugins = lib_subdirs(prefix, "plugins")
        qml = lib_subdirs(prefix, "qml")
        if plugins:
            export["QT_PLUGIN_PATH"] = join_paths(plugins, os.environ.get("QT_PLUGIN_PATH", ""))
        if qml:
            export["QML_IMPORT_PATH"] = join_paths(qml, os.environ.get("QML_IMPORT_PATH", ""))
            export["QML2_IMPORT_PATH"] = join_paths(qml, os.environ.get("QML2_IMPORT_PATH", ""))
        if plan["platform"] != "Windows":
            default = os.environ.get("XDG_DATA_DIRS", "/usr/local/share:/usr/share")
            export["XDG_DATA_DIRS"] = join_paths([str(prefix / "share")], default)
        for key, value in export.items():
            append_file("GITHUB_ENV", f"{key}={value}")
        for p in [str(prefix / "bin")] + ([str(prefix / "libexec")] if (prefix / "libexec").is_dir() else []):
            append_file("GITHUB_PATH", p)
        log("Exported: " + ", ".join(export) + ", PATH")


# --------------------------------------------------------------------------


def main(argv: list[str]) -> None:
    # Windows runners with Python < 3.15 default stdout to the ANSI code page (cp1252),
    # which can't encode the status glyphs used in log().
    for _stream in (sys.stdout, sys.stderr):
        try:
            _stream.reconfigure(encoding="utf-8", errors="replace")
        except (AttributeError, ValueError):
            pass
    if len(argv) < 2 or argv[1] not in {"plan", "sysdeps", "build", "finish"}:
        die("usage: kf6.py plan|sysdeps|build <tier>|finish")
    cfg = Config()
    command = argv[1]
    if command == "plan":
        cmd_plan(cfg)
    elif command == "sysdeps":
        cmd_sysdeps(cfg)
    elif command == "build":
        if len(argv) < 3:
            die("usage: kf6.py build <tier>")
        cmd_build(cfg, int(argv[2]))
    else:
        cmd_finish(cfg)


if __name__ == "__main__":
    main(sys.argv)
