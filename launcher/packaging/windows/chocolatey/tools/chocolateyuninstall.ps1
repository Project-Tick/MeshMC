$ErrorActionPreference = 'Stop'; # stop on all errors
$packageArgs = @{
  packageName   = $env:ChocolateyPackageName
  softwareName  = 'MeshMC*'
}

Write-Host "Removing Start Menu and Desktop shortcuts"
$StartMenuShortcut = Join-Path $env:programdata "Microsoft\Windows\Start Menu\Programs\MeshMC.lnk"
$DesktopShortcut = Join-Path $([Environment]::GetFolderPath("Desktop")) "MeshMC.lnk"
if (Test-Path $StartMenuShortcut) {
  Remove-Item $StartMenuShortcut
}
if (Test-Path $DesktopShortcut) {
  Remove-Item $DesktopShortcut
}

Write-Host "Removing MeshMC"
$installDir = Join-Path "$(Get-ToolsLocation)" "MeshMC"
if (Test-Path $installDir) {
  Remove-Item -Recurse $installDir
}
