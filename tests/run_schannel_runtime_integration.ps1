# SPDX-License-Identifier: MPL-2.0
param(
    [ValidateSet("12","13")][string]$TlsVersion="12",
    [int]$Port=8472,
    [int]$Exchanges=10,
    [ValidateSet("client","peer-clean","peer-abrupt")][string]$CloseMode="client",
    [ValidateSet("System","Custom")][string]$TrustMode="System",
    [switch]$ClientAuth,
    [string]$Alpn="fixture/1"
)
$ErrorActionPreference="Stop"
$repo=[IO.Path]::GetFullPath((Get-Location).Path)
$pki=Join-Path $repo "build\fixtures\interoperability-pki"
$root=Join-Path $pki "root.der"
$chain=Join-Path $pki "server-chain.pem"
$key=Join-Path $pki "server.key"
$clientDer=Join-Path $pki "client.der"
$clientPk8=Join-Path $pki "client.pk8"
$clientCa=Join-Path $pki "client-ca.pem"
$serverOut=Join-Path $repo ("build\win64-modern-msvc\schannel-tls"+$TlsVersion+"-server.log")
$serverErr=Join-Path $repo ("build\win64-modern-msvc\schannel-tls"+$TlsVersion+"-server.err")
$server=$null;$added=$false;$certificate=$null;$stage="RUNNER_START";$failed=$false
Write-Output "RUNNER_START TLS=$TlsVersion PORT=$Port TRUST=$TrustMode CLIENT_AUTH=$([int]$ClientAuth.IsPresent)"
try {
    $stage="PKI_READY"
    if(-not (Test-Path -LiteralPath $root)){& powershell -NoProfile -ExecutionPolicy Bypass -File tests\generate_interop_pki.ps1;if($LASTEXITCODE -ne 0){throw "PKI generation failed"}}
    Write-Output "PKI_READY DIRECTORY=$pki"
    if($TrustMode -eq "System"){
        $certificate=New-Object Security.Cryptography.X509Certificates.X509Certificate2($root)
        $store=New-Object Security.Cryptography.X509Certificates.X509Store("Root","CurrentUser")
        $store.Open([Security.Cryptography.X509Certificates.OpenFlags]::ReadWrite)
        try{$existing=$store.Certificates.Find([Security.Cryptography.X509Certificates.X509FindType]::FindByThumbprint,$certificate.Thumbprint,$false);if($existing.Count -eq 0){$store.Add($certificate);$added=$true}}finally{$store.Close()}
        Write-Output "TRUST_READY MODE=SYSTEM_TEST_FIXTURE STORE=CurrentUser/Root THUMBPRINT=$($certificate.Thumbprint) ADDED=$([int]$added)"
    }else{Write-Output "TRUST_READY MODE=CUSTOM DER=$root"}
    Remove-Item -LiteralPath $serverOut,$serverErr -Force -ErrorAction SilentlyContinue
    $serverClientCa=if($ClientAuth){$clientCa}else{"-"}
    $expectedHash=if($ClientAuth){(Get-FileHash -LiteralPath $clientDer -Algorithm SHA256).Hash.ToLowerInvariant()}else{"-"}
    $stage="SERVER_START";Write-Output "SERVER_START STDOUT_PATH=$serverOut STDERR_PATH=$serverErr"
    $arguments=@("tests\schannel_backend_tls_server.py",$Port,$chain,$key,$TlsVersion,$Exchanges,$CloseMode,$Alpn,$serverClientCa,$expectedHash)
    $server=Start-Process -FilePath python -ArgumentList $arguments -RedirectStandardOutput $serverOut -RedirectStandardError $serverErr -PassThru -WindowStyle Hidden
    $null=$server.Handle
    $deadline=[DateTime]::UtcNow.AddSeconds(15);$ready=$false
    do {Start-Sleep -Milliseconds 100;if(Test-Path -LiteralPath $serverOut){$ready=(Get-Content -Raw $serverOut) -match "READY"};$server.Refresh()} while(-not $ready -and -not $server.HasExited -and [DateTime]::UtcNow -lt $deadline)
    if(-not $ready){throw "server readiness timeout/exited"}
    $stage="SERVER_READY";Write-Output "SERVER_READY";Get-Content $serverOut
    $trustKind=if($TrustMode -eq "Custom"){"custom"}else{"system"};$trustArg=if($TrustMode -eq "Custom"){$root}else{"-"};$certArg=if($ClientAuth){$clientDer}else{"-"};$keyArg=if($ClientAuth){$clientPk8}else{"-"}
    $stage="CLIENT_START";Write-Output "CLIENT_START"
    & build\win64-modern-msvc\test_schannel_runtime_integration.exe 127.0.0.1 $Port localhost $TlsVersion $Exchanges $CloseMode $trustKind $trustArg required $Alpn $certArg $keyArg OK
    $clientExit=$LASTEXITCODE;Write-Output "CLIENT_EXIT EXIT_CODE=$clientExit"
    $stage="SERVER_EXIT";if(-not $server.WaitForExit(30000)){$server.Kill();throw "server completion timeout"};$server.Refresh();$serverExit=$server.ExitCode
    Write-Output "SERVER_EXIT EXIT_CODE=$serverExit";Get-Content $serverOut;if(Test-Path -LiteralPath $serverErr){Get-Content $serverErr}
    if($clientExit -ne 0 -or $serverExit -ne 0){throw "integration failed: client=$clientExit server=$serverExit"}
}catch{$failed=$true;Write-Output "RUNNER_FAILURE STAGE=$stage PROCESS=$($server.Id) EXIT_CODE=$($server.ExitCode) TIMEOUT=$($_.Exception.Message -match 'timeout') STDOUT_PATH=$serverOut STDERR_PATH=$serverErr MESSAGE=$($_.Exception.Message)";if(Test-Path $serverOut){Get-Content $serverOut};if(Test-Path $serverErr){Get-Content $serverErr}}
finally{
    $stage="CLEANUP";Write-Output "CLEANUP"
    if($server -ne $null -and -not $server.HasExited){$server.Kill()}
    if($added -and $certificate -ne $null){$store=New-Object Security.Cryptography.X509Certificates.X509Store("Root","CurrentUser");$store.Open([Security.Cryptography.X509Certificates.OpenFlags]::ReadWrite);try{$matches=$store.Certificates.Find([Security.Cryptography.X509Certificates.X509FindType]::FindByThumbprint,$certificate.Thumbprint,$false);foreach($match in $matches){$store.Remove($match)}}finally{$store.Close()};Write-Output "TRUST_REMOVED STORE=CurrentUser/Root THUMBPRINT=$($certificate.Thumbprint)"}
    Write-Output "RUNNER_END SUCCESS=$([int](-not $failed))"
}
if($failed){exit 1}