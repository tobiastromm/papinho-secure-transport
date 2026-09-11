# SPDX-License-Identifier: MPL-2.0
param([ValidateSet("0.5.0", "0.6.0", "0.6.1")][string]$Version="0.6.1",[string]$PackageDirectory,[string]$ValidationDirectory,[string]$ExpectedChecksumsFile,[switch]$CompileConsumers)
$ErrorActionPreference = "Stop"
$repo = Split-Path -Parent $PSScriptRoot
$libraryVersion = if ($Version -eq "0.5.0") { "0.5.0" } else { $Version }
$apiVersion = if ($Version -eq "0.5.0") { "2.0.0" } else { "2.1.0" }
if (-not $PackageDirectory) { $PackageDirectory = Join-Path $repo ("dist\packages\"+$Version) }
if (-not $ValidationDirectory) { $ValidationDirectory = Join-Path $repo ("dist\validation\"+$Version) }
$PackageDirectory = [IO.Path]::GetFullPath($PackageDirectory)
$ValidationDirectory = [IO.Path]::GetFullPath($ValidationDirectory)
if (-not $ExpectedChecksumsFile) { $ExpectedChecksumsFile = Join-Path $PackageDirectory "SHA256SUMS-packages.txt" }
$packages = @(
 @{Name=("papinho-secure-transport-"+$Version+"-src.zip");Hash="";Id="source"},
 @{Name=("papinho-secure-transport-"+$Version+"-win32-x86-vc6-retrozilla-nss.zip");Hash="";Id="win32-x86-vc6-retrozilla-nss"},
 @{Name=("papinho-secure-transport-"+$Version+"-win32-x64-msvc-19.51-schannel.zip");Hash="";Id="win32-x64-msvc-19.51-schannel"},
 @{Name=("papinho-secure-transport-"+$Version+"-win32-x64-msvc-19.51-openssl3.zip");Hash="";Id="win32-x64-msvc-19.51-openssl3"},
 @{Name=("papinho-secure-transport-"+$Version+"-win32-x64-msvc-19.51-schannel-openssl3.zip");Hash="";Id="win32-x64-msvc-19.51-schannel-openssl3"}
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
function Verify-ZipLayout($ZipPath) {
 $archive=[IO.Compression.ZipFile]::OpenRead($ZipPath)
 try {
  $names=New-Object 'System.Collections.Generic.HashSet[string]' ([StringComparer]::Ordinal)
  foreach($entry in $archive.Entries) {
   $name=$entry.FullName.Replace('\','/')
   if([string]::IsNullOrEmpty($name) -or $name.StartsWith('/') -or $name -match '(^|/)\.\.(/|$)' -or [IO.Path]::IsPathRooted($name)){throw ("unsafe ZIP entry: "+$name)}
   if(-not $names.Add($name)){throw ("duplicate ZIP entry: "+$name)}
  }
 } finally { $archive.Dispose() }
}
if(Test-Path -LiteralPath $ValidationDirectory){Remove-Item -LiteralPath $ValidationDirectory -Recurse -Force}
New-Item -ItemType Directory -Path $ValidationDirectory -Force|Out-Null
Add-Type -AssemblyName System.IO.Compression.FileSystem
$sourceExtract=$null
foreach($package in $packages){
 $zipPath=Join-Path $PackageDirectory $package.Name;Report "PACKAGE" $package.Name;Require-File $zipPath "missing package"
 $actualHash=(Get-FileHash -Algorithm SHA256 -LiteralPath $zipPath).Hash.ToLowerInvariant()
 if($actualHash -ne $package.Hash){Report "HASH" "FAIL";throw "external hash mismatch"};Report "HASH" "PASS"
 Verify-ZipLayout $zipPath;Report "ZIP_LAYOUT" "PASS"
 $extract=Join-Path $ValidationDirectory $package.Id;[IO.Compression.ZipFile]::ExtractToDirectory($zipPath,$extract);Report "EXTRACT" "PASS"
 Verify-Sums $extract;Report "INTERNAL_SHA256" "PASS";Require-File (Join-Path $extract "LICENSE") "missing MPL license";Require-File (Join-Path $extract "THIRD_PARTY_NOTICES.md") "missing notices"
 if(Test-Path -LiteralPath (Join-Path $extract "docs\codex")){throw "internal docs leaked"}
 if($package.Id -eq "source"){
  $sourceExtract=$extract
  $snapshot=Join-Path $extract "third_party\retrozilla-nss\source\retrozilla-2f274574d3c6ee8769914046920d649bbae9f81b-patched.zip"
  Require-File $snapshot "missing NSS corresponding source";Require-File (Join-Path $extract "third_party\retrozilla-nss\patches\0001-win32-secure-rng-fail-closed-nt4.patch") "missing NSS patch"
  if((Get-FileHash -Algorithm SHA256 -LiteralPath $snapshot).Hash.ToLowerInvariant() -ne "5371ce6fb2fd0df909faaed4cf92dc9c112844e1d1bedd7a8dc7f598b900d388"){throw "NSS source hash mismatch"}
  Require-File (Join-Path $extract "docs\target-matrix.md") "missing canonical target matrix"
  Require-File (Join-Path $extract "third_party\openssl\prebuilt\win32-x64-msvc-19.51-openssl3\3.5.8\MANIFEST.sha256") "missing OpenSSL prebuilt provenance"
  Report "SOURCE" "PASS"
 }else{
  foreach($file in @("VERSION","manifest.ini","consumer-link.ini","include\papinho_secure_transport.h","include\papinho_secure_transport_win32.h")){Require-File (Join-Path $extract $file) "SDK boundary failure"}
  if((Get-Content -Raw (Join-Path $extract "manifest.ini")) -notmatch ('source_package=papinho-secure-transport-'+[regex]::Escape($Version)+'-src.zip')){throw "missing exact source reference"}
  $manifest=Get-Content -Raw (Join-Path $extract "manifest.ini")
  $versionText=Get-Content -Raw (Join-Path $extract "VERSION")
  foreach($requiredVersion in @("package_version=$Version","library_version=$libraryVersion","api_version=$apiVersion","spi_version=3.0")){if($versionText -notmatch ('(?m)^'+[regex]::Escape($requiredVersion)+'$')){throw ("VERSION mismatch: "+$requiredVersion)}}
  if($manifest -notmatch ('(?m)^target_id=' + [regex]::Escape($package.Id) + '$')){throw "manifest target_id mismatch"}
  if((Get-Content -Raw (Join-Path $extract "consumer-link.ini")) -notmatch ('(?m)^target_id=' + [regex]::Escape($package.Id) + '$')){throw "consumer-link target_id mismatch"}
  Require-File (Join-Path $extract "docs\target-matrix.md") "missing canonical target matrix"
  if($package.Id -eq "win32-x64-msvc-19.51-schannel-openssl3" -and $manifest -notmatch '(?m)^provider_ids=schannel,openssl$'){throw "combined provider order mismatch"}
  Report "LICENSE" "PASS";Report "SOURCE" "PASS"
 };Report "ARCHIVE" "PASS"
 if($package.Id -eq "win32-x86-vc6-retrozilla-nss"){
  Report "NSS_PACKAGE_HASH" "PASS"
  Report "NSS_PACKAGE_LAYOUT" "PASS"
  Report "NSS_PACKAGE_EXTRACT" "PASS"
  Report "NSS_PACKAGE_INTERNAL_SHA256" "PASS"
  Report "NSS_PACKAGE_LICENSE" "PASS"
  Report "NSS_PACKAGE_SOURCE" "PASS"
  Report "NSS_PACKAGE_ARCHIVE" "PASS"
 }
}
if($CompileConsumers){
 $consumerCount=0
 if(-not$sourceExtract){throw "source package was not extracted"}
 $vc6Probe='call "'+(Join-Path $sourceExtract 'tools\vc6-env.bat')+'" >nul 2>&1 && where cl >nul 2>&1'
 cmd.exe /d /c $vc6Probe
 $vc6Available=$LASTEXITCODE -eq 0
 $consumerTemplate=Join-Path $sourceExtract "tests\release_package_consumer.c"
 $api21Template=Join-Path $sourceExtract "tests\release_package_api21_consumer.c"
 foreach($package in $packages|Where-Object Id -ne "source"){
  if($package.Id -like '*vc6*' -and -not $vc6Available){
   Report "NSS_CONSUMER_COMPILE_LINK" "NOT_APPLICABLE_TOOLCHAIN_UNAVAILABLE"
   continue
  }
  $sdk=Join-Path $ValidationDirectory $package.Id;$work=Join-Path $ValidationDirectory ("consumer-"+$package.Id);New-Item -ItemType Directory -Path $work -Force|Out-Null
  $consumer=Join-Path $work "release_package_consumer.c";Copy-Item -LiteralPath $consumerTemplate -Destination $consumer
  $include=Join-Path $sdk "include";$lib=Join-Path $sdk ("lib\"+$package.Id)
  $link=((Get-Content (Join-Path $sdk "consumer-link.ini")|Where-Object{$_ -like 'link_libraries=*'}).Substring(15)).Replace(',',' ')
  $runtimeFlag=if($package.Id -like '*vc6*'){''}else{'/MD'}
  $envBat=if($package.Id -like '*vc6*'){Join-Path $sourceExtract 'tools\vc6-env.bat'}else{Join-Path $sourceExtract 'tools\msvc-env.bat'}
  $runtime=Join-Path $sdk ("runtime\"+$package.Id);if(Test-Path -LiteralPath $runtime){Get-ChildItem -LiteralPath $runtime -File|Copy-Item -Destination $work -Force}
  foreach($role in @("client","server")) {
   $exe=Join-Path $work ("consumer-"+$role+".exe");$object=Join-Path $work ("release_package_consumer-"+$role+".obj")
   $define=if($role -eq "server"){'/DPST_RELEASE_SERVER_CONSUMER'}else{''}
   $command='call "'+$envBat+'" >nul && cl /nologo /W4 '+$runtimeFlag+' '+$define+' /I"'+$include+'" /Fo"'+$object+'" /Fe"'+$exe+'" /Tc"'+$consumer+'" /link /LIBPATH:"'+$lib+'" '+$link
   cmd.exe /d /c $command
   if($LASTEXITCODE -ne 0){Report "COMPILE" "FAIL";throw ("consumer compile/link failed: "+$package.Id+" "+$role)}
   Report "PACKAGE" $package.Id;Report "ROLE" $role.ToUpperInvariant();Report "COMPILE" "PASS";Report "LINK" "PASS"
   $consumerOut=Join-Path $work ("consumer-"+$role+".out");$consumerErr=Join-Path $work ("consumer-"+$role+".err")
   $consumerProcess=Start-Process -FilePath $exe -WorkingDirectory $work -Wait -PassThru -RedirectStandardOutput $consumerOut -RedirectStandardError $consumerErr
   Get-Content $consumerOut;if((Get-Item $consumerErr).Length){Get-Content $consumerErr}
   if($consumerProcess.ExitCode -ne 0){Report "RUNTIME" "FAIL";throw ("consumer runtime failed: "+$package.Id+" "+$role)}
   Report "RUNTIME" "PASS";Report "RESULT" "PASS";$consumerCount++
  }
  if($package.Id -like '*vc6*'){Report "NSS_CONSUMER_COMPILE_LINK" "PASS"}
  if($Version -ne "0.5.0") {
   $api21Source=Join-Path $work "release_package_api21_consumer.c";Copy-Item -LiteralPath $api21Template -Destination $api21Source
   $api21Object=Join-Path $work "release_package_api21_consumer.obj";$api21Exe=Join-Path $work "release_package_api21_consumer.exe"
   $api21Command='call "'+$envBat+'" >nul && cl /nologo /W4 '+$runtimeFlag+' /I"'+$include+'" /Fo"'+$api21Object+'" /Fe"'+$api21Exe+'" /Tc"'+$api21Source+'" /link /LIBPATH:"'+$lib+'" '+$link
   cmd.exe /d /c $api21Command
   if($LASTEXITCODE -ne 0){throw ("API 2.1 package consumer compile/link failed: "+$package.Id)}
   $api21Process=Start-Process -FilePath $api21Exe -WorkingDirectory $work -Wait -PassThru
   if($api21Process.ExitCode -ne 0){throw ("API 2.1 package consumer runtime failed: "+$package.Id)}
   Report "API21_PACKAGE_CONSUMER" ($package.Id+":PASS")
  }
 }
 $expectedConsumerCount=if($vc6Available){8}else{6}
 if($consumerCount -ne $expectedConsumerCount){throw ("consumer count mismatch: "+$consumerCount)}
 Report "CONSUMER_COUNT" $consumerCount
 $combinedId="win32-x64-msvc-19.51-schannel-openssl3";$combinedSdk=Join-Path $ValidationDirectory $combinedId;$combinedWork=Join-Path $ValidationDirectory "consumer-combined-selection"
 New-Item -ItemType Directory -Path $combinedWork -Force|Out-Null
 $selectionSource=Join-Path $sourceExtract "tests\test_public_combined_selection.c";$selectionObject=Join-Path $combinedWork "test_public_combined_selection.obj";$selectionExe=Join-Path $combinedWork "test_public_combined_selection.exe"
 $combinedInclude=Join-Path $combinedSdk "include";$combinedLib=Join-Path $combinedSdk ("lib\"+$combinedId);$combinedLink=((Get-Content (Join-Path $combinedSdk "consumer-link.ini")|Where-Object{$_ -like 'link_libraries=*'}).Substring(15)).Replace(',',' ')
 $selectionCommand='call "'+(Join-Path $sourceExtract 'tools\msvc-env.bat')+'" >nul && cl /nologo /W4 /MD /I"'+$combinedInclude+'" /Fo"'+$selectionObject+'" /Fe"'+$selectionExe+'" /Tc"'+$selectionSource+'" /link /LIBPATH:"'+$combinedLib+'" '+$combinedLink
 cmd.exe /d /c $selectionCommand
 if($LASTEXITCODE -ne 0){Report "COMBINED_SELECTION" "FAIL";throw "Combined public selection compile/link failed"}
 $combinedRuntime=Join-Path $combinedSdk ("runtime\"+$combinedId);Get-ChildItem -LiteralPath $combinedRuntime -File|Copy-Item -Destination $combinedWork -Force
 $selectionProcess=Start-Process -FilePath $selectionExe -WorkingDirectory $combinedWork -Wait -PassThru
 if($selectionProcess.ExitCode -ne 0){Report "COMBINED_SELECTION" "FAIL";throw "Combined public selection runtime failed"}
 Report "COMBINED_SELECTION" "PASS"
 if($Version -ne "0.5.0"){Report "API21_PACKAGE_ONLY_CONSUMER" "PASS";Report "NO_PRIVATE_HEADER_DEPENDENCY" "PASS"}
}
Report "CLEAN_MACHINE_RUNTIME" "NOT_PERFORMED"
Report "NT4_PACKAGE_RUNTIME_VALIDATION" "NOT_PERFORMED"
Report "RESULT" "PASS"
