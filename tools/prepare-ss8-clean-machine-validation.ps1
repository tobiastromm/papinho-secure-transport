# SPDX-License-Identifier: MPL-2.0
param([string]$Destination)
$ErrorActionPreference="Stop"
$repo=Split-Path -Parent $PSScriptRoot
if(-not$Destination){$Destination=Join-Path $repo "build\ss8-clean-machine-validation"}
$destination=[IO.Path]::GetFullPath($Destination)
if(Test-Path -LiteralPath $destination){Remove-Item -LiteralPath $destination -Recurse -Force}
New-Item -ItemType Directory -Path $destination|Out-Null
$packages=Join-Path $repo "dist\packages\0.5.0"
foreach($file in Get-ChildItem -LiteralPath $packages -File){Copy-Item -LiteralPath $file.FullName -Destination $destination}
Copy-Item -LiteralPath (Join-Path $repo "tools\run-ss8-clean-machine-validation.ps1") -Destination $destination
Copy-Item -LiteralPath (Join-Path $repo "tools\run-ss8-combined-real-tls.ps1") -Destination $destination
$readme=@'
PapinhoSecureTransport SS-8 clean-machine validation bundle

This directory must be copied to a separate Windows 10/11 x64 machine. It is
not sufficient to run it on the development host.

Prerequisites: PowerShell 5.1 and a supported modern MSVC x64/Windows SDK build
environment. VC6 is optional and is detected explicitly; do not compile the x86
VC6/NSS SDK with modern MSVC. Do not install or add global OpenSSL/NSS paths.

From an ordinary PowerShell prompt in this directory:

  powershell -NoProfile -ExecutionPolicy Bypass -File .\run-ss8-clean-machine-validation.ps1 -BundleDirectory . | Tee-Object clean-machine.log

After that command passes, run the package-only Combined real TLS matrix:

  powershell -NoProfile -ExecutionPolicy Bypass -File .\run-ss8-combined-real-tls.ps1 -BundleDirectory . | Tee-Object combined-real-tls.log

Return clean-machine.log, OS facts, compiler version output, real TLS fixture
outputs, Root/CA before/after residue counts, and loaded-module paths. The
runner verifies every transferred byte before extraction and performs the
structural plus CLIENT/SERVER compile/link/bootstrap gates. Real TLS/trust
gates remain explicit and must not be inferred from that bootstrap.
'@
[IO.File]::WriteAllText((Join-Path $destination "README.txt"),$readme,(New-Object Text.UTF8Encoding($false)))
$lines=Get-ChildItem -LiteralPath $destination -File|Where-Object Name -ne "TRANSFER-SHA256SUMS.txt"|Sort-Object Name|ForEach-Object{"{0}  {1}"-f(Get-FileHash -Algorithm SHA256 -LiteralPath $_.FullName).Hash.ToLowerInvariant(),$_.Name}
[IO.File]::WriteAllText((Join-Path $destination "TRANSFER-SHA256SUMS.txt"),(($lines-join"`n")+"`n"),(New-Object Text.UTF8Encoding($false)))
Write-Output ("SS8_CLEAN_MACHINE_BUNDLE="+$destination)
Write-Output ("TRANSFER_FILE_COUNT="+($lines.Count+1))
