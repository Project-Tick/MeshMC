# dumpbin: our own `dumpbin /dependents`

The win32 build bundles its DLLs with CMake's `fixup_bundle`, which finds each binary's
dependencies with `CMAKE_GP_TOOL=dumpbin` (hopper's `launcher/steps/win32.sh`). The old VM
had Microsoft's `dumpbin.exe`/`link.exe`/`mspdb80.dll` (VS 2008) in `Tools/Tools` for this,
which can't be redistributed. `dumpbin.c` replaces them, so the image can be published.

It does only `dumpbin /dependents FILE...`: the imported DLLs, then the delay-loaded ones,
laid out like Microsoft's (same headings, four-space indent, CRLF), without the banner and
section summary that CMake doesn't read. Any other option is refused. The image builds it
with its own MinGW 4.9.2 (`Dockerfile`) into `Tools/Tools/dumpbin.exe`, where the build
already looks for it.

Why not MinGW's `objdump`: CMake's `GetPrerequisites` can use it, but `objdump -p` prints
the whole export table too, and CMake parsing that line by line for every Qt DLL makes the
install step very slow. `dumpbin /dependents` output is a few lines.
