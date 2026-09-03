param([string[]]$Endpoints=@("www.cloudflare.com","www.google.com"))
$ErrorActionPreference="Stop"
if($Endpoints.Count -lt 2){throw "At least two configurable endpoints are required"}
$exe=Join-Path (Get-Location) "build\win64-modern-msvc-openssl\test_openssl_identity_integration.exe"
if(-not(Test-Path $exe)){throw "Build system-trust-integration first"}
function Test-TcpReachability([string]$Address,[int]$Port,[int]$TimeoutMilliseconds){
    $client=New-Object Net.Sockets.TcpClient
    try{
        $pending=$client.BeginConnect($Address,$Port,$null,$null)
        if(-not $pending.AsyncWaitHandle.WaitOne($TimeoutMilliseconds,$false)){return $false}
        $client.EndConnect($pending)
        return $true
    }catch{return $false}finally{$client.Close()}
}
foreach($endpoint in $Endpoints){
    try{$ip=([Net.Dns]::GetHostAddresses($endpoint)|Where-Object AddressFamily -eq InterNetwork|Select-Object -First 1).IPAddressToString}catch{Write-Output "ENVIRONMENT_FAILURE ENDPOINT=$endpoint REASON=DNS";exit 2}
    if(-not $ip){Write-Output "ENVIRONMENT_FAILURE ENDPOINT=$endpoint REASON=NO_IPV4";exit 2}
    if(-not(Test-TcpReachability $ip 443 10000)){Write-Output "ENVIRONMENT_FAILURE ENDPOINT=$endpoint REASON=TCP_UNREACHABLE";exit 2}
    foreach($tls in @("12","13")){
        Write-Output "SYSTEM_TRUST_BEGIN ENDPOINT=$endpoint TLS=$tls CUSTOM_CA_CONFIGURED=0 SYSTEM_TRUST_SELECTED=1 APPLICATION_BYTES_SENT=0"
        & $exe $ip 443 $endpoint $tls 0 bounded system - disabled - - - OK 0
        if($LASTEXITCODE-ne 0){Write-Output "PST_SYSTEM_TRUST_FAILURE ENDPOINT=$endpoint TLS=$tls";exit 1}
    }
}
Write-Output "OPENSSL_SYSTEM_TRUST_PUBLIC_MATRIX=PASS"