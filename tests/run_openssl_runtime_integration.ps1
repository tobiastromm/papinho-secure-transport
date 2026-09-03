param(
    [ValidateSet("12","13")][string]$MinimumTls="12",
    [ValidateSet("12","13")][string]$MaximumTls="13",
    [string]$ServerMinimumTls="",
    [string]$ServerMaximumTls="",
    [int]$Port=8480,
    [int]$Exchanges=1,
    [ValidateSet("client","peer-clean","data-then-close","peer-abrupt")][string]$CloseMode="client",
    [ValidateSet("OK","PROTOCOL","AUTH","TRUNCATED")][string]$Expected="OK",
    [int]$PayloadSize=25,
    [string]$CaseName="manual"
)
$ErrorActionPreference="Stop"
if(-not $ServerMinimumTls){$ServerMinimumTls=$MinimumTls}
if(-not $ServerMaximumTls){$ServerMaximumTls=$MaximumTls}
$repo=[IO.Path]::GetFullPath((Get-Location).Path)
$pki=Join-Path $repo "build\fixtures\interoperability-pki"
$artifacts=Join-Path $repo "build\phase-ossl-c"
[IO.Directory]::CreateDirectory($artifacts)|Out-Null
if(-not(Test-Path (Join-Path $pki "root.der"))){& powershell -NoProfile -ExecutionPolicy Bypass -File tests\generate_interop_pki.ps1;if($LASTEXITCODE-ne 0){throw "PKI generation failed"}}
$manifest=Join-Path $repo "third_party\openssl\prebuilt\win64-msvc-3.5.8\runtime\MANIFEST.sha256"
$hashOk=$true
foreach($line in Get-Content $manifest){$parts=$line -split '  ',2;$actual=(Get-FileHash -Algorithm SHA256 (Join-Path (Split-Path $manifest) $parts[1])).Hash.ToLowerInvariant();if($actual-ne $parts[0]){$hashOk=$false}}
if(-not $hashOk){throw "canonical runtime hash mismatch"}
$serverOut=Join-Path $artifacts "$CaseName-server.log";$serverErr=Join-Path $artifacts "$CaseName-server.err";$clientOut=Join-Path $artifacts "$CaseName-client.log";$clientErr=Join-Path $artifacts "$CaseName-client.err"
Remove-Item $serverOut,$serverErr,$clientOut,$clientErr -Force -ErrorAction SilentlyContinue
$failureArg=if($Expected-ne"OK"){"expect-failure"}else{"success"}
$arguments=@("tests\openssl_tls_server.py",$Port,(Join-Path $pki "server-chain.pem"),(Join-Path $pki "server.key"),$ServerMinimumTls,$ServerMaximumTls,$Exchanges,$CloseMode,$PayloadSize,$failureArg)
$server=Start-Process python -ArgumentList $arguments -RedirectStandardOutput $serverOut -RedirectStandardError $serverErr -PassThru -WindowStyle Hidden
$null=$server.Handle
$deadline=[DateTime]::UtcNow.AddSeconds(15);$ready=$false
do{Start-Sleep -Milliseconds 100;if(Test-Path $serverOut){$ready=(Get-Content -Raw $serverOut)-match"READY"};$server.Refresh()}while(-not$ready-and-not$server.HasExited-and[DateTime]::UtcNow-lt$deadline)
if(-not$ready){if(Test-Path $serverOut){Get-Content $serverOut};if(Test-Path $serverErr){Get-Content $serverErr};throw "server readiness failed"}
$clientArgs=@("127.0.0.1",$Port,$MinimumTls,$MaximumTls,$Exchanges,$CloseMode,(Join-Path $pki "root.der"),$Expected,$PayloadSize)
$client=Start-Process (Join-Path $repo "build\win64-modern-msvc-openssl\test_openssl_runtime_integration.exe") -ArgumentList $clientArgs -RedirectStandardOutput $clientOut -RedirectStandardError $clientErr -PassThru -WindowStyle Hidden
$null=$client.Handle
if(-not$client.WaitForExit(60000)){$client.Kill();$server.Kill();throw "client timeout"};$client.Refresh()
if(-not$server.WaitForExit(30000)){$server.Kill();throw "server timeout"};$server.Refresh()
Get-Content $clientOut;if(Test-Path $clientErr){Get-Content $clientErr};Get-Content $serverOut;if(Test-Path $serverErr){Get-Content $serverErr}
$summary="CASE=$CaseName CLIENT_EXIT=$($client.ExitCode) SERVER_EXIT=$($server.ExitCode) RUNTIME_HASH_MATCH=1"
$summary|Tee-Object -FilePath (Join-Path $artifacts "$CaseName-summary.txt")
if($client.ExitCode-ne 0-or$server.ExitCode-ne 0){exit 1}