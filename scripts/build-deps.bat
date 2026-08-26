@echo off
:: SPDX-FileCopyrightText: 2026 Project Tick
:: SPDX-License-Identifier: GPL-3.0-or-later
::
:: build-deps.bat — Build and install all MeshMC monorepo dependencies, then
::                   configure MeshMC itself.  (Windows — Command Prompt)
::
:: Usage:
::   scripts\build-deps.bat [--configure] [--build] [--clean] [--jobs N]
::
:: Parameters:
::   --configure    Also configure MeshMC after installing dependencies
::   --build        Also build MeshMC (implies --configure)
::   --clean        Remove existing build directories before building
::   --jobs N       Parallel build jobs (default: NUMBER_OF_PROCESSORS)
::
:: Environment variables:
::   MONOREPO_ROOT     Override monorepo root (default: auto-detected)
::   INSTALL_PREFIX    Override install prefix
::   BUILD_TYPE        Override build type (default: Release)
::   CMAKE_GENERATOR   Override generator (default: Ninja)

setlocal enableextensions enabledelayedexpansion

::############################################################################
:: Parse arguments
::############################################################################

set "_DO_CONFIGURE=0"
set "_DO_BUILD=0"
set "_DO_CLEAN=0"
set "_JOBS=0"

:parse_args
if "%~1"=="" goto args_done
if /i "%~1"=="--configure" ( set "_DO_CONFIGURE=1" & shift & goto parse_args )
if /i "%~1"=="--build"     ( set "_DO_BUILD=1"     & shift & goto parse_args )
if /i "%~1"=="--clean"     ( set "_DO_CLEAN=1"     & shift & goto parse_args )
if /i "%~1"=="--jobs"      ( set "_JOBS=%~2"       & shift & shift & goto parse_args )
echo [build-deps] Unknown argument: %~1
exit /b 1
:args_done

if "%_DO_BUILD%"=="1" set "_DO_CONFIGURE=1"

::############################################################################
:: Defaults
::############################################################################

set "_SCRIPT_DIR=%~dp0"

if not defined MONOREPO_ROOT (
    pushd "%_SCRIPT_DIR%..\.."
    set "MONOREPO_ROOT=!CD!"
    popd
)

set "_MESHMC_DIR=%MONOREPO_ROOT%\meshmc"

if not defined BUILD_TYPE      set "BUILD_TYPE=Release"
if not defined CMAKE_GENERATOR set "CMAKE_GENERATOR=Ninja"

if "%_JOBS%"=="0" (
    if defined NUMBER_OF_PROCESSORS (
        set "_JOBS=%NUMBER_OF_PROCESSORS%"
    ) else (
        set "_JOBS=4"
    )
)

if not defined INSTALL_PREFIX set "INSTALL_PREFIX=%MONOREPO_ROOT%\meshmc-deps"

echo [build-deps] Platform: windows-msvc
echo [build-deps] Install prefix: %INSTALL_PREFIX%
echo [build-deps] Build type:     %BUILD_TYPE%
echo [build-deps] Generator:      %CMAKE_GENERATOR%
echo [build-deps] Jobs:           %_JOBS%

::############################################################################
:: Main — goto ile subroutine yerine inline label'lar kullaniyoruz.
:: Bu sayede errorlevel hic bozulmuyor.
::############################################################################

goto build_deps_start

:die
echo [build-deps] ERROR: Last command failed. 1>&2
exit /b 1

::############################################################################
:: BUILD DEPS
::############################################################################
:build_deps_start
echo.
echo [build-deps] Building all MeshMC dependencies...
echo.

:: ---- extra-cmake-modules ------------------------------------------------
echo [build-deps] Installing extra-cmake-modules...
set "_ECM_DIR=%TEMP%\ecm"

if not exist "%_ECM_DIR%" (
    echo   ^> git clone ecm
    git clone --depth 1 --branch v6.13.0 https://invent.kde.org/frameworks/extra-cmake-modules.git "%_ECM_DIR%"
    if errorlevel 1 goto die
)

echo   ^> cmake configure ecm
cmake -S "%_ECM_DIR%" -B "%_ECM_DIR%\build" "-DCMAKE_INSTALL_PREFIX=%INSTALL_PREFIX%" -G "%CMAKE_GENERATOR%"
if errorlevel 1 goto die

echo   ^> cmake build ecm
cmake --build "%_ECM_DIR%\build" --parallel %_JOBS%
if errorlevel 1 goto die

echo   ^> cmake install ecm
cmake --install "%_ECM_DIR%\build"
if errorlevel 1 goto die

:: ---- in-tree dependencies ----------------------------------------------
:: Everything that used to be cloned and installed here is now part of the
:: MeshMC tree and configured by the main build:
::
::   libraries/zlib-ng, libraries/libarchive    git submodules
::   libraries/* (nbt++, systeminfo, ...)       git subtrees
::   cmark, toml++                              FetchContent (top-level CMake)
::
:: zlib-ng replaces neozip specifically so libarchive can link against a
:: standard zlib API and keep its DEFLATE support. Building libarchive from a
:: separate prefix like this used to do is exactly how it ended up compiled
:: without zlib -- unable to read a single .jar -- so please do not
:: reintroduce it here.
::
:: extra-cmake-modules above is the only external dependency left.
echo [build-deps] Remaining dependencies are in-tree; nothing else to install.
echo [build-deps] Ensure the submodules are checked out:
echo [build-deps]     git submodule update --init --recursive

echo.
echo [build-deps] All dependencies built and installed successfully!

if "%_DO_CONFIGURE%"=="1" goto do_configure
goto done

::############################################################################
:: CONFIGURE MESHMC
::############################################################################
:do_configure
echo.
echo [build-deps] Configuring MeshMC...

if defined CMAKE_PREFIX_PATH (
    set "CMAKE_PREFIX_PATH=%INSTALL_PREFIX%;%CMAKE_PREFIX_PATH%"
) else (
    set "CMAKE_PREFIX_PATH=%INSTALL_PREFIX%"
)

cmake --preset windows_msvc -S "%_MESHMC_DIR%"
if errorlevel 1 goto die

echo [build-deps] MeshMC configured with preset: windows_msvc

if "%_DO_BUILD%"=="1" goto do_build
goto done

::############################################################################
:: BUILD MESHMC
::############################################################################
:do_build
echo.
echo [build-deps] Building MeshMC...

cmake --build --preset windows_msvc --config %BUILD_TYPE% --parallel %_JOBS%
if errorlevel 1 goto die

echo [build-deps] MeshMC built successfully!
goto done

::############################################################################
:: DONE
::############################################################################
:done
echo.
echo [build-deps] Done!
endlocal
exit /b 0

::############################################################################
:: :install_lib  <repo_url>  [extra_cmake_arg ...]
::
:: Her extra arg ayri parametre olarak gelir, subroutine icinde birlestirilir.
:: Ornek: call :install_lib "https://..." "-DFOO=OFF" "-DBAR=ON"
::############################################################################
:install_lib
set "_REPO=%~1"

for %%F in ("%_REPO%") do set "_NAME=%%~nxF"

set "_SRC=%MONOREPO_ROOT%\%_NAME%"
set "_BLD=%_SRC%\build"

if not exist "%_SRC%" (
    echo [build-deps] Cloning %_NAME% ...
    git clone "%_REPO%" "%_SRC%"
    if errorlevel 1 exit /b 1
) else (
    echo [build-deps] Updating %_NAME% ...
    git -C "%_SRC%" pull
    if errorlevel 1 exit /b 1
)

echo [build-deps] Building %_NAME% ...

if "%_DO_CLEAN%"=="1" (
    if exist "%_BLD%" rmdir /s /q "%_BLD%"
)

:: Extra cmake args: shift past the repo URL, collect the rest
shift
set "_EXTRA_ARGS="
:collect_extra
if "%~1"=="" goto extra_done
set "_EXTRA_ARGS=%_EXTRA_ARGS% %~1"
shift
goto collect_extra
:extra_done

echo   ^> cmake configure %_NAME%
cmake -S "%_SRC%" -B "%_BLD%" ^
    "-DCMAKE_INSTALL_PREFIX=%INSTALL_PREFIX%" ^
    "-DCMAKE_BUILD_TYPE=%BUILD_TYPE%" ^
    %_EXTRA_ARGS% ^
    -G "%CMAKE_GENERATOR%"
if errorlevel 1 exit /b 1

echo   ^> cmake build %_NAME%
cmake --build "%_BLD%" --parallel %_JOBS%
if errorlevel 1 exit /b 1

echo   ^> cmake install %_NAME%
cmake --install "%_BLD%"
if errorlevel 1 exit /b 1

exit /b 0
