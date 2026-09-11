# SPDX-License-Identifier: MPL-2.0
param([string]$Destination,[ValidateSet("0.5.0","0.6.0")][string]$Version="0.6.0")
$ErrorActionPreference="Stop"
$repo=Split-Path -Parent $PSScriptRoot
if(-not$Destination){$Destination=Join-Path $repo ("build\m10-nt4-package-validation-"+$Version)}
$destination=[IO.Path]::GetFullPath($Destination)
$packages=Join-Path $repo ("dist\packages\"+$Version)
$nssZip=Join-Path $packages ("papinho-secure-transport-"+$Version+"-win32-x86-vc6-retrozilla-nss.zip")
$sourceZip=Join-Path $packages ("papinho-secure-transport-"+$Version+"-src.zip")
$work=Join-Path $repo ("build\m10-nt4-package-build-"+$Version)
if(Test-Path -LiteralPath $work){Remove-Item -LiteralPath $work -Recurse -Force}
New-Item -ItemType Directory -Path $work|Out-Null
Expand-Archive -LiteralPath $nssZip -DestinationPath (Join-Path $work "sdk")
Expand-Archive -LiteralPath $sourceZip -DestinationPath (Join-Path $work "source")
$sdk=Join-Path $work "sdk";$source=Join-Path $work "source";$target="win32-x86-vc6-retrozilla-nss"
$object=Join-Path $work "test_nss_server_direct.obj";$exe=Join-Path $work "test_nss_server_direct.exe"
$command='call "'+(Join-Path $source 'tools\vc6-env.bat')+'" >nul && cl /nologo /W4 /O2 /TC /I"'+(Join-Path $sdk 'include')+'" /I"'+(Join-Path $source 'src')+'" /I"'+(Join-Path $source 'third_party\retrozilla-nss\prebuilt\win32-x86-vc6\sdk\include\nspr')+'" /I"'+(Join-Path $source 'third_party\retrozilla-nss\prebuilt\win32-x86-vc6\sdk\public\nss')+'" /Fo"'+$object+'" /c "'+(Join-Path $source 'tests\test_nss_server_direct.c')+'" && cl /nologo /Fe"'+$exe+'" "'+$object+'" "'+(Join-Path $sdk ('lib\'+$target+'\papinho_secure_transport.lib'))+'" wsock32.lib'
cmd.exe /d /c $command
if($LASTEXITCODE-ne 0){throw "package-derived NT4 validation executable build failed"}
$api21Object=Join-Path $work "release_package_api21_consumer.obj";$api21Exe=Join-Path $work "release_package_api21_consumer.exe"
$api21Command='call "'+(Join-Path $source 'tools\vc6-env.bat')+'" >nul && cl /nologo /W4 /O2 /TC /I"'+(Join-Path $sdk 'include')+'" /Fo"'+$api21Object+'" /Fe"'+$api21Exe+'" "'+(Join-Path $source 'tests\release_package_api21_consumer.c')+'" /link /LIBPATH:"'+(Join-Path $sdk ('lib\'+$target))+'" papinho_secure_transport.lib wsock32.lib'
cmd.exe /d /c $api21Command
if($LASTEXITCODE-ne 0){throw "package-derived NT4 API 2.1 scheduler executable build failed"}
$clientObject=Join-Path $work "test_tls_runtime_integration.obj";$clientExe=Join-Path $work "test_tls_runtime_integration.exe"
$clientCommand='call "'+(Join-Path $source 'tools\vc6-env.bat')+'" >nul && cl /nologo /W4 /O2 /TC /I"'+(Join-Path $sdk 'include')+'" /Fo"'+$clientObject+'" /Fe"'+$clientExe+'" "'+(Join-Path $source 'tests\test_tls_runtime_integration.c')+'" /link /LIBPATH:"'+(Join-Path $sdk ('lib\'+$target))+'" papinho_secure_transport.lib wsock32.lib'
cmd.exe /d /c $clientCommand
if($LASTEXITCODE-ne 0){throw "package-derived NT4 CLIENT/API 2.1 integration executable build failed"}
if(Test-Path -LiteralPath $destination){Remove-Item -LiteralPath $destination -Recurse -Force}
New-Item -ItemType Directory -Path $destination|Out-Null
Copy-Item -LiteralPath $exe -Destination $destination
Copy-Item -LiteralPath $api21Exe -Destination $destination
Copy-Item -LiteralPath $clientExe -Destination $destination
Get-ChildItem -LiteralPath (Join-Path $sdk ('runtime\'+$target)) -File|Copy-Item -Destination $destination
$fixtures=Join-Path $repo "build\fixtures\interoperability-pki"
foreach($name in @("server.der","intermediate.der","server.pk8","root.der","client.der","client.pk8")){Copy-Item -LiteralPath (Join-Path $fixtures $name) -Destination $destination}
$caPath=Join-Path $destination "ca.der";Copy-Item -LiteralPath (Join-Path $fixtures "root.der") -Destination $caPath
$runners=@{
 "run_tls12.bat"=@("@echo off",'if "%1"=="" goto usage',"set PST_NSS_TRACE_FILE=tls12-backend.log","test_nss_server_direct.exe %1 12 0 server.der intermediate.der server.pk8 root.der","if errorlevel 1 goto fail","echo PAPINHOSECURETRANSPORT M10 NT4 PACKAGE TLS 1.2 PASS","goto end",":usage","echo Usage: run_tls12.bat PORT","goto fail_end",":fail","echo PAPINHOSECURETRANSPORT M10 NT4 PACKAGE TLS 1.2 FAIL",":fail_end","verify other 2>nul",":end");
 "run_tls13_mtls.bat"=@("@echo off",'if "%1"=="" goto usage',"set PST_NSS_TRACE_FILE=tls13-mtls-backend.log","echo NSS_SERVER_ALPN_ADVERTISED=NO","test_nss_server_direct.exe %1 13 2 server.der intermediate.der server.pk8 root.der 0 0 0 - -","if errorlevel 1 goto fail","echo PAPINHOSECURETRANSPORT M10 NT4 PACKAGE TLS 1.3 MTLS PASS","goto end",":usage","echo Usage: run_tls13_mtls.bat PORT","goto fail_end",":fail","echo PAPINHOSECURETRANSPORT M10 NT4 PACKAGE TLS 1.3 MTLS FAIL",":fail_end","verify other 2>nul",":end");
 "run_truncation.bat"=@("@echo off",'if "%1"=="" goto usage',"set PST_NSS_TRACE_FILE=truncation-backend.log","test_nss_server_direct.exe %1 12 0 server.der intermediate.der server.pk8 root.der 0 0 0 - - raw-abrupt","if errorlevel 1 goto fail","echo PAPINHOSECURETRANSPORT M10 NT4 PACKAGE TRUNCATION PASS","goto end",":usage","echo Usage: run_truncation.bat PORT","goto fail_end",":fail","echo PAPINHOSECURETRANSPORT M10 NT4 PACKAGE TRUNCATION FAIL",":fail_end","verify other 2>nul",":end");
 "run_api21_scheduler.bat"=@("@echo off","release_package_api21_consumer.exe","if errorlevel 1 goto fail","echo PAPINHOSECURETRANSPORT M10 NT4 API 2.1 SCHEDULER PASS","goto end",":fail","echo PAPINHOSECURETRANSPORT M10 NT4 API 2.1 SCHEDULER FAIL",":end")}
$runners["run_client_tls12.bat"]=@("@echo off",'if "%3"=="" goto usage',"set PST_NSS_TRACE_FILE=client-tls12-backend.log","test_tls_runtime_integration.exe %1 %2 %3 ca.der client.der client.pk8 12 12 fixture/1","if errorlevel 1 goto fail","echo PAPINHOSECURETRANSPORT M10 NT4 CLIENT TLS 1.2 API 2.1 WAITSET PASS","goto end",":usage","echo Usage: run_client_tls12.bat HOST PORT HOSTNAME","goto fail",":fail","echo PAPINHOSECURETRANSPORT M10 NT4 CLIENT TLS 1.2 API 2.1 WAITSET FAIL",":end")
$runners["run_client_tls13.bat"]=@("@echo off",'if "%3"=="" goto usage',"set PST_NSS_TRACE_FILE=client-tls13-backend.log","test_tls_runtime_integration.exe %1 %2 %3 ca.der client.der client.pk8 13 13 fixture/1","if errorlevel 1 goto fail","echo PAPINHOSECURETRANSPORT M10 NT4 CLIENT TLS 1.3 API 2.1 WAITSET PASS","goto end",":usage","echo Usage: run_client_tls13.bat HOST PORT HOSTNAME","goto fail",":fail","echo PAPINHOSECURETRANSPORT M10 NT4 CLIENT TLS 1.3 API 2.1 WAITSET FAIL",":end")
foreach($entry in $runners.GetEnumerator()){[IO.File]::WriteAllLines((Join-Path $destination $entry.Key),$entry.Value,[Text.Encoding]::ASCII)}
[IO.File]::WriteAllLines((Join-Path $destination "README.txt"),@("PST M10 NT4 package-derived validation","NSS SDK ZIP SHA-256: "+(Get-FileHash $nssZip).Hash.ToLowerInvariant(),"Run one BAT at a time on real Windows NT 4.0 SP6 x86.","Run run_api21_scheduler.bat and return its console output.","TLS 1.3 required-mTLS server: run_tls13_mtls.bat 8443","Modern client for that gate (from the project root):","python tests\openssl_server_test_client.py NT4_IP 8443 13 build\fixtures\interoperability-pki\root.pem build\fixtures\interoperability-pki\client-chain.pem build\fixtures\interoperability-pki\client.key - clean -","RetroZilla NSS SERVER does not advertise complete PST SERVER ALPN semantics; this gate intentionally sends and expects no ALPN.","Return console output and every *-backend.log file.","PENDING until real NT4 execution."),[Text.Encoding]::ASCII)
$lines=Get-ChildItem -LiteralPath $destination -File|Sort-Object Name|ForEach-Object{"{0} *{1}"-f(Get-FileHash -Algorithm SHA256 $_.FullName).Hash.ToLowerInvariant(),$_.Name}
[IO.File]::WriteAllLines((Join-Path $destination "MANIFEST.sha256"),$lines,[Text.Encoding]::ASCII)
Write-Output ("M10_NT4_PACKAGE_BUNDLE="+$destination)
Write-Output "NT4_PACKAGE_EXECUTION=AWAITING_OWNER_EXECUTION"
