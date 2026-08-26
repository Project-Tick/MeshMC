@echo off
rem
rem Windows counterpart of update.sh: regenerates locale\template.pot from the
rem sources in this repository. Keep the two in sync -- same directory list,
rem same exclusions, same (ordinal) sort order, so both produce byte identical
rem output.
rem
rem Requires lupdate.exe and lconvert.exe from Qt (qttools). Either put them on
rem PATH (a Qt command prompt does that), or point QT_BIN_DIR at the Qt bin
rem directory, or set LUPDATE_BIN / LCONVERT_BIN to the full paths.
rem
rem   set QT_BIN_DIR=C:\Qt\6.11.2\msvc2022_64\bin
rem   locale\update.bat
rem
setlocal EnableExtensions

set "ROOT=%~dp0"
if "%ROOT:~-1%"=="\" set "ROOT=%ROOT:~0,-1%"
for %%I in ("%ROOT%\..") do set "REPO=%%~fI"

rem Directories scanned for translatable strings, relative to the repository
rem root. Override by setting SRC_DIRS before calling this script.
if not defined SRC_DIRS set "SRC_DIRS=launcher plugins crashreporter updater"

rem plugins/staging/ is gated behind the MeshMC_STAGING_PLUGINS option (off by
rem default), so those strings are not shipped and must not reach translators.
if not defined EXCLUDE_RE set "EXCLUDE_RE=^plugins/staging/"

set "TEMPLATE_PO=%ROOT%\template.pot"
set "BASE_LST_FILE=%ROOT%\base_lst_file"

rem lupdate writes source locations relative to the .ts file and lconvert copies
rem them into the .pot verbatim, so the intermediate .ts has to sit at the
rem repository root for the locations to read "launcher/Foo.cpp:12".
set "TEMPLATE_TS=%REPO%\template.ts"

if not defined LUPDATE_BIN set "LUPDATE_BIN=lupdate"
if not defined LCONVERT_BIN set "LCONVERT_BIN=lconvert"
if defined QT_BIN_DIR (
    if /i "%LUPDATE_BIN%"=="lupdate" set "LUPDATE_BIN=%QT_BIN_DIR%\lupdate.exe"
    if /i "%LCONVERT_BIN%"=="lconvert" set "LCONVERT_BIN=%QT_BIN_DIR%\lconvert.exe"
)

call :require_tool "%LUPDATE_BIN%"
if errorlevel 1 goto :fail
call :require_tool "%LCONVERT_BIN%"
if errorlevel 1 goto :fail

cd /d "%REPO%" || goto :fail

echo Writing lst file...
set "MMC_REPO=%REPO%"
set "MMC_SRC_DIRS=%SRC_DIRS%"
set "MMC_EXCLUDE_RE=%EXCLUDE_RE%"
set "MMC_LST=%BASE_LST_FILE%"
powershell -NoProfile -ExecutionPolicy Bypass -Command "& { $ErrorActionPreference = 'Stop'; $repo = $env:MMC_REPO; $dirs = $env:MMC_SRC_DIRS.Split(' ', [StringSplitOptions]::RemoveEmptyEntries); $ex = $env:MMC_EXCLUDE_RE; $list = New-Object System.Collections.Generic.List[string]; foreach ($d in $dirs) { $p = Join-Path $repo $d; if (-not (Test-Path -LiteralPath $p)) { throw ('source directory not found: ' + $d) }; Get-ChildItem -LiteralPath $p -Recurse -File | ForEach-Object { $e = $_.Extension.ToLowerInvariant(); if ($e -eq '.h' -or $e -eq '.cpp' -or $e -eq '.ui') { $r = $_.FullName.Substring($repo.Length + 1).Replace('\', '/'); if ((-not $ex) -or ($r -notmatch $ex)) { $list.Add($r) } } } }; $arr = $list.ToArray(); [Array]::Sort($arr, [System.StringComparer]::Ordinal); $nl = [string][char]10; [IO.File]::WriteAllText($env:MMC_LST, ($arr -join $nl) + $nl, (New-Object System.Text.UTF8Encoding($false))); Write-Host ('    ' + $arr.Length + ' files found') }"
if errorlevel 1 goto :fail

echo Generating new template...
echo     Generating .ts
if exist "%TEMPLATE_TS%" del /q "%TEMPLATE_TS%"
"%LUPDATE_BIN%" "@%BASE_LST_FILE%" -ts "%TEMPLATE_TS%"
if errorlevel 1 goto :fail

echo     Converting .ts to .pot
"%LCONVERT_BIN%" "%TEMPLATE_TS%" -o "%TEMPLATE_PO%"
if errorlevel 1 goto :fail

if exist "%TEMPLATE_TS%" del /q "%TEMPLATE_TS%"
echo All done!
endlocal
exit /b 0

:require_tool
rem A full path is checked directly -- "where" rejects those with an error.
if exist "%~1" exit /b 0
where /q "%~1" 2>nul && exit /b 0
echo ERROR: %~1 not found. Put the Qt tools on PATH, or set QT_BIN_DIR^, 1>&2
echo        LUPDATE_BIN and LCONVERT_BIN. 1>&2
exit /b 1

:fail
if exist "%TEMPLATE_TS%" del /q "%TEMPLATE_TS%"
echo FAILED 1>&2
endlocal
exit /b 1
