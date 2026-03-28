@echo off
rem SPDX-License-Identifier: GPL-3.0-or-later
rem SPDX-FileCopyrightText: 2026 Project Tick
rem
rem MeshMC Bootstrap Script (Windows)
rem Checks dependencies, installs via vcpkg/scoop, and sets up lefthook.

setlocal EnableDelayedExpansion

cd /d "%~dp0"

echo.
echo [INFO]  MeshMC Bootstrap
echo ---------------------------------------------
echo.

rem ── Submodules ──────────────────────────────────────────────────────────────
if not exist ".git" (
    echo [ERR]  Not a git repository. Please run this script from the project root. 1>&2
    exit /b 1
)

echo [INFO]  Setting up submodules...
git submodule update --init --recursive
if errorlevel 1 (
    echo [ERR]  Submodule initialization failed. 1>&2
    exit /b 1
)
echo [ OK ]  Submodules initialized
echo.

rem ── Check for scoop ─────────────────────────────────────────────────────────
set "HAS_SCOOP=0"
where scoop >nul 2>&1 && set "HAS_SCOOP=1"

if "%HAS_SCOOP%"=="0" (
    echo [WARN]  Scoop is not installed.
    echo [INFO]  Installing Scoop...
    powershell -NoProfile -Command "Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser -Force; Invoke-RestMethod -Uri https://get.scoop.sh | Invoke-Expression"
    if errorlevel 1 (
        echo [ERR]  Scoop installation failed. Install manually from https://scoop.sh 1>&2
        exit /b 1
    )
    rem Refresh PATH
    set "PATH=%USERPROFILE%\scoop\shims;%PATH%"
    echo [ OK ]  Scoop installed
) else (
    echo [ OK ]  Scoop is installed
)
echo.

rem ── Check for vcpkg ─────────────────────────────────────────────────────────
set "HAS_VCPKG=0"
where vcpkg >nul 2>&1 && set "HAS_VCPKG=1"

if "%HAS_VCPKG%"=="0" (
    echo [WARN]  vcpkg is not installed.
    echo [INFO]  You can install vcpkg via: scoop install vcpkg
    echo [INFO]  Or clone from https://github.com/microsoft/vcpkg
)
echo.

rem ── Dependency Checks ──────────────────────────────────────────────────────
echo [INFO]  Checking dependencies...
echo.

set "MISSING="

call :check_cmd npm npm
call :check_cmd Go go
call :check_cmd lefthook lefthook
call :check_cmd reuse reuse
call :check_cmd CMake cmake
call :check_cmd Git git

echo.

rem ── Install Missing Dependencies via Scoop ─────────────────────────────────
if not defined MISSING (
    echo [ OK ]  All command-line dependencies are already installed!
    goto :vcpkg_libs
)

echo [INFO]  Missing dependencies: %MISSING%
echo [INFO]  Installing via Scoop...
echo.

rem Add extras bucket for some packages
scoop bucket add extras >nul 2>&1
scoop bucket add main >nul 2>&1

echo !MISSING! | findstr /i "npm" >nul && (
    echo [INFO]  Installing Node.js ^(includes npm^)...
    scoop install nodejs
)

echo !MISSING! | findstr /i "Go" >nul && (
    echo [INFO]  Installing Go...
    scoop install go
)

echo !MISSING! | findstr /i "CMake" >nul && (
    echo [INFO]  Installing CMake...
    scoop install cmake
)

echo !MISSING! | findstr /i "reuse" >nul && (
    echo [INFO]  Installing reuse via pip...
    where pip >nul 2>&1 || scoop install python
    pip install reuse
)

echo !MISSING! | findstr /i "lefthook" >nul && (
    echo [INFO]  Installing lefthook...
    where go >nul 2>&1 && (
        go install github.com/evilmartians/lefthook@latest
        set "PATH=%GOPATH%\bin;%USERPROFILE%\go\bin;%PATH%"
    ) || (
        scoop install lefthook
    )
)

echo.
echo [ OK ]  Package installation complete
echo.

rem ── vcpkg Libraries ─────────────────────────────────────────────────────────
:vcpkg_libs
where vcpkg >nul 2>&1
if errorlevel 1 (
    echo [WARN]  vcpkg not found. Skipping C/C++ library installation.
    echo [WARN]  You will need to manually install: qt6, quazip, zlib, extra-cmake-modules
    goto :setup_lefthook
)

echo [INFO]  Installing C/C++ libraries via vcpkg...

set "VCPKG_TRIPLET=x64-windows"

vcpkg install qt5-base:%VCPKG_TRIPLET% >nul 2>&1 || (
    echo [INFO]  Installing Qt6...
    vcpkg install qtbase:%VCPKG_TRIPLET%
)

echo [INFO]  Installing quazip...
vcpkg install quazip:%VCPKG_TRIPLET%

echo [INFO]  Installing zlib...
vcpkg install zlib:%VCPKG_TRIPLET%

echo [INFO]  Installing extra-cmake-modules...
vcpkg install ecm:%VCPKG_TRIPLET%

echo.
echo [ OK ]  vcpkg libraries installed
echo.

rem ── Lefthook Setup ─────────────────────────────────────────────────────────
:setup_lefthook
where lefthook >nul 2>&1
if errorlevel 1 (
    echo [ERR]  lefthook is not available. Cannot set up git hooks. 1>&2
    exit /b 1
)

echo [INFO]  Setting up lefthook...
lefthook install
if errorlevel 1 (
    echo [ERR]  lefthook install failed. 1>&2
    exit /b 1
)
echo [ OK ]  Lefthook hooks installed into .git\hooks
echo.

echo ---------------------------------------------
echo [ OK ]  Bootstrap successful! You're all set.
echo.

endlocal
exit /b 0

rem ── Helper Functions ────────────────────────────────────────────────────────
:check_cmd
set "NAME=%~1"
set "CMD=%~2"
where %CMD% >nul 2>&1
if errorlevel 1 (
    echo [WARN]  %NAME% is NOT installed
    set "MISSING=!MISSING! %NAME%"
) else (
    for /f "delims=" %%i in ('where %CMD%') do (
        echo [ OK ]  %NAME% is installed ^(%%i^)
        goto :eof
    )
)
goto :eof
