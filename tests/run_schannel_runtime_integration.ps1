param([ValidateSet("12","13")][string]$TlsVersion="12",[int]$Port=8472,[int]$Exchanges=10,[ValidateSet("client","peer-clean","peer-abrupt")][string]$CloseMode="client")
$ErrorActionPreference="Stop"
$root=Join-Path (Get-Location) "build\fixtures\interoperability-pki\root.der"
$chain=Join-Path (Get-Location) "build\fixtures\interoperability-pki\server-chain.pem"
$key=Join-Path (Get-Location) "build\fixtures\interoperability-pki\server.key"
$serverOut=Join-Path (Get-Location) ("build\win64-modern-msvc\schannel-tls"+$TlsVersion+"-server.log")
$serverErr=Join-Path (Get-Location) ("build\win64-modern-msvc\schannel-tls"+$TlsVersion+"-server.err")
$server=$null
$added=$false
$certificate=$null
try {
    if(-not (Test-Path -LiteralPath $root)){& powershell -NoProfile -ExecutionPolicy Bypass -File tests\generate_interop_pki.ps1;if($LASTEXITCODE -ne 0){throw "PKI generation failed"}}
    $certificate=New-Object Security.Cryptography.X509Certificates.X509Certificate2($root)
    $store=New-Object Security.Cryptography.X509Certificates.X509Store("Root","CurrentUser")
    $store.Open([Security.Cryptography.X509Certificates.OpenFlags]::ReadWrite)
    $existing=$store.Certificates.Find([Security.Cryptography.X509Certificates.X509FindType]::FindByThumbprint,$certificate.Thumbprint,$false)
    if($existing.Count -eq 0){$store.Add($certificate);$added=$true}
    $store.Close()
    Write-Output "TRUST_READY STORE=CurrentUser/Root THUMBPRINT=$($certificate.Thumbprint) ADDED=$([int]$added)"
    Remove-Item -LiteralPath $serverOut,$serverErr -Force -ErrorAction SilentlyContinue
    $arguments=@("tests\schannel_backend_tls_server.py",$Port,$chain,$key,$TlsVersion,$Exchanges,$CloseMode)
    $server=Start-Process -FilePath python -ArgumentList $arguments -RedirectStandardOutput $serverOut -RedirectStandardError $serverErr -PassThru -WindowStyle Hidden
    $deadline=[DateTime]::UtcNow.AddSeconds(15)
    do {Start-Sleep -Milliseconds 100;$ready=(Test-Path -LiteralPath $serverOut) -and ((Get-Content -Raw $serverOut) -match "READY")} while(-not $ready -and -not $server.HasExited -and [DateTime]::UtcNow -lt $deadline)
    if(-not $ready){throw "server did not become ready: $(Get-Content -Raw $serverErr -ErrorAction SilentlyContinue)"}
    Get-Content $serverOut
    & build\win64-modern-msvc\test_schannel_runtime_integration.exe 127.0.0.1 $Port localhost $TlsVersion $Exchanges $CloseMode
    $clientExit=$LASTEXITCODE
    if(-not $server.WaitForExit(30000)){$server.Kill();throw "server completion timeout"}
    $server.WaitForExit()
    Get-Content $serverOut
    if(Test-Path -LiteralPath $serverErr){Get-Content $serverErr}
    $serverExit=$server.ExitCode
    if($null -eq $serverExit -and (Get-Content -Raw $serverOut) -match "PASS TLS"){$serverExit=0}
    if($clientExit -ne 0 -or $serverExit -ne 0){throw "integration failed: client=$clientExit server=$serverExit"}
} finally {
    if($server -ne $null -and -not $server.HasExited){$server.Kill()}
    if($added -and $certificate -ne $null){$store=New-Object Security.Cryptography.X509Certificates.X509Store("Root","CurrentUser");$store.Open([Security.Cryptography.X509Certificates.OpenFlags]::ReadWrite);$matches=$store.Certificates.Find([Security.Cryptography.X509Certificates.X509FindType]::FindByThumbprint,$certificate.Thumbprint,$false);foreach($match in $matches){$store.Remove($match)};$store.Close();Write-Output "TRUST_REMOVED STORE=CurrentUser/Root THUMBPRINT=$($certificate.Thumbprint)"}
}