$ErrorActionPreference = 'Stop'; # stop on all errors
$pp         = Get-PackageParameters
$toolsDir   = "$(Split-Path -parent $MyInvocation.MyCommand.Definition)"
$installDir = Join-Path "$(Get-ToolsLocation)" "MeshMC"
$url      = 'https://github.com/Project-Tick/MeshMC/releases/download/v10.0.0/MeshMC-Windows-MSVC-v10.0.0.zip'
$checksum = '4c4efe696a1d383f3017c9d6beb3e7561530431a99b724a0c5a300e262345fc9'

$packageArgs = @{
  packageName   = $env:ChocolateyPackageName
  unzipLocation = $installDir
  url           = $url
  softwareName  = 'MeshMC*'
  checksum      = $checksum
  checksumType  = 'sha256'
}

Install-ChocolateyZipPackage @packageArgs

Write-Host "Removing Updater"
Remove-Item $(Join-Path $installdir "meshmc-updater.exe")

Write-Host "Creating Start Menu and Desktop shortcuts"
$startmenu = Join-Path $env:programdata "Microsoft\Windows\Start Menu\Programs"
Install-ChocolateyShortcut -shortcutFilePath $(Join-Path $startmenu "MeshMC.lnk") -TargetPath $(Join-Path $installDir "meshmc.exe")
$desktop = [Environment]::GetFolderPath("Desktop")
Install-ChocolateyShortcut -shortcutFilePath $(Join-Path $desktop "MeshMC.lnk") -TargetPath $(Join-Path $installDir "meshmc.exe")
