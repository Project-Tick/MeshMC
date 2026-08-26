# SPDX-FileCopyrightText: 2026 Project Tick
# SPDX-License-Identifier: GPL-3.0-or-later
#
# build-deps.ps1 — Build and install all MeshMC monorepo dependencies, then
#                   configure MeshMC itself.  (Windows — PowerShell)
#
# Usage:
#   .\scripts\build-deps.ps1 [-Configure] [-Build] [-Clean] [-Jobs N]
#
# Parameters:
#   -Configure    Also configure MeshMC after installing dependencies
#   -Build        Also build MeshMC (implies -Configure)
#   -Clean        Remove existing build directories before building
#   -Jobs N       Parallel build jobs (default: number of processors)
#
# Environment variables:
#   MONOREPO_ROOT     Override monorepo root (default: auto-detected)
#   INSTALL_PREFIX    Override install prefix
#   BUILD_TYPE        Override build type (default: Release)
#   CMAKE_GENERATOR   Override generator (default: Ninja)

[CmdletBinding()]
param(
    [switch]$Configure,
    [switch]$Build,
    [switch]$Clean,
    [int]$Jobs = 0
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

##############################################################################
# Helpers
##############################################################################

function Write-Log   { param([string]$Msg) Write-Host "[build-deps] $Msg" -ForegroundColor Green }
function Write-Warn  { param([string]$Msg) Write-Warning "[build-deps] $Msg" }
function Write-Err   { param([string]$Msg) Write-Host "[build-deps] $Msg" -ForegroundColor Red; exit 1 }

function Invoke-Cmd {
    $cmdLine = $args -join ' '
    Write-Host "  > $cmdLine" -ForegroundColor DarkGray
    & $args[0] $args[1..($args.Length - 1)]
    if ($LASTEXITCODE -ne 0) {
        Write-Err "Command failed (exit $LASTEXITCODE): $cmdLine"
    }
}

##############################################################################
# Defaults
##############################################################################

$ScriptDir    = Split-Path -Parent $MyInvocation.MyCommand.Definition
$MonorepoRoot = if ($env:MONOREPO_ROOT) { $env:MONOREPO_ROOT } else { (Resolve-Path "$ScriptDir\..\..").Path }
$MeshMCDir    = Join-Path $MonorepoRoot 'meshmc'
$BuildType    = if ($env:BUILD_TYPE)    { $env:BUILD_TYPE }       else { 'Release' }
$Generator    = if ($env:CMAKE_GENERATOR) { $env:CMAKE_GENERATOR } else { 'Ninja' }

if ($Jobs -le 0) {
    $Jobs = $env:NUMBER_OF_PROCESSORS
    if (-not $Jobs) { $Jobs = 4 }
}

if ($Build) { $Configure = $true }

##############################################################################
# Platform detection — MSVC vs MinGW
##############################################################################

# If MINGW_PREFIX is set we're inside MSYS2 — but that means the user should
# use build-deps.sh instead, so this script assumes MSVC.

$Platform = 'windows-msvc'
Write-Log "Platform: $Platform"

##############################################################################
# CMake flags
##############################################################################

$InstallPrefix = if ($env:INSTALL_PREFIX) {
    $env:INSTALL_PREFIX
} else {
    Join-Path $MonorepoRoot 'meshmc-deps'
}

$CMakeCommon = @(
    "-DCMAKE_INSTALL_PREFIX=$InstallPrefix"
    "-DCMAKE_BUILD_TYPE=$BuildType"
)

Write-Log "Install prefix: $InstallPrefix"
Write-Log "Build type:     $BuildType"
Write-Log "Generator:      $Generator"
Write-Log "Jobs:           $Jobs"

##############################################################################
# Build a single library
##############################################################################

$script:BuiltCount = 0
$TotalLibs = 15

function Install-Lib {
    param(
        [Parameter(Mandatory)][string]$Repo,
        [Parameter(ValueFromRemainingArguments)][string[]]$ExtraArgs
    )

    $script:BuiltCount++

    $Name = [System.IO.Path]::GetFileNameWithoutExtension($Repo)
    $Src  = Join-Path $MonorepoRoot $Name
    $Bld  = Join-Path $Src 'build'

    if (-not (Test-Path $Src)) {
        Write-Log "Cloning $Repo ..."
        Invoke-Cmd git clone $Repo $Src
    }
    else {
        Write-Log "Updating $Name ..."
        Invoke-Cmd git -C $Src pull
    }

    Write-Log "[$script:BuiltCount/$TotalLibs] Building $Name ..."

    if ($Clean -and (Test-Path $Bld)) {
        Remove-Item $Bld -Recurse -Force
    }

    $configArgs = @(
        '-S', $Src,
        '-B', $Bld
    ) + $CMakeCommon

    if ($ExtraArgs) {
        $configArgs += $ExtraArgs
    }

    $configArgs += @('-G', $Generator)

    Invoke-Cmd cmake @configArgs
    Invoke-Cmd cmake --build $Bld --parallel $Jobs
    Invoke-Cmd cmake --install $Bld
}

##############################################################################
# Build all dependencies (in correct order)
##############################################################################

function Build-Deps {
    Write-Log 'Building all MeshMC dependencies...'
    Write-Host ''

    # External dependencies (not in monorepo)
    Write-Log 'Installing extra-cmake-modules...'
    $EcmDir = Join-Path $env:TEMP 'ecm'
    if (-not (Test-Path $EcmDir)) {
        Invoke-Cmd git clone --depth 1 --branch v6.13.0 https://invent.kde.org/frameworks/extra-cmake-modules.git $EcmDir
    }
    Invoke-Cmd cmake -S $EcmDir -B "$EcmDir\build" "-DCMAKE_INSTALL_PREFIX=$InstallPrefix" -G $Generator
    Invoke-Cmd cmake --install "$EcmDir\build"

    # Everything that used to be built and installed here is now part of the
    # MeshMC tree and configured by the main build:
    #
    #   libraries/zlib-ng, libraries/libarchive    git submodules
    #   libraries/* (nbt++, systeminfo, ...)       git subtrees
    #   cmark, toml++                              FetchContent (top-level CMake)
    #
    # zlib-ng replaces neozip specifically so libarchive can link against a
    # standard zlib API and keep its DEFLATE support. Building libarchive from
    # a separate prefix like this used to do is exactly how it ended up
    # compiled without zlib — unable to read a single .jar — so please do not
    # reintroduce it here.
    #
    # extra-cmake-modules above is the only external dependency left.
    Write-Log 'Remaining dependencies are in-tree; nothing else to install.'
    Write-Log 'Ensure the submodules are checked out:'
    Write-Log '    git submodule update --init --recursive'

    Write-Host ''
    Write-Log 'All dependencies built and installed successfully!'
}

##############################################################################
# Configure MeshMC
##############################################################################

function Configure-MeshMC {
    Write-Log 'Configuring MeshMC...'

    $Preset = 'windows_msvc'

    # Ensure the dependency prefix is on CMAKE_PREFIX_PATH
    if ($env:CMAKE_PREFIX_PATH) {
        $env:CMAKE_PREFIX_PATH = "$InstallPrefix;$env:CMAKE_PREFIX_PATH"
    } else {
        $env:CMAKE_PREFIX_PATH = $InstallPrefix
    }

    Invoke-Cmd cmake --preset $Preset -S $MeshMCDir

    Write-Log "MeshMC configured with preset: $Preset"
}

##############################################################################
# Build MeshMC
##############################################################################

function Build-MeshMC {
    Write-Log 'Building MeshMC...'

    $Preset = 'windows_msvc'
    Invoke-Cmd cmake --build --preset $Preset --config $BuildType --parallel $Jobs

    Write-Log 'MeshMC built successfully!'
}

##############################################################################
# Main
##############################################################################

Build-Deps

if ($Configure) {
    Write-Host ''
    Configure-MeshMC
}

if ($Build) {
    Write-Host ''
    Build-MeshMC
}

Write-Host ''
Write-Log 'Done!'
