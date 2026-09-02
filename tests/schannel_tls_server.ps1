param(
    [string]$BindAddress = "127.0.0.1",
    [int]$Port = 8462,
    [Parameter(Mandatory = $true)][string]$CertificateOutputPath,
    [int]$AcceptTimeoutSeconds = 120
)

$ErrorActionPreference = "Stop"
$expected = [Text.Encoding]::ASCII.GetBytes("pst-phase5-public-runtime")
$listener = $null
$client = $null
$tls = $null
$certificate = $null
$failed = $false

try {
    $certificatePath = [IO.Path]::GetFullPath((Join-Path (Get-Location) $CertificateOutputPath))
    [IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($certificatePath)) | Out-Null
    $certificate = New-SelfSignedCertificate `
        -Subject "CN=PST 7F SCHANNEL TEST ONLY" `
        -DnsName "localhost" `
        -CertStoreLocation "Cert:\CurrentUser\My" `
        -KeyAlgorithm RSA `
        -KeyLength 2048 `
        -HashAlgorithm SHA256 `
        -NotAfter (Get-Date).AddDays(30) `
        -Type SSLServerAuthentication
    Export-Certificate -Cert $certificate -FilePath $certificatePath -Type CERT -Force | Out-Null
    Write-Output "IDENTITY_CREATE STORE=CurrentUser/My THUMBPRINT=$($certificate.Thumbprint) PROFILE=RSA2048_SHA256 SAN=localhost"

    $address = [Net.IPAddress]::Parse($BindAddress)
    $listener = New-Object Net.Sockets.TcpListener($address, $Port)
    $listener.Start()
    Write-Output "READY $BindAddress`:$Port ENGINE=SCHANNEL TLS12 ACCEPT_TIMEOUT_SECONDS=$AcceptTimeoutSeconds CERT_DER=$certificatePath"

    $accept = $listener.AcceptTcpClientAsync()
    if (-not $accept.Wait($AcceptTimeoutSeconds * 1000)) {
        Write-Output "FIXTURE_TIMEOUT REASON=ACCEPT TIMEOUT_SECONDS=$AcceptTimeoutSeconds"
        throw "accept timeout"
    }
    $client = $accept.Result
    $client.ReceiveTimeout = 30000
    $client.SendTimeout = 30000
    Write-Output "ACCEPT REMOTE=$($client.Client.RemoteEndPoint)"

    $tls = New-Object Net.Security.SslStream($client.GetStream(), $false)
    $tls.ReadTimeout = 30000
    $tls.WriteTimeout = 30000
    $tls.AuthenticateAsServer($certificate, $false, [Security.Authentication.SslProtocols]::Tls12, $false)
    Write-Output "HANDSHAKE ENGINE=SCHANNEL TLS=$($tls.SslProtocol) CIPHER=$($tls.CipherAlgorithm) STRENGTH=$($tls.CipherStrength)"

    $received = New-Object byte[] $expected.Length
    $total = 0
    while ($total -lt $expected.Length) {
        $count = $tls.Read($received, $total, $expected.Length - $total)
        if ($count -eq 0) { break }
        $total += $count
    }
    $match = $total -eq $expected.Length
    if ($match) {
        for ($i = 0; $i -lt $expected.Length; ++$i) {
            if ($received[$i] -ne $expected[$i]) { $match = $false; break }
        }
    }
    $sent = 0
    if ($match) {
        $tls.Write($expected, 0, $expected.Length)
        $tls.Flush()
        $sent = $expected.Length
    }
    Write-Output "IO RECV=$total SEND=$sent CONTENT_MATCH=$($match.ToString().ToUpperInvariant())"
    if (-not $match) { throw "payload mismatch" }
    Write-Output "PASS ENGINE=SCHANNEL TLS12 WRITE=$sent READ=$total CONTENT_MATCH=1"
}
catch {
    $failed = $true
    $exception = $_.Exception
    $level = 0
    while ($exception -ne $null) {
        Write-Output "SCHANNEL_ERROR LEVEL=$level TYPE=$($exception.GetType().FullName) HRESULT=0x$($exception.HResult.ToString('X8')) MESSAGE=$($exception.Message)"
        $exception = $exception.InnerException
        ++$level
    }
}
finally {
    if ($tls -ne $null) { $tls.Dispose() }
    if ($client -ne $null) { $client.Close() }
    if ($listener -ne $null) { $listener.Stop() }
    if ($certificate -ne $null) {
        $thumbprint = $certificate.Thumbprint
        foreach ($storeName in "My", "CA") {
            $storePath = "Cert:\CurrentUser\$storeName\$thumbprint"
            if (Test-Path -LiteralPath $storePath) {
                Remove-Item -LiteralPath $storePath -Force
                Write-Output "IDENTITY_REMOVE STORE=CurrentUser/$storeName THUMBPRINT=$thumbprint"
            }
        }
    }
}

if ($failed) { exit 1 }