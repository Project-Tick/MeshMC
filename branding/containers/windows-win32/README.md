# windows-win32: win32 toolchain (Wine)

Image `meshmc/toolchains/windows-win32:dev`. Builds the `win32` Launcher on Linux under
Wine with the exact toolchain of the old Win7 build VM (snapshot `2026-10-21-win7-vm`),
replacing the VM.

    docker build -t meshmc/toolchains/windows-win32:dev .

The build steps are hopper's `build/win32.sh`.

## Pieces

| Piece | What / why |
|-------|------------|
| `Dockerfile` | Minimal Arch pinned by digest, packages from the Arch Linux Archive snapshot 2026/10/20 (only `lib32-glibc` and `lib32-gcc-libs` added, for GE-Proton's 32-bit side), GE-Proton10-15, the `Tools/` subset (see `.dockerignore`), a ready-made Wine prefix (with the `libvkd3d` DLLs copied in by hand, as the prefix isn't made through Proton's launcher script). Sets `WINE`, `TOOLS`, `PREFIX_TEMPLATE` for the build steps. |
| `GE-Proton10-15/` | The Wine, run as `files/bin/wine` (the i386 launcher, classic 32/64-bit mode). Known good from the original Wine prototype; copied in as-is. Not in git. |
| `Tools/` | The VM's toolchain, hash-verified against the VM disk (see the snapshot). Not in git. The image takes `Qt/5.6.3/mingw49_32` + `Qt/Tools/mingw492_32` (Qt 5.6.3, MinGW 4.9.2), `CMake-3.20.1` (the CMake the VM built with, extracted from the VM disk), `GnuWin32` (zlib), `OpenSSL-Win32` (OpenSSL 1.0 DLLs). The rest (`QtX`: Qt 5.15.2, which the `windows-win32-qt5.15` image takes, and 6.5.3, `CMake`: 4.1.2; and, as they can't be redistributed, `Tools/`: the VM's MSVC `dumpbin`/`link`, and `jdk1.8.0_112`: Oracle's JDK, under the Binary Code License) isn't used. |
| `dumpbin/` | Our own `dumpbin /dependents` for the bundling, built in the image with its MinGW into `Tools/Tools/dumpbin.exe` (see [its README](dumpbin/README.md)). Replaces the VM's MSVC dumpbin (2026-10-30). |
| `tarballs/` | `OpenJDK8U-jdk_x64_windows_hotspot_8u504b01.zip`: Temurin JDK 8, the same release as the macOS image, for the Launcher's jars (`javac -target 7`). From Adoptium; SHA-256 matches the Adoptium API and the release's `.sha256.txt`, PGP signature good (Adoptium key `3B04D753C9050D9A5D343F39843C48A565F8F04B`), checked 2026-10-30; the Dockerfile checks the SHA-256 again. Installed as `Tools/jdk8`, its Windows path in `JDK` for the build steps. Not in git (`SHA256SUMS` is). Replaces the VM's Oracle JDK 8u112 (2026-10-30). |

## Notes

- Debian's Wine 8.0 was tried first and hangs CMake's Java check (`java -version` makes Wine
  start `explorer.exe /desktop`, which inherits CMake's output pipe and never exits).
  GE-Proton10-15 doesn't. Don't swap the Wine without re-verifying.
- `win32-5.15.2` (Qt 5.15.2, MinGW 8.1) is built again since 2026-11-05, with the
  [`windows-win32-qt5.15`](../windows-win32-qt5.15/README.md) image (this one plus `Tools/QtX`'s Qt 5.15.2).
