param([string]$OutputDirectory = "build\fixtures\interoperability-pki")

$ErrorActionPreference = "Stop"
$git = (Get-Command git -ErrorAction Stop).Source
$gitRoot = Split-Path (Split-Path $git -Parent) -Parent
$openssl = Join-Path $gitRoot "usr\bin\openssl.exe"
if (-not (Test-Path -LiteralPath $openssl -PathType Leaf)) {
    throw "Git OpenSSL not found at expected path: $openssl"
}

$output = [IO.Path]::GetFullPath((Join-Path (Get-Location) $OutputDirectory))
[IO.Directory]::CreateDirectory($output) | Out-Null

function Write-Ascii([string]$Name, [string]$Content) {
    [IO.File]::WriteAllText((Join-Path $output $Name), $Content, [Text.Encoding]::ASCII)
}
function Run-OpenSsl([string[]]$Arguments) {
    & $openssl @Arguments
    if ($LASTEXITCODE -ne 0) { throw "OpenSSL failed ($LASTEXITCODE): $($Arguments -join ' ')" }
}

Write-Ascii "root.cnf" @"
[req]
distinguished_name=dn
x509_extensions=v3_ca
prompt=no
[dn]
CN=PST 7F TEST ONLY ROOT CA
[v3_ca]
basicConstraints=critical,CA:true,pathlen:1
keyUsage=critical,keyCertSign,cRLSign
subjectKeyIdentifier=hash
authorityKeyIdentifier=keyid:always,issuer
"@
Write-Ascii "intermediate.cnf" @"
[req]
distinguished_name=dn
prompt=no
[dn]
CN=PST 7F TEST ONLY INTERMEDIATE CA
"@
Write-Ascii "intermediate.ext" @"
basicConstraints=critical,CA:true,pathlen:0
keyUsage=critical,keyCertSign,cRLSign
subjectKeyIdentifier=hash
authorityKeyIdentifier=keyid,issuer
"@
Write-Ascii "server.cnf" @"
[req]
distinguished_name=dn
prompt=no
[dn]
CN=localhost
"@
Write-Ascii "server.ext" @"
basicConstraints=critical,CA:false
keyUsage=critical,digitalSignature,keyEncipherment
extendedKeyUsage=serverAuth
subjectAltName=DNS:localhost
subjectKeyIdentifier=hash
authorityKeyIdentifier=keyid,issuer
"@
Run-OpenSsl @("req", "-x509", "-newkey", "rsa:2048", "-nodes", "-sha256", "-days", "30", "-config", (Join-Path $output "root.cnf"), "-keyout", (Join-Path $output "root.key"), "-out", (Join-Path $output "root.pem"))
Run-OpenSsl @("x509", "-in", (Join-Path $output "root.pem"), "-outform", "DER", "-out", (Join-Path $output "root.der"))
Run-OpenSsl @("req", "-new", "-newkey", "rsa:2048", "-nodes", "-sha256", "-config", (Join-Path $output "intermediate.cnf"), "-keyout", (Join-Path $output "intermediate.key"), "-out", (Join-Path $output "intermediate.csr"))
Run-OpenSsl @("x509", "-req", "-sha256", "-days", "30", "-in", (Join-Path $output "intermediate.csr"), "-CA", (Join-Path $output "root.pem"), "-CAkey", (Join-Path $output "root.key"), "-CAcreateserial", "-extfile", (Join-Path $output "intermediate.ext"), "-out", (Join-Path $output "intermediate.pem"))
Run-OpenSsl @("x509", "-in", (Join-Path $output "intermediate.pem"), "-outform", "DER", "-out", (Join-Path $output "intermediate.der"))
Run-OpenSsl @("req", "-new", "-newkey", "rsa:2048", "-nodes", "-sha256", "-config", (Join-Path $output "server.cnf"), "-keyout", (Join-Path $output "server.key"), "-out", (Join-Path $output "server.csr"))
Run-OpenSsl @("x509", "-req", "-sha256", "-days", "30", "-in", (Join-Path $output "server.csr"), "-CA", (Join-Path $output "intermediate.pem"), "-CAkey", (Join-Path $output "intermediate.key"), "-CAcreateserial", "-extfile", (Join-Path $output "server.ext"), "-out", (Join-Path $output "server.pem"))
$chain = [IO.File]::ReadAllText((Join-Path $output "server.pem")) + [IO.File]::ReadAllText((Join-Path $output "intermediate.pem"))
[IO.File]::WriteAllText((Join-Path $output "server-chain.pem"), $chain, [Text.Encoding]::ASCII)

Write-Output "PKI_READY DIRECTORY=$output PROFILE=RSA2048_SHA256 CHAIN=ROOT_INTERMEDIATE_LEAF SAN=localhost"