# SPDX-License-Identifier: MPL-2.0
param([string]$BundleDirectory=(Split-Path -Parent $PSScriptRoot),[int]$BasePort=8900)
$ErrorActionPreference="Stop"
$bundle=[IO.Path]::GetFullPath($BundleDirectory)
$source=Join-Path $bundle "work\source"
$packages=Join-Path $bundle "work\packages"
$sdk=Join-Path $packages "win32-x64-msvc-19.51-schannel-openssl3"
$target="win32-x64-msvc-19.51-schannel-openssl3"
$include=Join-Path $sdk "include";$library=Join-Path $sdk ("lib\"+$target);$runtime=Join-Path $sdk ("runtime\"+$target)
$output=Join-Path $bundle "combined-real-tls";$pki=Join-Path $output "pki";$bin=Join-Path $output "bin";$logs=Join-Path $output "logs"
foreach($required in @($source,$include,$library,$runtime)){if(-not(Test-Path -LiteralPath $required)){throw("missing package-only input: "+$required)}}
if(Test-Path -LiteralPath $output){Remove-Item -LiteralPath $output -Recurse -Force}
New-Item -ItemType Directory -Path $pki,$bin,$logs|Out-Null
Push-Location $bundle
try{& powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $source "tests\generate_interop_pki.ps1") -OutputDirectory "combined-real-tls\pki";if($LASTEXITCODE-ne 0){throw "PKI generation failed"}}finally{Pop-Location}
Get-ChildItem -LiteralPath $runtime -Filter "*.dll" -File|Copy-Item -Destination $bin
$schannelClient=Join-Path $bin "combined-client-schannel.exe";$opensslClient=Join-Path $bin "combined-client-openssl.exe";$combinedServer=Join-Path $bin "combined-server.exe"
$schannelSource=Join-Path $output "combined-client-schannel.c";$opensslSource=Join-Path $output "combined-client-openssl.c"
[IO.File]::WriteAllText($schannelSource,([IO.File]::ReadAllText((Join-Path $source "tests\test_schannel_runtime_integration.c")).Replace("info.provider_count!=1","info.provider_count<2")),(New-Object Text.UTF8Encoding($false)))
[IO.File]::WriteAllText($opensslSource,([IO.File]::ReadAllText((Join-Path $source "tests\test_openssl_identity_integration.c")).Replace("info.provider_count!=1","info.provider_count<2")),(New-Object Text.UTF8Encoding($false)))
$link='papinho_secure_transport.lib libssl.lib libcrypto.lib ws2_32.lib secur32.lib crypt32.lib ncrypt.lib bcrypt.lib'
function Compile-Harness([string]$HarnessSource,[string]$Executable,[switch]$PrivateHeaders){
 $response=$Executable+".rsp";$arguments=@('/nologo','/MD','/W4','/D_CRT_SECURE_NO_WARNINGS',('/I"'+$include+'"'))
 if($PrivateHeaders){$arguments+=('/I"'+(Join-Path $source 'src')+'"')}
 $arguments+=@('/Fe"'+$Executable+'"','/Tc"'+$HarnessSource+'"','/link','/LIBPATH:"'+$library+'"')+$link.Split(' ')
 [IO.File]::WriteAllLines($response,$arguments,(New-Object Text.ASCIIEncoding))
 $command='call "'+(Join-Path $source 'tools\msvc-env.bat')+'" >nul && cl @"'+$response+'"'
 cmd.exe /d /c $command
 if($LASTEXITCODE-ne 0){throw("compile/link failed: "+$HarnessSource)}
}
Compile-Harness $schannelSource $schannelClient
Compile-Harness $opensslSource $opensslClient
Compile-Harness (Join-Path $source "tests\test_openssl_server_integration.c") $combinedServer -PrivateHeaders
function Wait-Ready($Process,[string]$Out){
 $deadline=[DateTime]::UtcNow.AddSeconds(20);do{Start-Sleep -Milliseconds 100;$Process.Refresh();if((Test-Path $Out)-and((Get-Content -Raw $Out)-match '(?m)^READY ')){return}}while(-not$Process.HasExited-and[DateTime]::UtcNow-lt$deadline);throw("readiness failed: "+$Out)
}
function Finish-Process($Process,[string]$Out,[string]$Err){
 if(-not$Process.WaitForExit(30000)){$Process.Kill();throw("process timeout: "+$Out)};$Process.Refresh();Get-Content $Out;if((Test-Path $Err)-and(Get-Item $Err).Length){Get-Content $Err};if($Process.ExitCode-ne 0){throw("process failed: "+$Out)}
}
function Run-CombinedClient([string]$Name,[string]$Provider,[int]$Tls,[int]$Port){
 $serverOut=Join-Path $logs ($Name+"-server.log");$serverErr=Join-Path $logs ($Name+"-server.err");$clientOut=Join-Path $logs ($Name+"-client.log");$clientErr=Join-Path $logs ($Name+"-client.err")
 $server=Start-Process python -ArgumentList @((Join-Path $source "tests\schannel_backend_tls_server.py"),$Port,(Join-Path $pki "server-chain.pem"),(Join-Path $pki "server.key"),$Tls,1,"client","fixture/1","-","-") -RedirectStandardOutput $serverOut -RedirectStandardError $serverErr -WindowStyle Hidden -PassThru
 $null=$server.Handle
 Wait-Ready $server $serverOut
 $exe=if($Provider-eq"schannel"){$schannelClient}else{$opensslClient}
 $arguments=@("127.0.0.1",$Port,"localhost",$Tls,1,"client","custom",(Join-Path $pki "root.der"),"required","fixture/1","-","-","OK",4)
 $client=Start-Process $exe -ArgumentList $arguments -WorkingDirectory $bin -Wait -PassThru -RedirectStandardOutput $clientOut -RedirectStandardError $clientErr
 Get-Content $clientOut;if((Get-Item $clientErr).Length){Get-Content $clientErr};if($client.ExitCode-ne 0){throw("client failed: "+$Name)}
 Finish-Process $server $serverOut $serverErr
 $text=Get-Content -Raw $clientOut;if($text-notmatch 'WRITE=25 READ=25 CONTENT_MATCH=1 PASS=1'-or$text-notmatch 'SHUTDOWN=COMPLETE'){throw("client assertions failed: "+$Name)}
 Write-Output ("COMBINED_CLIENT_CASE="+$Name+" PROVIDER="+$Provider+" TLS="+$Tls+" PASS=1")
}
function Run-CombinedServer([string]$Name,[string]$Selection,[int]$Tls,[string]$Alpn,[string]$Close,[int]$Port,[string]$ExpectedProvider){
 $serverOut=Join-Path $logs ($Name+"-server.log");$serverErr=Join-Path $logs ($Name+"-server.err");$clientOut=Join-Path $logs ($Name+"-client.log");$clientErr=Join-Path $logs ($Name+"-client.err")
 $scenario=if($Close-eq"data-abrupt"){"truncate"}else{"echo"}
 $serverArgs=@($Port,$Tls,0,(Join-Path $pki "server.der"),(Join-Path $pki "intermediate.der"),(Join-Path $pki "server.pk8"),(Join-Path $pki "root.der"),$Selection,$Alpn,$scenario,"custom")
 $server=Start-Process $combinedServer -ArgumentList $serverArgs -WorkingDirectory $bin -RedirectStandardOutput $serverOut -RedirectStandardError $serverErr -WindowStyle Hidden -PassThru
 $null=$server.Handle
 Wait-Ready $server $serverOut
 if($ExpectedProvider-eq"openssl"-and$Name-eq"exact-openssl-tls12"){
  $modules=@($server.Modules|Where-Object ModuleName -Match '^lib(ssl|crypto)-3-x64\.dll$');$modules|ForEach-Object{Write-Output("COMBINED_LOADED_MODULE="+$_.FileName)}
  if($modules.Count-ne 2-or@($modules|Where-Object{-not$_.FileName.StartsWith($bin,[StringComparison]::OrdinalIgnoreCase)}).Count){throw "non-package OpenSSL module loaded"}
 }
 $expectedAlpn=if($Alpn-eq"-"){"-"}else{"fixture/1"};$clientAlpn=if($Alpn-eq"-"){"-"}else{"fixture/1"}
 $client=Start-Process python -ArgumentList @((Join-Path $source "tests\openssl_server_test_client.py"),"127.0.0.1",$Port,$Tls,(Join-Path $pki "root.pem"),"-","-",$clientAlpn,$Close,$expectedAlpn) -RedirectStandardOutput $clientOut -RedirectStandardError $clientErr -WindowStyle Hidden -Wait -PassThru
 Get-Content $clientOut;if((Get-Item $clientErr).Length){Get-Content $clientErr};if($client.ExitCode-ne 0){throw("fixture client failed: "+$Name)}
 Finish-Process $server $serverOut $serverErr
 $text=Get-Content -Raw $serverOut;if($text-notmatch("BOUND_PROVIDER="+[regex]::Escape($ExpectedProvider))){throw("provider mismatch: "+$Name)}
 if($Close-eq"data-abrupt"){if($text-notmatch 'SERVER_TRUNCATION=PASS'){throw("no-fallback truncation failed: "+$Name)}}elseif($text-notmatch 'READ=25 WRITE=25 CONTENT_MATCH=1'.Replace(' ','.*')){throw("server I/O failed: "+$Name)}
 if($text-notmatch 'SERVER_LOGGING=PASS'-or$text-notmatch 'LOG_SECRET_HITS=0'){throw("logging failed: "+$Name)}
 Write-Output ("COMBINED_SERVER_CASE="+$Name+" PROVIDER="+$ExpectedProvider+" TLS="+$Tls+" PASS=1")
}
Run-CombinedClient "exact-schannel-tls12" "schannel" 12 ($BasePort+1)
Run-CombinedClient "exact-openssl-tls12" "openssl" 12 ($BasePort+2)
Run-CombinedClient "exact-openssl-tls13" "openssl" 13 ($BasePort+3)
Run-CombinedServer "exact-schannel-tls12" "exact-schannel" 12 "-" "clean" ($BasePort+4) "schannel"
Run-CombinedServer "exact-openssl-tls12" "exact" 12 "fixture/1" "clean" ($BasePort+5) "openssl"
Run-CombinedServer "exact-openssl-tls13" "exact" 13 "fixture/1" "clean" ($BasePort+6) "openssl"
Run-CombinedServer "ordered-schannel-first" "ordered" 12 "-" "clean" ($BasePort+7) "schannel"
Run-CombinedServer "ordered-openssl-first" "ordered-openssl" 12 "-" "clean" ($BasePort+8) "openssl"
Run-CombinedServer "automatic" "automatic" 12 "-" "clean" ($BasePort+9) "schannel"
Run-CombinedServer "alpn-prebinding-filter" "ordered" 12 "fixture/1" "clean" ($BasePort+10) "openssl"
Run-CombinedServer "no-post-binding-fallback" "ordered" 12 "fixture/1" "data-abrupt" ($BasePort+11) "openssl"
foreach($dll in @("libssl-3-x64.dll","libcrypto-3-x64.dll")){$a=(Get-FileHash (Join-Path $runtime $dll) -Algorithm SHA256).Hash;$b=(Get-FileHash (Join-Path $bin $dll) -Algorithm SHA256).Hash;if($a-ne$b){throw("runtime hash mismatch: "+$dll)};Write-Output("COMBINED_RUNTIME_HASH="+$dll+" SHA256="+$a.ToLower()+" PASS=1")}
$api21Consumer=Join-Path $packages "consumer-win32-x64-msvc-19.51-schannel-openssl3\release_package_api21_consumer.exe"
if(-not(Test-Path -LiteralPath $api21Consumer -PathType Leaf)){throw "missing Combined API 2.1 package-only consumer"}
$api21=Start-Process -FilePath $api21Consumer -WorkingDirectory (Split-Path -Parent $api21Consumer) -Wait -PassThru
if($api21.ExitCode-ne 0){throw "Combined API 2.1 wait-set consumer failed"}
Write-Output "COMBINED_API21_WAITSET=PASS"
$secretHits=@(Get-ChildItem $logs -File|Select-String -Pattern 'BEGIN (RSA |EC |)PRIVATE KEY|PRIVATE_KEY').Count;if($secretHits-ne 0){throw "secret found in logs"}
Write-Output "COMBINED_LOG_SECRET_HITS=0"
Write-Output "COMBINED_REAL_TLS_CLEAN_MACHINE=PASS"
