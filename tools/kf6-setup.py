#!/usr/bin/env python3
"""Build KDE Frameworks 6 for MeshMC development (Windows and macOS).

MeshMC's CI builds KDE Frameworks 6 from the official release tarballs with
.github/actions/setup-kf6/kf6.py. This tool runs that very same code on a
developer machine, so a local build uses the same sources, versions and
CMake arguments as CI. What it adds on top:

  * finds your Qt 6 (or takes --qt-prefix) and, on Windows, enters the Visual
    Studio developer environment for you
  * checks the native prerequisites and installs what is missing, *after asking*
    (Homebrew packages on macOS; vcpkg ports, SnoreToast, DocBook and a few
    Chocolatey tools on Windows)
  * keeps everything under one directory (see --root), never in system locations
  * remembers what it built: running it again is instant, and after a failure the
    next run continues with what is still missing
  * writes an env script (env.sh / env.ps1) to source before configuring MeshMC

On Linux install KDE Frameworks 6 from your distribution (installing Plasma
pulls it in anyway); this tool does not cover Linux.

Examples:
  python tools/kf6-setup.py --qt-prefix ~/Qt/6.12.0/macos
  python tools/kf6-setup.py --frameworks "kio kxmlgui"     # only what you need
  python tools/kf6-setup.py --plan-only                    # show the plan, change nothing
"""

from __future__ import annotations

import argparse
import importlib.util
import io
import json
import os
import platform
import re
import shlex
import shutil
import subprocess
import sys
import zipfile
from pathlib import Path

sys.dont_write_bytecode = True  # keep the repository free of __pycache__

REPO = Path(__file__).resolve().parent.parent
KF6_PY = REPO / ".github" / "actions" / "setup-kf6" / "kf6.py"

# Keep in sync with .github/actions/setup-kf6/action.yml ("Install system dependencies (Windows)").
WIN_VCPKG_PORTS = [
    "gettext-libintl", "gperf", "zlib", "bzip2", "liblzma", "libxml2", "iso-codes", "cmark",
    "libarchive", "libqrencode", "libxslt", "zstd", "libical", "libdmtx", "nu-book-zxing-cpp",
]
# command that must be on PATH -> Chocolatey package that provides it
WIN_CHOCO_TOOLS = [
    (("pkg-config",), "pkgconfiglite"),
    (("msgfmt",), "gettext"),
    (("win_bison", "bison"), "winflexbison3"),
    (("win_flex", "flex"), "winflexbison3"),
]
SNORETOAST_TAG = "v0.9.1"  # knotifications needs it on Windows
DOCBOOK_DTD_URL = "https://www.oasis-open.org/docbook/xml/4.5/docbook-xml-4.5.zip"
DOCBOOK_XSL_URL = (
    "https://github.com/docbook/xslt10-stylesheets/releases/download/"
    "snapshot%2F2020-06-03/docbook-xsl-snapshot.zip"
)
# Homebrew packages that this tool needs in addition to what kf6.py lists for macOS,
# and the commands that make a package unnecessary if you already have them.
MAC_EXTRA_PACKAGES = ["cmake", "ninja", "zstd"]
MAC_SATISFIED_BY_COMMAND = {"pkgconf": "pkg-config", "cmake": "cmake", "ninja": "ninja"}

# environment variables that finish() builds from "new entries + whatever is already set"
LIST_VARS = ["CMAKE_PREFIX_PATH", "QT_PLUGIN_PATH", "QML_IMPORT_PATH", "QML2_IMPORT_PATH", "XDG_DATA_DIRS"]
XDG_DEFAULT = ["/usr/local/share", "/usr/share"]

kf = None  # kf6.py, loaded by load_kf6()


# --------------------------------------------------------------------------
# small helpers
# --------------------------------------------------------------------------


def say(msg: str = "") -> None:
    print(msg, flush=True)


def fail(msg: str, code: int = 1) -> None:
    print(f"error: {msg}", file=sys.stderr, flush=True)
    sys.exit(code)


def load_kf6():
    global kf
    if not KF6_PY.exists():
        fail(f"{KF6_PY} not found; run this from a MeshMC checkout.")
    os.environ.pop("GITHUB_ACTIONS", None)  # kf6.py must not emit CI workflow commands
    spec = importlib.util.spec_from_file_location("kf6", KF6_PY)
    kf = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(kf)


def run(cmd: list[str], **kw) -> int:
    say("$ " + " ".join(shlex.quote(str(c)) for c in cmd))
    return subprocess.run([str(c) for c in cmd], **kw).returncode


def arch_is_arm() -> bool:
    return platform.machine().lower() in {"arm64", "aarch64"}


def confirm(question: str, assume_yes: bool) -> bool:
    if assume_yes:
        return True
    if not sys.stdin.isatty():
        return False
    return input(f"{question} [y/N] ").strip().lower() in {"y", "yes"}


# --------------------------------------------------------------------------
# Qt and Visual Studio
# --------------------------------------------------------------------------


def version_key(path: Path) -> tuple:
    return tuple(int(p) for p in re.findall(r"\d+", path.parent.name))


def find_qt(explicit: str, plat: str) -> str:
    """Return a Qt 6 prefix: --qt-prefix, the usual environment variables, or a scan of the usual places."""
    def valid(p: str) -> bool:
        return bool(p) and (Path(p) / "lib" / "cmake" / "Qt6Core" / "Qt6CoreConfig.cmake").exists()

    if explicit:
        if not valid(explicit):
            fail(f"--qt-prefix {explicit} is not a Qt 6 installation (lib/cmake/Qt6Core is missing).")
        return explicit
    for var in ("QT_ROOT_DIR", "QTDIR"):
        if valid(os.environ.get(var, "")):
            return os.environ[var]
    qt6_dir = os.environ.get("Qt6_DIR", "")
    if qt6_dir and valid(str(Path(qt6_dir).parents[2])):
        return str(Path(qt6_dir).parents[2])

    roots = [Path.home() / "Qt"] + ([Path("C:/Qt")] if plat == "Windows" else [])
    if plat == "Windows":
        kit = re.compile(r"msvc\d+_arm64$" if arch_is_arm() else r"msvc\d+_64$")
    else:
        kit = re.compile(r"(macos|clang_64)$")
    found = sorted(
        (c for root in roots for c in root.glob("6.*/*") if kit.match(c.name) and valid(str(c))),
        key=version_key,
    )
    if found:
        say(f"Using Qt {found[-1]}" + (f" (also found: {', '.join(str(f) for f in found[:-1])})" if len(found) > 1 else ""))
        return str(found[-1])
    if plat == "macOS":
        res = subprocess.run(["brew", "--prefix", "qt"], capture_output=True, text=True) if shutil.which("brew") else None
        if res and res.returncode == 0 and valid(res.stdout.strip()):
            say(f"Using Homebrew's Qt: {res.stdout.strip()}")
            return res.stdout.strip()
    fail("No Qt 6 installation found. Install Qt 6 (Qt online installer, or `pip install aqtinstall`) "
         "and pass --qt-prefix <dir containing lib/cmake/Qt6Core>.")
    raise AssertionError


def enter_vs_environment() -> None:
    """Import the Visual Studio developer environment (like a 'Developer Command Prompt') if cl is not on PATH."""
    if shutil.which("cl"):
        return
    pf86 = os.environ.get("ProgramFiles(x86)", r"C:\Program Files (x86)")
    vswhere = Path(pf86) / "Microsoft Visual Studio" / "Installer" / "vswhere.exe"
    if not vswhere.exists():
        fail("Visual Studio with the 'Desktop development with C++' workload is required "
             "(vswhere.exe not found). Install it, or run this from a Developer PowerShell.")
    res = subprocess.run(
        [str(vswhere), "-latest", "-products", "*", "-property", "installationPath",
         "-requires", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64"],
        capture_output=True, text=True)
    root = res.stdout.strip().splitlines()[0] if res.stdout.strip() else ""
    bat = Path(root) / "VC" / "Auxiliary" / "Build" / "vcvarsall.bat"
    if not root or not bat.exists():
        fail("No Visual Studio installation with the C++ tools found.")
    target = "arm64" if arch_is_arm() else "amd64"
    say(f"Entering the Visual Studio environment ({bat}, {target})")
    res = subprocess.run(f'cmd.exe /d /s /c ""{bat}" {target} >nul && set"',
                         capture_output=True, text=True)
    if res.returncode != 0:
        fail("vcvarsall.bat failed:\n" + res.stdout + res.stderr)
    for line in res.stdout.splitlines():
        name, sep, value = line.partition("=")
        if sep and name:
            os.environ[name] = value
    if not shutil.which("cl"):
        fail("cl.exe is still not on PATH after running vcvarsall.bat.")


# --------------------------------------------------------------------------
# prerequisites: each returns (pending actions, function that records the resulting state)
# An action is (description, callable). Nothing is changed before the user agreed.
# --------------------------------------------------------------------------


def brew_has(pkg: str) -> bool:
    cmd = MAC_SATISFIED_BY_COMMAND.get(pkg)
    if cmd and shutil.which(cmd):
        return True
    return subprocess.run(["brew", "list", "--versions", pkg], capture_output=True).returncode == 0


def collect_macos(names: list[str]):
    if not shutil.which("brew"):
        fail("Homebrew is required for the native dependencies: https://brew.sh")
    if subprocess.run(["xcode-select", "-p"], capture_output=True).returncode != 0:
        fail("The Xcode command line tools are missing: run `xcode-select --install`.")
    table = kf.SYSTEM_DEPS["macOS"]
    pkgs = list(table.get("*", []))
    for n in names:
        pkgs += table.get(n, [])
    pkgs = list(dict.fromkeys(pkgs + MAC_EXTRA_PACKAGES))
    missing = [p for p in pkgs if not brew_has(p)]

    actions = []
    if missing:
        # Never upgrade what the user already has.
        env = dict(os.environ, HOMEBREW_NO_AUTO_UPDATE="1", HOMEBREW_NO_INSTALL_CLEANUP="1",
                   HOMEBREW_NO_INSTALL_UPGRADE="1")

        def install() -> None:
            for pkg in missing:
                if run(["brew", "install", pkg], env=env) != 0:
                    fail(f"brew install {pkg} failed")
        actions.append((f"brew install {' '.join(missing)}", install))

    if "kdoctools" in names and subprocess.run(["perl", "-MURI", "-e1"], capture_output=True).returncode != 0:
        say("warning: kdoctools needs the Perl module URI, which is missing "
            "(install it with `cpanm --local-lib ~/perl5 URI` and set PERL5LIB, or `brew install cpanminus`).")

    def state() -> dict:
        st = {"path": [], "prefix": []}
        for pkg in pkgs:
            if pkg in MAC_SATISFIED_BY_COMMAND:
                continue
            res = subprocess.run(["brew", "--prefix", pkg], capture_output=True, text=True)
            prefix = res.stdout.strip().splitlines()[-1] if res.returncode == 0 and res.stdout.strip() else ""
            if prefix and Path(prefix).is_dir():
                # Many are keg-only: neither bin/ nor the libraries are reachable by default.
                st["prefix"].append(prefix)
                if (Path(prefix) / "bin").is_dir():
                    st["path"].append(str(Path(prefix) / "bin"))
        return st

    return actions, state


def find_vcpkg(explicit: str) -> Path:
    cands = [explicit, os.environ.get("VCPKG_ROOT", ""), os.environ.get("VCPKG_INSTALLATION_ROOT", ""),
             str(REPO / "cmake" / "vcpkg")]
    on_path = shutil.which("vcpkg")
    if on_path:
        cands.append(str(Path(on_path).parent))
    for c in cands:
        if c and (Path(c) / "vcpkg.exe").exists():
            return Path(c)
    fail("vcpkg.exe not found. Bootstrap the checkout in cmake/vcpkg (`cmake\\vcpkg\\bootstrap-vcpkg.bat`), "
         "or pass --vcpkg-root.")
    raise AssertionError


def vcpkg_installed(root: Path, triplet: str) -> set[str]:
    info = root / "installed" / "vcpkg" / "info"
    return {p.name.split("_")[0] for p in info.glob(f"*_{triplet}.list")}


def tool_dirs(triplet_root: Path) -> list[str]:
    """Directories in the vcpkg tree that hold executables (gperf, ...), as CI collects them."""
    exts = {".exe", ".cmd", ".bat", ".com"}
    # bin/ always (its DLLs are needed at run time), the tools root itself too, like CI.
    found = [str(triplet_root / "bin")] if (triplet_root / "bin").is_dir() else []
    for d in sorted(p for p in (triplet_root / "tools").rglob("*") if p.is_dir()):
        if any(f.suffix.lower() in exts for f in d.iterdir() if f.is_file()):
            found.append(str(d))
    if (triplet_root / "tools").is_dir():
        found.append(str(triplet_root / "tools"))
    return list(dict.fromkeys(found))


def build_snoretoast(deps: Path, scratch: Path) -> None:
    src, bld = scratch / "snoretoast-src", scratch / "snoretoast-build"
    shutil.rmtree(src, ignore_errors=True)
    shutil.rmtree(bld, ignore_errors=True)
    steps = [
        ["git", "clone", "--depth", "1", "--branch", SNORETOAST_TAG, "https://github.com/KDE/snoretoast.git", src],
        # The frameworks use the dynamic CRT (/MD); a static runtime would mix the two.
        ["cmake", "-S", src, "-B", bld, "-G", "Ninja", "-DCMAKE_BUILD_TYPE=Release",
         f"-DCMAKE_INSTALL_PREFIX={deps}", "-DBUILD_STATIC_RUNTIME=OFF"],
        ["cmake", "--build", bld],
        ["cmake", "--install", bld],
    ]
    for step in steps:
        if run(step) != 0:
            fail(f"SnoreToast: '{step[0]} {step[1]}' failed")


def find_xsl_root(base: Path) -> Path | None:
    """The DocBook stylesheets' root is the directory that contains lib/lib.xsl."""
    for lib in base.rglob("lib.xsl"):
        if lib.parent.name == "lib":
            return lib.parent.parent
    return None


def ensure_docbook(base: Path) -> None:
    """Unpack the DocBook DTD and stylesheets kdoctools needs and point the environment at them."""
    dtd, xsl = base / "dtd", base / "xsl"
    if not (dtd / "docbookx.dtd").exists():
        say("Downloading the DocBook XML 4.5 DTD")
        data = kf.http_get(DOCBOOK_DTD_URL)
        shutil.rmtree(dtd, ignore_errors=True)
        zipfile.ZipFile(io.BytesIO(data)).extractall(dtd)
    if not (dtd / "docbookx.dtd").exists():
        fail(f"docbookx.dtd not found in {dtd}")
    if not find_xsl_root(xsl):
        say("Downloading the DocBook XSL stylesheets")
        data = kf.http_get(DOCBOOK_XSL_URL)
        shutil.rmtree(xsl, ignore_errors=True)
        zipfile.ZipFile(io.BytesIO(data)).extractall(xsl)
    xsl_root = find_xsl_root(xsl)
    if not xsl_root:
        fail(f"lib/lib.xsl not found in the DocBook XSL archive ({xsl})")
    os.environ["XML_CATALOG_FILES"] = str(dtd / "catalog.xml")
    os.environ["DocBookXML4_ROOT"] = str(dtd)
    # kf6.py appends this to kdoctools' CMake arguments (per_framework_args).
    os.environ["KF6_KDOCTOOLS_ARGS"] = f"-DDocBookXML4_DTD_DIR={dtd.as_posix()} -DDocBookXSL_DIR={xsl_root.as_posix()}"


def collect_windows(names: list[str], root: Path, work: Path, vcpkg_arg: str):
    for tool in ("cmake", "ninja", "git"):
        if not shutil.which(tool):
            fail(f"'{tool}' is not on PATH. Install it (e.g. `winget install Kitware.CMake Ninja-build.Ninja Git.Git`); "
                 "the Visual Studio 'C++ CMake tools' component provides cmake and ninja too.")
    vcpkg = find_vcpkg(vcpkg_arg)
    triplet = "arm64-windows" if arch_is_arm() else "x64-windows"
    triplet_root = vcpkg / "installed" / triplet
    deps = root / "deps"

    actions = []
    missing_ports = [p for p in WIN_VCPKG_PORTS if p not in vcpkg_installed(vcpkg, triplet)]
    if missing_ports:
        def install_ports() -> None:
            # Classic mode into this vcpkg's installed/ tree, like CI. Run outside the repo so a
            # vcpkg.json manifest does not interfere.
            if run([vcpkg / "vcpkg.exe", "install", "--classic", *WIN_VCPKG_PORTS, f"--triplet={triplet}"],
                   cwd=work) != 0:
                fail("vcpkg install failed")
        actions.append((f"vcpkg install --classic {' '.join(missing_ports)} --triplet={triplet}  ({vcpkg})",
                        install_ports))

    choco_pkgs = []
    for cmds, pkg in WIN_CHOCO_TOOLS:
        if not any(shutil.which(c) for c in cmds) and pkg not in choco_pkgs:
            choco_pkgs.append(pkg)
    if choco_pkgs:
        if not shutil.which("choco"):
            fail(f"Missing tools: install the Chocolatey packages {', '.join(choco_pkgs)} (https://chocolatey.org/install) "
                 "or put equivalents on PATH.")

        def install_choco() -> None:
            for pkg in choco_pkgs:
                rc = run(["choco", "install", pkg, "-y", "--no-progress", "--allow-empty-checksums"])
                if rc not in (0, 1641, 3010):
                    fail(f"choco install {pkg} failed (it needs an elevated shell)")
        actions.append((f"choco install {' '.join(choco_pkgs)}  (needs an elevated shell)", install_choco))

    if "knotifications" in names and not list((deps / "bin").glob("SnoreToast*")):
        actions.append((f"build SnoreToast {SNORETOAST_TAG} into {deps}", lambda: build_snoretoast(deps, work)))

    def state() -> dict:
        os.environ["PKG_CONFIG_PATH"] = os.pathsep.join(
            [str(triplet_root / "lib" / "pkgconfig")] + ([os.environ["PKG_CONFIG_PATH"]] if os.environ.get("PKG_CONFIG_PATH") else []))
        return {"path": tool_dirs(triplet_root) + ([str(deps / "bin")] if (deps / "bin").is_dir() else []),
                "prefix": [str(triplet_root)] + ([str(deps)] if deps.is_dir() else [])}

    return actions, state


# --------------------------------------------------------------------------
# env scripts
# --------------------------------------------------------------------------


def read_lines(path: Path) -> list[str]:
    return [l.strip() for l in path.read_text(encoding="utf-8").splitlines() if l.strip()] if path.exists() else []


def write_env_script(dest: Path, plat: str, sets: dict[str, str], prepends: dict[str, list[str]]) -> Path:
    """Write a script that prepends to the variables (never replaces what the user already has)."""
    if plat == "Windows":
        out = dest / "env.ps1"
        lines = ["# Generated by tools/kf6-setup.py. Dot-source it: . .\\env.ps1"]
        for k, v in sets.items():
            lines.append(f"$env:{k} = '{v.replace(chr(39), chr(39) * 2)}'")
        for k, items in prepends.items():
            joined = ";".join(items).replace("'", "''")
            lines.append(f"$env:{k} = '{joined}' + $(if ($env:{k}) {{ ';' + $env:{k} }} else {{ '' }})")
    else:
        out = dest / "env.sh"
        lines = ["# Generated by tools/kf6-setup.py. Source it: . ./env.sh"]
        for k, v in sets.items():
            lines.append(f"export {k}={shlex.quote(v)}")
        for k, items in prepends.items():
            # An unset XDG_DATA_DIRS means "/usr/local/share:/usr/share": keep that default.
            rest = f"${{{k}:-{':'.join(XDG_DEFAULT)}}}" if k == "XDG_DATA_DIRS" else f"${{{k}:+:${k}}}"
            sep = ":" if k == "XDG_DATA_DIRS" else ""
            lines.append(f"export {k}={shlex.quote(':'.join(items) + sep)}\"{rest}\"")
    out.write_text("\n".join(lines) + "\n", encoding="utf-8")
    return out


# --------------------------------------------------------------------------
# main
# --------------------------------------------------------------------------


def default_root(plat: str) -> Path:
    if plat == "Windows":
        return Path(os.environ.get("LOCALAPPDATA", str(Path.home() / "AppData" / "Local"))) / "MeshMC-dev" / "kf6"
    return Path.home() / ".meshmc-dev" / "kf6"


def main() -> None:
    ap = argparse.ArgumentParser(
        description="Build KDE Frameworks 6 for MeshMC development (Windows and macOS).",
        formatter_class=argparse.RawDescriptionHelpFormatter, epilog="Examples:" + __doc__.split("Examples:")[1])
    ap.add_argument("--qt-prefix", default="", help="Qt 6 installation (contains lib/cmake/Qt6Core); found automatically if omitted")
    ap.add_argument("--frameworks", default="", help="what to build, e.g. \"kio kxmlgui\"; dependencies are added (default: all, like CI)")
    ap.add_argument("--exclude", default="", help="frameworks to leave out")
    ap.add_argument("--kf-version", default="", help="KDE Frameworks release (default: the one CI uses)")
    ap.add_argument("--build-type", default="Release", choices=["Release", "RelWithDebInfo", "Debug", "MinSizeRel"],
                    help="default: Release, as in CI")
    ap.add_argument("--root", default="", help="where everything lives (default: ~/.meshmc-dev/kf6, %%LOCALAPPDATA%%\\MeshMC-dev\\kf6)")
    ap.add_argument("--vcpkg-root", default="", help="Windows: vcpkg checkout to take the native libraries from (default: $VCPKG_ROOT or cmake/vcpkg)")
    ap.add_argument("--jobs", type=int, default=0, help="parallel jobs (default: CPU count)")
    ap.add_argument("--keep-going", action="store_true", help="after a failure keep building what does not depend on it")
    ap.add_argument("--verbose", action="store_true", help="stream the full build output")
    ap.add_argument("--yes", "-y", action="store_true", help="install missing prerequisites without asking")
    ap.add_argument("--plan-only", action="store_true", help="show the plan and the missing prerequisites; change nothing")
    ap.add_argument("--force", action="store_true", help="run the build step even if this exact setup is already built")
    args = ap.parse_args()

    if sys.version_info < (3, 10):
        fail("Python 3.10 or newer is required.")
    plat = {"Darwin": "macOS", "Windows": "Windows", "Linux": "Linux"}.get(platform.system(), platform.system())
    if plat == "Linux":
        say("On Linux install KDE Frameworks 6 from your distribution (installing Plasma pulls it in).\n"
            "This tool only covers Windows and macOS.")
        sys.exit(0)
    if plat not in {"macOS", "Windows"}:
        fail(f"unsupported platform: {plat}")

    load_kf6()
    if plat == "Windows":
        enter_vs_environment()
    qt_prefix = find_qt(args.qt_prefix, plat)

    # One directory per KDE Frameworks + Qt combination: a prefix must not mix builds against different Qts.
    qt_version = kf.detect_qt(_probe_cfg(qt_prefix))["version"]
    kf_version = args.kf_version or "6.31.0"
    root = Path(args.root).expanduser() if args.root else default_root(plat)
    base = (root / f"{kf_version}-qt{qt_version}-{args.build_type}").resolve()
    prefix, work = base / "prefix", base / "work"
    work.mkdir(parents=True, exist_ok=True)

    # kf6.py talks to GitHub Actions through files; point them at scratch files in our work directory.
    gh = {k: work / f"gh-{k.lower()}.txt" for k in ("GITHUB_OUTPUT", "GITHUB_ENV", "GITHUB_PATH", "GITHUB_STEP_SUMMARY")}
    for k, p in gh.items():
        p.write_text("")
        os.environ[k] = str(p)

    inputs = {
        "version": args.kf_version, "frameworks": args.frameworks, "exclude": args.exclude,
        "qt-prefix": qt_prefix, "build-type": args.build_type, "linkage": "shared",
        "install-prefix": str(prefix), "work-dir": str(work),
        "keep-going": str(args.keep_going).lower(), "verbose": str(args.verbose).lower(),
        "jobs": str(args.jobs), "cache": "true", "system-deps": "false", "export-env": "true",
    }
    os.environ["KF6_INPUTS"] = json.dumps(inputs)
    cfg = kf.Config()

    # ---- plan (twice on Windows if kdoctools is involved: its DocBook arguments are part of its fingerprint)
    kf.cmd_plan(cfg)
    plan = kf.load_plan(cfg)
    names = [f["name"] for f in plan["frameworks"]]
    if plat == "Windows" and "kdoctools" in names and not args.plan_only:
        ensure_docbook(base / "docbook")
        kf.cmd_plan(cfg)
        plan = kf.load_plan(cfg)

    tiers = sorted({f["group"] for f in plan["frameworks"]})
    stamp_file = prefix / ".kf6-setup.json"
    stamp = {"key": plan["keys"][str(tiers[-1])], "frameworks": names}
    up_to_date = (not args.force and stamp_file.exists()
                  and json.loads(stamp_file.read_text()) == stamp
                  and (work / "sysdeps.json").exists())

    # ---- prerequisites
    if up_to_date and not args.plan_only:
        say(f"\nKDE Frameworks {plan['version']} is already built in {prefix}.")
    else:
        actions, record_state = (collect_macos(names) if plat == "macOS"
                                 else collect_windows(names, base, work, args.vcpkg_root))
        if actions:
            say("\nMissing prerequisites, these would be run:")
            for desc, _ in actions:
                say(f"  - {desc}")
        else:
            say("\nAll native prerequisites are present.")
        if args.plan_only:
            say(f"\nPlan only: nothing was built or installed. Everything would live in {base}")
            return
        if actions:
            if not confirm("Install them now?", args.yes):
                fail("Prerequisites are missing. Re-run with --yes to install them, or install them yourself.")
            for _, fn in actions:
                fn()
        (work / "sysdeps.json").write_text(json.dumps(record_state()))

        # ---- build, tier by tier (each tier is resumable)
        ok = True
        for tier in tiers:
            try:
                kf.cmd_build(cfg, tier)
            except SystemExit:
                ok = False
                break
        if not ok:
            report(cfg, plan)
            fail(f"The build failed. Logs: {work / 'logs'}. Fix the cause (or use --keep-going) and run the tool "
                 "again: it continues with what is still missing.")

    if up_to_date:  # nothing was built: tell finish() that everything is in place
        kf.save_status(cfg, {n: {"state": "cached", "seconds": 0} for n in names})

    # ---- finish: let kf6.py compute the environment, then turn it into a script
    for var in LIST_VARS:  # keep the generated script independent of this shell's current values
        os.environ.pop(var, None)
    for p in gh.values():
        p.write_text("")
    try:
        kf.cmd_finish(cfg)
    except SystemExit:
        report(cfg, plan)
        fail("Some frameworks failed or were skipped, see above. Run the tool again after fixing the cause.")
    report(cfg, plan)

    exported = dict(l.split("=", 1) for l in read_lines(gh["GITHUB_ENV"]) if "=" in l)
    sets = {"KF6_PREFIX": exported.pop("KF6_PREFIX", str(prefix))}
    prepends = {}
    for var in LIST_VARS:
        if var in exported:
            items = [i for i in exported[var].split(os.pathsep) if i and i not in XDG_DEFAULT]
            if var == "CMAKE_PREFIX_PATH":
                items.append(qt_prefix)
            prepends[var] = items
    sysdeps = kf.load_sysdeps(cfg)
    path_items = read_lines(gh["GITHUB_PATH"])
    if plat == "Windows":  # the DLLs of the dependencies and of Qt are only found through PATH
        path_items += sysdeps["path"] + [str(Path(qt_prefix) / "bin")]
    prepends["PATH"] = list(dict.fromkeys(path_items))

    script = write_env_script(base, plat, sets, prepends)
    stamp_file.write_text(json.dumps(stamp))
    say(f"\nKDE Frameworks {plan['version']} is ready: {prefix}")
    say("Activate it in the shell you configure MeshMC from:")
    say(f"  . {shlex.quote(str(script)) if plat == 'macOS' else chr(34) + str(script) + chr(34)}")


def report(cfg, plan: dict) -> None:
    status = kf.load_status(cfg)
    groups: dict[str, list[str]] = {}
    for f in plan["frameworks"]:
        st = status.get(f["name"], {"state": "not run"})
        note = f" ({st['reason']})" if st.get("reason") else ""
        groups.setdefault(st["state"], []).append(f["name"] + note)
    say("\nResult:")
    for state in ("built", "cached", "failed", "skipped", "not run"):
        if groups.get(state):
            say(f"  {state:8} {len(groups[state]):3}: " + ", ".join(groups[state]))


def _probe_cfg(qt_prefix: str):
    """A throw-away Config, only to reuse kf6.detect_qt for the Qt version."""
    saved = os.environ.get("KF6_INPUTS")
    os.environ["KF6_INPUTS"] = json.dumps({"qt-prefix": qt_prefix})
    try:
        return kf.Config()
    finally:
        if saved is None:
            os.environ.pop("KF6_INPUTS", None)
        else:
            os.environ["KF6_INPUTS"] = saved


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        fail("interrupted; run the tool again to continue.", 130)
