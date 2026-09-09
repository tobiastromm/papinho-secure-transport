# SPDX-License-Identifier: MPL-2.0
param([string]$Destination)
$ErrorActionPreference="Stop"
$repo=Split-Path -Parent $PSScriptRoot
if(-not$Destination){$Destination=Join-Path $repo "build\ss8-nt4-package-validation"}
$destination=[IO.Path]::GetFullPath($Destination)
$packages=Join-Path $repo "dist\packages\0.5.0"
$nssZip=Join-Path $packages "papinho-secure-transport-0.5.0-win32-x86-vc6-retrozilla-nss.zip"
$sourceZip=Join-Path $packages "papinho-secure-transport-0.5.0-src.zip"
$work=Join-Path $repo "build\ss8-nt4-package-build"
if(Test-Path -LiteralPath $work){Remove-Item -LiteralPath $work -Recurse -Force}
New-Item -ItemType Directory -Path $work|Out-Null
Expand-Archive -LiteralPath $nssZip -DestinationPath (Join-Path $work "sdk")
Expand-Archive -LiteralPath $sourceZip -DestinationPath (Join-Path $work "source")
$sdk=Join-Path $work "sdk";$source=Join-Path $work "source";$target="win32-x86-vc6-retrozilla-nss"
$object=Join-Path $work "test_nss_server_direct.obj";$exe=Join-Path $work "test_nss_server_direct.exe"
$command='call "'+(Join-Path $source 'tools\vc6-env.bat')+'" >nul && cl /nologo /W4 /O2 /TC /I"'+(Join-Path $sdk 'include')+'" /I"'+(Join-Path $source 'src')+'" /I"'+(Join-Path $source 'third_party\retrozilla-nss\prebuilt\win32-x86-vc6\sdk\include\nspr')+'" /I"'+(Join-Path $source 'third_party\retrozilla-nss\prebuilt\win32-x86-vc6\sdk\public\nss')+'" /Fo"'+$object+'" /c "'+(Join-Path $source 'tests\test_nss_server_direct.c')+'" && cl /nologo /Fe"'+$exe+'" "'+$object+'" "'+(Join-Path $sdk ('lib\'+$target+'\papinho_secure_transport.lib'))+'" wsock32.lib'
cmd.exe /d /c $command
if($LASTEXITCODE-ne 0){throw "package-derived NT4 validation executable build failed"}
if(Test-Path -LiteralPath $destination){Remove-Item -LiteralPath $destination -Recurse -Force}
New-Item -ItemType Directory -Path $destination|Out-Null
Copy-Item -LiteralPath $exe -Destination $destination
Get-ChildItem -LiteralPath (Join-Path $sdk ('runtime\'+$target)) -File|Copy-Item -Destination $destination
$fixtures=Join-Path $repo "build\fixtures\interoperability-pki"
foreach($name in @("server.der","intermediate.der","server.pk8","root.der")){Copy-Item -LiteralPath (Join-Path $fixtures $name) -Destination $destination}
$runners=@{
 "run_tls12.bat"=@("@echo off",'if "%1"=="" goto usage',"set PST_NSS_TRACE_FILE=tls12-backend.log","test_nss_server_direct.exe %1 12 0 server.der intermediate.der server.pk8 root.der","if errorlevel 1 goto fail","echo PAPINHOSECURETRANSPORT SS-8 NT4 PACKAGE TLS 1.2 PASS","goto end",":usage","echo Usage: run_tls12.bat PORT","goto fail_end",":fail","echo PAPINHOSECURETRANSPORT SS-8 NT4 PACKAGE TLS 1.2 FAIL",":fail_end","verify other 2>nul",":end");
 "run_tls13_mtls.bat"=@("@echo off",'if "%1"=="" goto usage',"set PST_NSS_TRACE_FILE=tls13-mtls-backend.log","test_nss_server_direct.exe %1 13 2 server.der intermediate.der server.pk8 root.der","if errorlevel 1 goto fail","echo PAPINHOSECURETRANSPORT SS-8 NT4 PACKAGE TLS 1.3 MTLS PASS","goto end",":usage","echo Usage: run_tls13_mtls.bat PORT","goto fail_end",":fail","echo PAPINHOSECURETRANSPORT SS-8 NT4 PACKAGE TLS 1.3 MTLS FAIL",":fail_end","verify other 2>nul",":end");
 "run_truncation.bat"=@("@echo off",'if "%1"=="" goto usage',"set PST_NSS_TRACE_FILE=truncation-backend.log","test_nss_server_direct.exe %1 12 0 server.der intermediate.der server.pk8 root.der 0 0 0 - - raw-abrupt","if errorlevel 1 goto fail","echo PAPINHOSECURETRANSPORT SS-8 NT4 PACKAGE TRUNCATION PASS","goto end",":usage","echo Usage: run_truncation.bat PORT","goto fail_end",":fail","echo PAPINHOSECURETRANSPORT SS-8 NT4 PACKAGE TRUNCATION FAIL",":fail_end","verify other 2>nul",":end")}
foreach($entry in $runners.GetEnumerator()){[IO.File]::WriteAllLines((Join-Path $destination $entry.Key),$entry.Value,[Text.Encoding]::ASCII)}
[IO.File]::WriteAllLines((Join-Path $destination "README.txt"),@("PST SS-8 NT4 package-derived validation","Source SDK ZIP SHA-256: "+(Get-FileHash $nssZip).Hash.ToLowerInvariant(),"Run one BAT at a time on real Windows NT 4.0 SP6 x86.","Return console output and every *-backend.log file.","PENDING until real NT4 execution."),[Text.Encoding]::ASCII)
$lines=Get-ChildItem -LiteralPath $destination -File|Sort-Object Name|ForEach-Object{"{0} *{1}"-f(Get-FileHash -Algorithm SHA256 $_.FullName).Hash.ToLowerInvariant(),$_.Name}
[IO.File]::WriteAllLines((Join-Path $destination "MANIFEST.sha256"),$lines,[Text.Encoding]::ASCII)
Write-Output ("SS8_NT4_PACKAGE_BUNDLE="+$destination)
Write-Output "NT4_PACKAGE_EXECUTION=AWAITING_OWNER_EXECUTION"
