
#!/usr/bin/env python3

from __future__ import annotations

import os
import pathlib
import subprocess
import sys
import tarfile
import urllib.request


QT_MAJOR = os.environ["QT_MAJOR"]
QT_VERSION = os.environ["QT_VERSION"]
BUILD_TYPE = os.environ.get("BUILD_TYPE", "Release")
PREFIX = pathlib.Path(os.environ["KDE_PREFIX"]).resolve()
CACHE_HIT = os.environ.get("KF_CACHE_HIT", "").lower() == "true"

if QT_MAJOR == "5":
    KF_VERSION = "5.116.0"
    KF_SERIES = "5.116"
    MODULES = [
        "extra-cmake-modules",
        "kcoreaddons",
        "kcodecs",
        "kguiaddons",
        "ki18n",
        "kconfig",
        "kwidgetsaddons",
        "kitemviews",
        "kwindowsystem",
        "kdbusaddons",
        "kcrash",
        "kauth",
        "karchive",
        "sonnet",
        "kcompletion",
        "kdoctools",
        "kconfigwidgets",
        "kglobalaccel",
        "kiconthemes",
        "ktextwidgets",
        "kxmlgui",
    ]
elif QT_MAJOR == "6":
    KF_VERSION = "6.31.0"
    KF_SERIES = "6.31"
    MODULES = [
        "extra-cmake-modules",
        "kcoreaddons",
        "kcodecs",
        "kguiaddons",
        "ki18n",
        "kconfig",
        "kwidgetsaddons",
        "kitemviews",
        "kwindowsystem",
        "kdbusaddons",
        "kcrash",
        "kauth",
        "karchive",
        "sonnet",
        "kcompletion",
        "kcolorscheme",
        "kdoctools",
        "kconfigwidgets",
        "kglobalaccel",
        "breeze-icons",
        "kiconthemes",
        "ktextwidgets",
        "kxmlgui",
    ]
else:
    raise SystemExit(f"Unsupported Qt major version: {QT_MAJOR}")


def run(args: list[str], *, cwd: pathlib.Path | None = None) -> None:
    print("+", subprocess.list2cmdline(args), flush=True)
    subprocess.run(args, cwd=cwd, check=True)


def append_github_env(name: str, value: str) -> None:
    env_file = os.environ.get("GITHUB_ENV")
    if env_file:
        with open(env_file, "a", encoding="utf-8") as file:
            file.write(f"{name}={value}\n")


def append_github_path(value: pathlib.Path) -> None:
    path_file = os.environ.get("GITHUB_PATH")
    if path_file:
        with open(path_file, "a", encoding="utf-8") as file:
            file.write(str(value) + "\n")


def find_prefixes() -> list[str]:
    result = [str(PREFIX)]

    qt_root = os.environ.get("QT_ROOT_DIR")
    if qt_root:
        result.append(qt_root)

    # aqt's Qt root is explicit on native builds. MSYS2 may instead
    # provide Qt through its own CMAKE_PREFIX_PATH/PATH.
    existing = os.environ.get("CMAKE_PREFIX_PATH", "")
    for entry in existing.split(os.pathsep):
        if entry and entry not in result:
            result.append(entry)

    if sys.platform == "darwin":
        brew = shutil_which("brew")
        if brew:
            result.append(
                subprocess.check_output(
                    [brew, "--prefix", "gettext"],
                    text=True,
                ).strip()
            )

    if os.name == "nt" and not os.environ.get("MSYSTEM"):
        vcpkg_root = (
            os.environ.get("VCPKG_INSTALLATION_ROOT")
            or os.environ.get("VCPKG_ROOT")
        )
        if vcpkg_root:
            triplet = (
                "arm64-windows"
                if os.environ.get("RUNNER_ARCH") == "ARM64"
                else "x64-windows"
            )
            result.append(str(pathlib.Path(vcpkg_root) / "installed" / triplet))

    # Preserve order while removing duplicates.
    return list(dict.fromkeys(result))


def shutil_which(program: str) -> str | None:
    import shutil
    return shutil.which(program)


def download_source(module: str, source_dir: pathlib.Path) -> pathlib.Path:
    archive_name = f"{module}-{KF_VERSION}.tar.xz"
    url = (
        "https://download.kde.org/stable/frameworks/"
        f"{KF_SERIES}/{archive_name}"
    )
    archive_path = source_dir / archive_name
    extracted_path = source_dir / f"{module}-{KF_VERSION}"

    if extracted_path.is_dir():
        return extracted_path

    print(f"Downloading {url}", flush=True)
    request = urllib.request.Request(
        url,
        headers={"User-Agent": "ProjectTick-CI/1.0"},
    )
    with urllib.request.urlopen(request, timeout=120) as response:
        archive_path.write_bytes(response.read())

    with tarfile.open(archive_path, "r:xz") as archive:
        if os.name == "nt" and hasattr(tarfile, "fully_trusted_filter"):
            # Work around MSYS2 Python's Windows path handling in
            # Python 3.14's default tarfile data filter.
            archive.extractall(source_dir, filter="fully_trusted")
        else:
            archive.extractall(source_dir)

    if not extracted_path.is_dir():
        raise RuntimeError(
            f"Archive did not contain expected directory: {extracted_path}"
        )

    return extracted_path


def build_module(
    module: str,
    source_dir: pathlib.Path,
    build_root: pathlib.Path,
    prefixes: list[str],
) -> None:
    source = download_source(module, source_dir)
    build = build_root / module
    build.mkdir(parents=True, exist_ok=True)

    prefix_list = ";".join(prefixes)
    args = [
        "cmake",
        "-S", str(source),
        "-B", str(build),
        "-G", "Ninja",
        f"-DCMAKE_INSTALL_PREFIX={PREFIX}",
        f"-DCMAKE_PREFIX_PATH={prefix_list}",
        f"-DCMAKE_BUILD_TYPE={BUILD_TYPE}",
        "-DBUILD_TESTING=OFF",
        "-DBUILD_QCH=OFF",
        "-DBUILD_WITH_QML=OFF",
        "-DBUILD_WITH_QTSCRIPT=OFF",
    ]

    if QT_MAJOR == "6" and module in {"kcoreaddons", "kguiaddons", "kwidgetsaddons"}:
        args.append("-DBUILD_PYTHON_BINDINGS=OFF")

    if QT_MAJOR == "6" and module == "kcoreaddons":
        args.append("-DKCOREADDONS_USE_QML=OFF")

    if module == "kconfig":
        args.append("-DKCONFIG_USE_QML=OFF")

    if module == "kguiaddons" and os.environ.get("RUNNER_OS") == "Linux":
        args.append("-DWITH_WAYLAND=OFF")

    # GNU gperf generates legacy 'register' declarations in KCodecs 6.31.0.
    # Clang rejects them in C++17 mode unless this diagnostic is disabled.
    if sys.platform == "darwin" and module == "kcodecs":
        args.append("-DCMAKE_CXX_FLAGS=-Wno-register")

    # sccache/MSVC can drop /FS while using shared PDB files.
    # Embedded debug information (/Z7) avoids that shared-PDB conflict.
    if os.name == "nt" and not os.environ.get("MSYSTEM"):
        args.extend([
            "-DCMAKE_POLICY_DEFAULT_CMP0141=NEW",
            "-DCMAKE_MSVC_DEBUG_INFORMATION_FORMAT=Embedded",
        ])

    # Keep the framework build tied to the same major Qt version.
    args.append(f"-DQT_MAJOR_VERSION={QT_MAJOR}")

    run(args)
    run(["cmake", "--build", str(build), "--parallel"])
    run(["cmake", "--install", str(build)])


def export_environment(prefixes: list[str]) -> None:
    path_entries = [str(PREFIX / "bin")]

    if os.name == "nt":
        path_entries.extend(
            [
                str(PREFIX / "lib"),
                str(PREFIX / "bin"),
            ]
        )
    else:
        lib_dirs = [PREFIX / "lib", PREFIX / "lib64"]
        existing = os.environ.get(
            "DYLD_LIBRARY_PATH" if sys.platform == "darwin"
            else "LD_LIBRARY_PATH",
            "",
        )
        lib_paths = [str(path) for path in lib_dirs if path.is_dir()]
        if existing:
            lib_paths.extend(existing.split(os.pathsep))

        variable = (
            "DYLD_LIBRARY_PATH"
            if sys.platform == "darwin"
            else "LD_LIBRARY_PATH"
        )
        append_github_env(variable, os.pathsep.join(lib_paths))

    for entry in path_entries:
        if pathlib.Path(entry).is_dir():
            append_github_path(pathlib.Path(entry))

    cmake_prefixes = [str(PREFIX)]
    qt_root = os.environ.get("QT_ROOT_DIR")
    if qt_root:
        cmake_prefixes.append(qt_root)

    existing = os.environ.get("CMAKE_PREFIX_PATH", "")
    if existing:
        cmake_prefixes.extend(
            entry for entry in existing.split(os.pathsep) if entry
        )

    append_github_env(
        "CMAKE_PREFIX_PATH",
        os.pathsep.join(dict.fromkeys(cmake_prefixes)),
    )


def main() -> None:
    PREFIX.mkdir(parents=True, exist_ok=True)

    prefixes = find_prefixes()

    if not CACHE_HIT:
        work_root = pathlib.Path(
            os.environ.get("RUNNER_TEMP", pathlib.Path.cwd())
        ) / "meshmc-kf-build"
        source_dir = work_root / "sources"
        build_root = work_root / "build"
        source_dir.mkdir(parents=True, exist_ok=True)
        build_root.mkdir(parents=True, exist_ok=True)

        modules = list(MODULES)

        # KGlobalAccel officially supports Linux and FreeBSD only.
        # KXmlGui does not require it on Windows or macOS.
        if QT_MAJOR == "6" and os.environ.get("RUNNER_OS") in {"Windows", "macOS"}:
            modules = [
                module for module in modules
                if module not in {"kglobalaccel", "kauth"}
            ]
        
        if QT_MAJOR == "5" and os.environ.get("RUNNER_OS") in {"Windows"}:
            modules = [
                module for module in modules
                if module not in {"kglobalaccel", "kauth"}
            ]

        if QT_MAJOR == "5" and os.environ.get("RUNNER_OS") in {"macOS"}:
            modules = [
                module for module in modules
                if module not in {"kglobalaccel"}
            ]

        for module in modules:
            print(
                f"::group::Building {module} {KF_VERSION}",
                flush=True,
            )
            try:
                build_module(module, source_dir, build_root, prefixes)
            finally:
                print("::endgroup::", flush=True)

            # Later modules must discover earlier installed modules.
            prefixes = find_prefixes()

    export_environment(prefixes)
    print(f"KDE Frameworks installed at: {PREFIX}", flush=True)


if __name__ == "__main__":
    try:
        main()
    except subprocess.CalledProcessError as error:
        print(
            f"::error::KDE Frameworks build failed with exit code "
            f"{error.returncode}",
            flush=True,
        )
        raise
    except Exception as error:
        print(f"::error::{error}", flush=True)
        raise
