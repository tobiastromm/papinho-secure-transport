# SPDX-License-Identifier: MPL-2.0
param([string]$PackageDirectory,[string]$ValidationDirectory,[switch]$CompileConsumers)
$ErrorActionPreference = "Stop"
$repo = Split-Path -Parent $PSScriptRoot
if (-not $PackageDirectory) { $PackageDirectory = Join-Path $repo "dist\packages\0.4.0" }
if (-not $ValidationDirectory) { $ValidationDirectory = Join-Path $repo "dist\validation\0.4.0" }
$PackageDirectory = [IO.Path]::GetFullPath($PackageDirectory)
$ValidationDirectory = [IO.Path]::GetFullPath($ValidationDirectory)
$packages = @(
 @{Name="papinho-secure-transport-0.4.0-src.zip";Hash="8d20b8975c06029fc828a42cdb6c75dc96c74ee1e0dedab867077fbf21cc223d";Id="source"},
 @{Name="papinho-secure-transport-0.4.0-windows-nt4-x86-vc6-retrozilla-nss.zip";Hash="55e38a19d743849317cab919dc7ba682ee823137666ee60004cf562e1788bed4";Id="windows-nt4-x86-vc6-retrozilla-nss"},
 @{Name="papinho-secure-transport-0.4.0-windows-x64-msvc-schannel.zip";Hash="044f22dc2eac6ef82a53a096f0a27d1ce6719bb7d7798d71b6ebaa648f53eda8";Id="windows-x64-msvc-schannel"},
 @{Name="papinho-secure-transport-0.4.0-windows-x64-msvc-openssl-3.5.8.zip";Hash="a59847425895dad2b3b8bb96ae40b70e2698f3a02d1812c0f8de40404953741f";Id="windows-x64-msvc-openssl-3.5.8"},
 @{Name="papinho-secure-transport-0.4.0-windows-x64-msvc-schannel-openssl-3.5.8.zip";Hash="8e78dd36f6f9486c7eea50df093a74c3cd2838b0a1823b8f9c406f3d496a0010";Id="windows-x64-msvc-schannel-openssl-3.5.8"}
)
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
  if((Get-FileHash -Algorithm SHA256 -LiteralPath $snapshot).Hash.ToLowerInvariant() -ne "5371ce6fb2fd0df909faaed4cf92dc9c112844e1d1bedd7a8dc7f598b900d388"){throw "NSS source hash mismatch"};Report "SOURCE" "PASS"
 }else{
  foreach($file in @("VERSION","manifest.ini","consumer-link.ini","include\papinho_secure_transport.h","include\papinho_secure_transport_win32.h")){Require-File (Join-Path $extract $file) "SDK boundary failure"}
  if((Get-Content -Raw (Join-Path $extract "manifest.ini")) -notmatch 'source_package=papinho-secure-transport-0.4.0-src.zip'){throw "missing exact source reference"}
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
  & $exe;if($LASTEXITCODE -ne 0){Report "RUNTIME" "FAIL";throw ("consumer runtime failed: "+$package.Id)}
  Report "RUNTIME" "PASS";Report "RESULT" "PASS";$consumerCount++
 }
 if($consumerCount -ne 4){throw ("consumer count mismatch: "+$consumerCount)}
 Report "CONSUMER_COUNT" $consumerCount
}
Report "CLEAN_MACHINE_RUNTIME" "NOT_PERFORMED"
Report "NT4_FINAL_RUNTIME_VALIDATION" "PENDING_REAL_NT4"
Report "RESULT" "PASS_WITH_REQUIRED_EXTERNAL_NT4_GATE_PENDING"
