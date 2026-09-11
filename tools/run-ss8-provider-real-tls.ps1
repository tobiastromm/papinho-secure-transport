# SPDX-License-Identifier: MPL-2.0
param([string]$BundleDirectory=(Split-Path -Parent $PSScriptRoot),[int]$BasePort=8920)
$ErrorActionPreference="Stop"
$bundle=[IO.Path]::GetFullPath($BundleDirectory)
$source=Join-Path $bundle "work\source"
$packages=Join-Path $bundle "work\packages"
$output=Join-Path $bundle "provider-real-tls"
$pki=Join-Path $output "pki"
$logs=Join-Path $output "logs"
foreach($required in @($source,$packages)){if(-not(Test-Path -LiteralPath $required)){throw("missing package-only input: "+$required)}}
if(Test-Path -LiteralPath $output){Remove-Item -LiteralPath $output -Recurse -Force}
New-Item -ItemType Directory -Path $output,$pki,$logs|Out-Null
Push-Location $bundle
try{& powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $source "tests\generate_interop_pki.ps1") -OutputDirectory "provider-real-tls\pki";if($LASTEXITCODE-ne 0){throw "PKI generation failed"}}finally{Pop-Location}

function Compile-Harness([string]$Target,[string]$SourceName,[string]$OutputName,[switch]$PrivateHeaders){
 $sdk=Join-Path $packages $Target;$include=Join-Path $sdk "include";$library=Join-Path $sdk ("lib\"+$Target);$runtime=Join-Path $sdk ("runtime\"+$Target);$bin=Join-Path $output $Target
 New-Item -ItemType Directory -Path $bin -Force|Out-Null
 Get-ChildItem -LiteralPath $runtime -Filter "*.dll" -File -ErrorAction SilentlyContinue|Copy-Item -Destination $bin -Force
 $exe=Join-Path $bin $OutputName;$response=$exe+".rsp"
 $arguments=@('/nologo','/MD','/W4','/D_CRT_SECURE_NO_WARNINGS',('/I"'+$include+'"'),('/I"'+(Join-Path $source 'tests')+'"'))
 if($PrivateHeaders){$arguments+=('/I"'+(Join-Path $source 'src')+'"')}
 $link=if($Target-like '*openssl3'){'papinho_secure_transport.lib libssl.lib libcrypto.lib ws2_32.lib secur32.lib crypt32.lib ncrypt.lib bcrypt.lib'}else{'papinho_secure_transport.lib ws2_32.lib secur32.lib crypt32.lib ncrypt.lib bcrypt.lib'}
 $arguments+=@('/Fe"'+$exe+'"','/Tc"'+(Join-Path $source ("tests\"+$SourceName))+'"','/link','/LIBPATH:"'+$library+'"')+$link.Split(' ')
 [IO.File]::WriteAllLines($response,$arguments,(New-Object Text.ASCIIEncoding))
 $command='call "'+(Join-Path $source 'tools\msvc-env.bat')+'" >nul && cl @"'+$response+'"'
 $compileOutput=cmd.exe /d /c $command;$compileExit=$LASTEXITCODE;$compileOutput|Write-Host;if($compileExit-ne 0){throw("compile/link failed: "+$SourceName)}
 return $exe
}
function Wait-Ready($Process,[string]$Out){$deadline=[DateTime]::UtcNow.AddSeconds(20);do{Start-Sleep -Milliseconds 100;$Process.Refresh();if((Test-Path $Out)-and((Get-Content -Raw $Out)-match '(?m)^READY ')){return}}while(-not$Process.HasExited-and[DateTime]::UtcNow-lt$deadline);throw("readiness failed: "+$Out)}
function Finish-Process($Process,[string]$Out,[string]$Err){if(-not$Process.WaitForExit(30000)){$Process.Kill();throw("process timeout: "+$Out)};$Process.Refresh();Get-Content $Out;if((Test-Path $Err)-and(Get-Item $Err).Length){Get-Content $Err};if($Process.ExitCode-ne 0){throw("process failed: "+$Out)}}
function Start-Fixture([string]$Name,[int]$Port,[int]$Tls,[switch]$ClientAuth){
 $out=Join-Path $logs ($Name+"-server.log");$err=Join-Path $logs ($Name+"-server.err");$ca=if($ClientAuth){Join-Path $pki 'client-ca.pem'}else{'-'};$hash=if($ClientAuth){(Get-FileHash (Join-Path $pki 'client.der') -Algorithm SHA256).Hash.ToLower()}else{'-'}
 $args=@((Join-Path $source 'tests\schannel_backend_tls_server.py'),$Port,(Join-Path $pki 'server-chain.pem'),(Join-Path $pki 'server.key'),$Tls,1,'client','fixture/1',$ca,$hash)
 $process=Start-Process python -ArgumentList $args -RedirectStandardOutput $out -RedirectStandardError $err -WindowStyle Hidden -PassThru;$null=$process.Handle;Wait-Ready $process $out;return @($process,$out,$err)
}
function Run-ProviderClient([string]$Name,[string]$Exe,[int]$Port,[int]$Tls){
 $fixture=Start-Fixture $Name $Port $Tls -ClientAuth;$out=Join-Path $logs ($Name+"-client.log");$err=Join-Path $logs ($Name+"-client.err")
 $args=@('127.0.0.1',$Port,'localhost',$Tls,1,'client','custom',(Join-Path $pki 'root.der'),'required','fixture/1',(Join-Path $pki 'client.der'),(Join-Path $pki 'client.pk8'),'OK',4)
 $client=Start-Process $Exe -ArgumentList $args -WorkingDirectory (Split-Path $Exe) -Wait -PassThru -RedirectStandardOutput $out -RedirectStandardError $err;Get-Content $out;if((Get-Item $err).Length){Get-Content $err};if($client.ExitCode-ne 0){throw("client failed: "+$Name)};Finish-Process $fixture[0] $fixture[1] $fixture[2]
 $text=Get-Content -Raw $out;if($text-notmatch 'WRITE=25 READ=25 CONTENT_MATCH=1 PASS=1'-or$text-notmatch 'SHUTDOWN=COMPLETE'){throw("client assertions failed: "+$Name)};Write-Output("PROVIDER_CLIENT_CASE="+$Name+" TLS="+$Tls+" PASS=1")
}
function Run-ProviderServer([string]$Name,[string]$Exe,[int]$Port,[int]$Tls,[string]$Provider,[switch]$Abrupt){
 $out=Join-Path $logs ($Name+"-server.log");$err=Join-Path $logs ($Name+"-server.err");$clientOut=Join-Path $logs ($Name+"-client.log");$clientErr=Join-Path $logs ($Name+"-client.err")
 if($Provider-eq'schannel'){$args=@($Port,$Tls,(Join-Path $pki 'server.der'),(Join-Path $pki 'intermediate.der'),(Join-Path $pki 'server.pk8'),(Join-Path $pki 'root.der'),'disabled');if($Abrupt){$args+='abrupt'}}else{$args=@($Port,$Tls,2,(Join-Path $pki 'server.der'),(Join-Path $pki 'intermediate.der'),(Join-Path $pki 'server.pk8'),(Join-Path $pki 'root.der'),'exact','fixture/1','echo','custom')}
 $server=Start-Process $Exe -ArgumentList $args -WorkingDirectory (Split-Path $Exe) -RedirectStandardOutput $out -RedirectStandardError $err -WindowStyle Hidden -PassThru;$null=$server.Handle;Wait-Ready $server $out
 $alpn=if($Provider-eq'openssl'){'fixture/1'}else{'-'};$cert=if($Provider-eq'openssl'){Join-Path $pki 'client.pem'}else{'-'};$key=if($Provider-eq'openssl'){Join-Path $pki 'client.key'}else{'-'};$close=if($Abrupt){'data-abrupt'}else{'clean'}
 $clientArgs=@((Join-Path $source 'tests\openssl_server_test_client.py'),'127.0.0.1',$Port,$Tls,(Join-Path $pki 'root.pem'),$cert,$key,$alpn,$close,$alpn)
 $client=Start-Process python -ArgumentList $clientArgs -RedirectStandardOutput $clientOut -RedirectStandardError $clientErr -WindowStyle Hidden -Wait -PassThru;Get-Content $clientOut;if((Get-Item $clientErr).Length){Get-Content $clientErr};if($client.ExitCode-ne 0){throw("fixture client failed: "+$Name)};Finish-Process $server $out $err
 $text=Get-Content -Raw $out;if($Abrupt){if($text-notmatch 'TRUNCATED'){throw("truncation assertion failed: "+$Name)}}elseif($text-notmatch 'READ=25 WRITE=25 CONTENT_MATCH=1'-or$text-notmatch 'PASS=1'){throw("server assertions failed: "+$Name)};Write-Output("PROVIDER_SERVER_CASE="+$Name+" TLS="+$Tls+" PASS=1")
}
$schannel='win32-x64-msvc-19.51-schannel';$openssl='win32-x64-msvc-19.51-openssl3'
$schannelClient=Compile-Harness $schannel 'test_schannel_runtime_integration.c' 'schannel-client.exe'
$schannelServer=Compile-Harness $schannel 'test_schannel_server_direct.c' 'schannel-server.exe' -PrivateHeaders
$opensslClient=Compile-Harness $openssl 'test_openssl_identity_integration.c' 'openssl-client.exe'
$opensslServer=Compile-Harness $openssl 'test_openssl_server_integration.c' 'openssl-server.exe' -PrivateHeaders
Run-ProviderClient 'schannel-client-tls12' $schannelClient ($BasePort+1) 12
Run-ProviderServer 'schannel-server-tls12' $schannelServer ($BasePort+2) 12 'schannel'
Run-ProviderServer 'schannel-server-truncation' $schannelServer ($BasePort+3) 12 'schannel' -Abrupt
Run-ProviderClient 'openssl-client-tls12' $opensslClient ($BasePort+4) 12
Run-ProviderClient 'openssl-client-tls13' $opensslClient ($BasePort+5) 13
Run-ProviderServer 'openssl-server-tls12' $opensslServer ($BasePort+6) 12 'openssl'
Run-ProviderServer 'openssl-server-tls13' $opensslServer ($BasePort+7) 13 'openssl'
$opensslRuntime=Join-Path $packages ($openssl+'\runtime\'+$openssl);$opensslBin=Split-Path $opensslClient
foreach($dll in @('libssl-3-x64.dll','libcrypto-3-x64.dll')){$a=(Get-FileHash (Join-Path $opensslRuntime $dll) -Algorithm SHA256).Hash;$b=(Get-FileHash (Join-Path $opensslBin $dll) -Algorithm SHA256).Hash;if($a-ne$b){throw("runtime hash mismatch: "+$dll)};Write-Output("OPENSSL_PACKAGE_LOCAL_DLL="+$dll+" SHA256="+$a.ToLower()+" PASS=1")}
$secretHits=@(Get-ChildItem $logs -File|Select-String -Pattern 'BEGIN (RSA |EC |)PRIVATE KEY|PRIVATE_KEY').Count;if($secretHits-ne 0){throw "secret found in logs"}
Write-Output 'SCHANNEL_REAL_TLS_CLEAN_MACHINE=PASS'
Write-Output 'OPENSSL_REAL_TLS12_CLEAN_MACHINE=PASS'
Write-Output 'OPENSSL_REAL_TLS13_CLEAN_MACHINE=PASS'
Write-Output 'OPENSSL_PACKAGE_LOCAL_DLLS=PASS'
Write-Output 'PROVIDER_REAL_TLS_LOG_SECRET_HITS=0'
