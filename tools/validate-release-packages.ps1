# SPDX-License-Identifier: MPL-2.0
param([string]$PackageDirectory,[string]$ValidationDirectory,[string]$ExpectedChecksumsFile,[switch]$CompileConsumers)
$ErrorActionPreference = "Stop"
$repo = Split-Path -Parent $PSScriptRoot
if (-not $PackageDirectory) { $PackageDirectory = Join-Path $repo "dist\packages\0.4.0" }
if (-not $ValidationDirectory) { $ValidationDirectory = Join-Path $repo "dist\validation\0.4.0" }
$PackageDirectory = [IO.Path]::GetFullPath($PackageDirectory)
$ValidationDirectory = [IO.Path]::GetFullPath($ValidationDirectory)
if (-not $ExpectedChecksumsFile) { $ExpectedChecksumsFile = Join-Path $PackageDirectory "SHA256SUMS-packages.txt" }
$packages = @(
 @{Name="papinho-secure-transport-0.4.0-src.zip";Hash="";Id="source"},
 @{Name="papinho-secure-transport-0.4.0-win32-x86-vc6-retrozilla-nss.zip";Hash="";Id="win32-x86-vc6-retrozilla-nss"},
 @{Name="papinho-secure-transport-0.4.0-win32-x64-msvc-19.51-schannel.zip";Hash="";Id="win32-x64-msvc-19.51-schannel"},
 @{Name="papinho-secure-transport-0.4.0-win32-x64-msvc-19.51-openssl3.zip";Hash="";Id="win32-x64-msvc-19.51-openssl3"},
 @{Name="papinho-secure-transport-0.4.0-win32-x64-msvc-19.51-schannel-openssl3.zip";Hash="";Id="win32-x64-msvc-19.51-schannel-openssl3"}
)
if ($ExpectedChecksumsFile) {
 $checksumPath=[IO.Path]::GetFullPath($ExpectedChecksumsFile);$expected=@{}
 foreach($line in [IO.File]::ReadAllLines($checksumPath)){
  if($line -notmatch '^([0-9a-f]{64})  ([^\\/]+\.zip)$'){throw "invalid external checksum line"}
  if($expected.ContainsKey($matches[2])){throw "duplicate external checksum entry"}
  $expected[$matches[2]]=$matches[1]
 }
 if($expected.Count -ne 5){throw "expected exactly five external checksums"}
 foreach($package in $packages){if(-not $expected.ContainsKey($package.Name)){throw ("missing external checksum: "+$package.Name)};$package.Hash=$expected[$package.Name]}
}
function Report($Name,$Value) { Write-Output ($Name+"="+$Value) }
function Require-File($Path,$Reason) { if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) { throw ($Reason+": "+$Path) } }
function Verify-Sums($Root) {
 $sumFile=Join-Path $Root "SHA256SUMS.txt"; Require-File $sumFile "missing SHA256SUMS"; $listed=0
 foreach($line in [IO.File]::ReadAllLines($sumFile)) {
  if($line -notmatch '^([0-9a-f]{64})  (.+)$'){throw "invalid SHA256SUMS line"}
  $listed++;$path=Join-Path $Root $matches[2].Replace('/','\');Require-File $path "missing hashed file"
  if((Get-FileHash -Algorithm SHA256 -LiteralPath $path).Hash.ToLowerInvariant() -ne $matches[1]){throw ("internal hash mismatch: "+$path)}
 }
 $actual=(Get-ChildItem -LiteralPath $Root -Recurse -File|Where-Object Name -ne "SHA256SUMS.txt").Count
 if($listed -ne $actual){throw "SHA256SUMS coverage mismatch"}
}
if(Test-Path -LiteralPath $ValidationDirectory){Remove-Item -LiteralPath $ValidationDirectory -Recurse -Force}
New-Item -ItemType Directory -Path $ValidationDirectory -Force|Out-Null
Add-Type -AssemblyName System.IO.Compression.FileSystem
foreach($package in $packages){
 $zipPath=Join-Path $PackageDirectory $package.Name;Report "PACKAGE" $package.Name;Require-File $zipPath "missing package"
 $actualHash=(Get-FileHash -Algorithm SHA256 -LiteralPath $zipPath).Hash.ToLowerInvariant()
 if($actualHash -ne $package.Hash){Report "HASH" "FAIL";throw "external hash mismatch"};Report "HASH" "PASS"
 $extract=Join-Path $ValidationDirectory $package.Id;[IO.Compression.ZipFile]::ExtractToDirectory($zipPath,$extract);Report "EXTRACT" "PASS"
 Verify-Sums $extract;Report "INTERNAL_SHA256" "PASS";Require-File (Join-Path $extract "LICENSE") "missing MPL license";Require-File (Join-Path $extract "THIRD_PARTY_NOTICES.md") "missing notices"
 if(Test-Path -LiteralPath (Join-Path $extract "docs\codex")){throw "internal docs leaked"}
 if($package.Id -eq "source"){
  $snapshot=Join-Path $extract "third_party\retrozilla-nss\source\retrozilla-2f274574d3c6ee8769914046920d649bbae9f81b-patched.zip"
  Require-File $snapshot "missing NSS corresponding source";Require-File (Join-Path $extract "third_party\retrozilla-nss\patches\0001-win32-secure-rng-fail-closed-nt4.patch") "missing NSS patch"
  if((Get-FileHash -Algorithm SHA256 -LiteralPath $snapshot).Hash.ToLowerInvariant() -ne "5371ce6fb2fd0df909faaed4cf92dc9c112844e1d1bedd7a8dc7f598b900d388"){throw "NSS source hash mismatch"}
  Require-File (Join-Path $extract "docs\target-matrix.md") "missing canonical target matrix"
  Require-File (Join-Path $extract "third_party\openssl\prebuilt\win32-x64-msvc-19.51-openssl3\3.5.8\MANIFEST.sha256") "missing OpenSSL prebuilt provenance"
  Report "SOURCE" "PASS"
 }else{
  foreach($file in @("VERSION","manifest.ini","consumer-link.ini","include\papinho_secure_transport.h","include\papinho_secure_transport_win32.h")){Require-File (Join-Path $extract $file) "SDK boundary failure"}
  if((Get-Content -Raw (Join-Path $extract "manifest.ini")) -notmatch 'source_package=papinho-secure-transport-0.4.0-src.zip'){throw "missing exact source reference"}
  $manifest=Get-Content -Raw (Join-Path $extract "manifest.ini")
  if($manifest -notmatch ('(?m)^target_id=' + [regex]::Escape($package.Id) + '$')){throw "manifest target_id mismatch"}
  if((Get-Content -Raw (Join-Path $extract "consumer-link.ini")) -notmatch ('(?m)^target_id=' + [regex]::Escape($package.Id) + '$')){throw "consumer-link target_id mismatch"}
  Require-File (Join-Path $extract "docs\target-matrix.md") "missing canonical target matrix"
  if($package.Id -eq "win32-x64-msvc-19.51-schannel-openssl3" -and $manifest -notmatch '(?m)^provider_ids=schannel,openssl$'){throw "combined provider order mismatch"}
  Report "LICENSE" "PASS";Report "SOURCE" "PASS"
 };Report "ARCHIVE" "PASS"
}
if($CompileConsumers){
 $consumerCount=0
 $consumer=Join-Path $repo "tests\release_package_consumer.c"
 foreach($package in $packages|Where-Object Id -ne "source"){
  $sdk=Join-Path $ValidationDirectory $package.Id;$work=Join-Path $ValidationDirectory ("consumer-"+$package.Id);New-Item -ItemType Directory -Path $work -Force|Out-Null
  $include=Join-Path $sdk "include";$lib=Join-Path $sdk ("lib\"+$package.Id);$exe=Join-Path $work "consumer.exe"
  $link=((Get-Content (Join-Path $sdk "consumer-link.ini")|Where-Object{$_ -like 'link_libraries=*'}).Substring(15)).Replace(',',' ')
  $define=if($package.Id -like '*openssl*'){'/DPST_RELEASE_EXPECT_OPENSSL'}else{''}
  $runtimeFlag=if($package.Id -like '*vc6*'){''}else{'/MD'}
  $envBat=if($package.Id -like '*vc6*'){Join-Path $repo 'tools\vc6-env.bat'}else{Join-Path $repo 'tools\msvc-env.bat'}
  $object=Join-Path $work 'release_package_consumer.obj'
  $command='call "'+$envBat+'" >nul && cl /nologo /W4 '+$runtimeFlag+' '+$define+' /I"'+$include+'" /Fo"'+$object+'" /Fe"'+$exe+'" /Tc"'+$consumer+'" /link /LIBPATH:"'+$lib+'" '+$link
  cmd.exe /d /c $command
  if($LASTEXITCODE -ne 0){Report "COMPILE" "FAIL";throw ("consumer compile/link failed: "+$package.Id)}
  Report "PACKAGE" $package.Id;Report "COMPILE" "PASS";Report "LINK" "PASS"
  $runtime=Join-Path $sdk ("runtime\"+$package.Id);if(Test-Path -LiteralPath $runtime){Get-ChildItem -LiteralPath $runtime -File|Copy-Item -Destination $work -Force}
  $consumerOut=Join-Path $work "consumer.out";$consumerErr=Join-Path $work "consumer.err"
  $consumerProcess=Start-Process -FilePath $exe -WorkingDirectory $work -Wait -PassThru -RedirectStandardOutput $consumerOut -RedirectStandardError $consumerErr
  Get-Content $consumerOut;if((Get-Item $consumerErr).Length){Get-Content $consumerErr}
  if($consumerProcess.ExitCode -ne 0){Report "RUNTIME" "FAIL";throw ("consumer runtime failed: "+$package.Id)}
  Report "RUNTIME" "PASS";Report "RESULT" "PASS";$consumerCount++
 }
 if($consumerCount -ne 4){throw ("consumer count mismatch: "+$consumerCount)}
 Report "CONSUMER_COUNT" $consumerCount
}
Report "CLEAN_MACHINE_RUNTIME" "NOT_PERFORMED"
Report "NT4_FINAL_RUNTIME_VALIDATION" "PASS_RECORDED_EXTERNAL_EVIDENCE"
Report "RESULT" "PASS"
