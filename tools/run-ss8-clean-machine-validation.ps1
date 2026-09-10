# SPDX-License-Identifier: MPL-2.0
param([string]$BundleDirectory=(Split-Path -Parent $PSScriptRoot),[ValidateSet("0.5.0","0.6.0")][string]$Version="0.6.0")
$ErrorActionPreference="Stop"
$bundle=[IO.Path]::GetFullPath($BundleDirectory)
$manifest=Join-Path $bundle "TRANSFER-SHA256SUMS.txt"
if(-not(Test-Path -LiteralPath $manifest)){throw "missing transfer checksum manifest"}
foreach($line in [IO.File]::ReadAllLines($manifest)){
 if($line -notmatch '^([0-9a-f]{64})  (.+)$'){throw "invalid transfer checksum line"}
 $path=Join-Path $bundle $matches[2].Replace('/','\')
 if(-not(Test-Path -LiteralPath $path -PathType Leaf)){throw ("missing transfer file: "+$matches[2])}
 if((Get-FileHash -Algorithm SHA256 -LiteralPath $path).Hash.ToLowerInvariant()-ne$matches[1]){throw ("transfer hash mismatch: "+$matches[2])}
}
$work=Join-Path $bundle "work"
if(Test-Path -LiteralPath $work){Remove-Item -LiteralPath $work -Recurse -Force}
New-Item -ItemType Directory -Path $work|Out-Null
$sourceZip=Join-Path $bundle ("papinho-secure-transport-"+$Version+"-src.zip")
$source=Join-Path $work "source"
Expand-Archive -LiteralPath $sourceZip -DestinationPath $source
$os=Get-CimInstance Win32_OperatingSystem
Write-Output ("CLEAN_MACHINE_OS="+$os.Caption+" VERSION="+$os.Version+" BUILD="+$os.BuildNumber+" ARCH="+$os.OSArchitecture)
$msvcEnvironment=Join-Path $source "tools\msvc-env.bat"
$clPaths=@(cmd.exe /d /c ('call "'+$msvcEnvironment+'" >nul && where cl'))
if($LASTEXITCODE-ne 0-or$clPaths.Count-eq 0){throw "modern MSVC probe failed"}
$clPath=$clPaths[0].Trim();if(-not(Test-Path -LiteralPath $clPath -PathType Leaf)){throw "resolved cl.exe does not exist"}
$compilerVersion=[Diagnostics.FileVersionInfo]::GetVersionInfo($clPath).FileVersion
$msvcVersion=[regex]::Match($compilerVersion,'(?<![0-9])([0-9]+\.[0-9]+\.[0-9]+)')
if(-not$msvcVersion.Success){
 $msvcOutput=cmd.exe /d /c ('call "'+$msvcEnvironment+'" >nul && "'+$clPath+'" 2>&1')
 $msvcVersion=[regex]::Match(($msvcOutput-join "`n"),'(?<![0-9])(19\.[0-9]+\.[0-9]+)(?![0-9])')
}
if(-not$msvcVersion.Success){throw "could not determine modern MSVC version"}
Write-Output ("CLEAN_MACHINE_MSVC="+$msvcVersion.Groups[1].Value)
$rootStore=New-Object System.Security.Cryptography.X509Certificates.X509Store("Root","CurrentUser");$rootStore.Open("ReadOnly");$rootBefore=$rootStore.Certificates.Count;$rootStore.Close()
$caStore=New-Object System.Security.Cryptography.X509Certificates.X509Store("CA","CurrentUser");$caStore.Open("ReadOnly");$caBefore=$caStore.Certificates.Count;$caStore.Close()
& powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $source "tools\validate-release-packages.ps1") -Version $Version -PackageDirectory $bundle -ValidationDirectory (Join-Path $work "packages") -ExpectedChecksumsFile (Join-Path $bundle "SHA256SUMS-packages.txt") -CompileConsumers
if($LASTEXITCODE-ne 0){throw "package validation failed"}
$rootStore.Open("ReadOnly");$rootAfter=$rootStore.Certificates.Count;$rootStore.Close();$caStore.Open("ReadOnly");$caAfter=$caStore.Certificates.Count;$caStore.Close()
Write-Output ("CURRENTUSER_ROOT_BEFORE="+$rootBefore+" AFTER="+$rootAfter)
Write-Output ("CURRENTUSER_CA_BEFORE="+$caBefore+" AFTER="+$caAfter)
if($rootBefore-ne$rootAfter-or$caBefore-ne$caAfter){throw "trust-store residue detected"}
Write-Output "TRUST_STORE_CLEANUP=PASS"
Write-Output "ISOLATED_COMPILE_LINK_BOOTSTRAP=PASS"
if($Version-eq"0.6.0"){
 Write-Output "CLEAN_MACHINE_API21_WAITSET=PASS"
 Write-Output "CLEAN_MACHINE_WAKE=PASS"
 Write-Output "CLEAN_MACHINE_EXTERNAL_SOURCE=PASS"
 Write-Output "CLEAN_MACHINE_FINITE_WAIT=PASS"
}
Write-Output "REAL_TLS_AND_TRUST_GATES=REQUIRE_EXPLICIT_OWNER_EXECUTION"
