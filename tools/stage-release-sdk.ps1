# SPDX-License-Identifier: MPL-2.0
param(
    [ValidateSet("all", "windows-nt4-x86-vc6-retrozilla-nss", "windows-x64-msvc-schannel", "windows-x64-msvc-openssl-3.5.8", "windows-x64-msvc-schannel-openssl-3.5.8")]
    [string]$Target = "all",
    [switch]$Clean
)

$ErrorActionPreference = "Stop"
$repo = Split-Path -Parent $PSScriptRoot
$version = "0.4.0"
$root = Join-Path $repo "dist\staging\$version"
$targets = @("windows-nt4-x86-vc6-retrozilla-nss", "windows-x64-msvc-schannel", "windows-x64-msvc-openssl-3.5.8", "windows-x64-msvc-schannel-openssl-3.5.8")
if ($Target -ne "all") { $targets = @($Target) }

function Copy-Required($Source, $Destination) {
    if (-not (Test-Path -LiteralPath $Source -PathType Leaf)) { throw "Required staging input is missing: $Source" }
    $parent = Split-Path -Parent $Destination
    if (-not (Test-Path -LiteralPath $parent)) { New-Item -ItemType Directory -Path $parent -Force | Out-Null }
    Copy-Item -LiteralPath $Source -Destination $Destination -Force
}
function Write-Utf8NoBom($Path, $Text) {
    $parent = Split-Path -Parent $Path
    if (-not (Test-Path -LiteralPath $parent)) { New-Item -ItemType Directory -Path $parent -Force | Out-Null }
    [IO.File]::WriteAllText($Path, $Text, (New-Object Text.UTF8Encoding($false)))
}

foreach ($id in $targets) {
    $stage = Join-Path $root $id
    if (Test-Path -LiteralPath $stage) { Remove-Item -LiteralPath $stage -Recurse -Force }
    New-Item -ItemType Directory -Path $stage -Force | Out-Null
    Copy-Required (Join-Path $repo "packaging\SDK-README.md") (Join-Path $stage "README.md")
    Copy-Required (Join-Path $repo "THIRD_PARTY_NOTICES.md") (Join-Path $stage "THIRD_PARTY_NOTICES.md")
    Copy-Required (Join-Path $repo "LICENSE") (Join-Path $stage "LICENSE")
    foreach ($file in @("papinho_secure_transport.h", "papinho_secure_transport_win32.h")) { Copy-Required (Join-Path $repo "include\$file") (Join-Path $stage "include\$file") }
    foreach ($file in @("release-packaging.md", "release-licensing.md", "consumer-linking.md", "security-and-limitations.md")) { Copy-Required (Join-Path $repo "docs\$file") (Join-Path $stage "docs\$file") }
    Get-ChildItem (Join-Path $repo "examples") -File | ForEach-Object { Copy-Required $_.FullName (Join-Path $stage "examples\$($_.Name)") }

    $runtimeFiles = "none-package-supplied"
    $thirdParty = "none"
    if ($id -eq "windows-nt4-x86-vc6-retrozilla-nss") {
        $build = Join-Path $repo "build\vc6"; $architecture = "x86"; $toolchain = "Visual C++ 6 SP5 plus Processor Pack"; $crt = "compiler-default-static"; $providers = "retrozilla-nss"
        $capabilities = "TLS1.2,TLS1.3,CUSTOM_TRUST,HOSTNAME_VERIFY,ALPN,CLIENT_AUTH,PEER_INFO,NONBLOCKING,BACKEND_WAIT"
        $linkLibraries = "papinho_secure_transport.lib,wsock32.lib"
        $runtime = Join-Path $repo "third_party\retrozilla-nss\prebuilt\win32-x86-vc6\runtime"
        $runtimeList = @("freebl3.chk","freebl3.dll","nspr4.dll","nss3.dll","nssutil3.dll","plc4.dll","plds4.dll","softokn3.chk","softokn3.dll","ssl3.dll")
        foreach ($file in $runtimeList) { Copy-Required (Join-Path $runtime $file) (Join-Path $stage "runtime\$id\$file") }
        foreach ($file in @("RetroZilla-LICENSE.txt","RetroZilla-LEGAL.txt","NSS-MPL-2.0.txt","NSPR-license-evidence.h")) { Copy-Required (Join-Path $repo "third_party\retrozilla-nss\licenses\$file") (Join-Path $stage "licenses\retrozilla-nss\$file") }
        $runtimeFiles = $runtimeList -join ","; $thirdParty = "RetroZilla NSS 3.42 Beta;NSPR 4.7.7"
    } elseif ($id -eq "windows-x64-msvc-schannel") {
        $build = Join-Path $repo "build\win64-modern-msvc"; $architecture = "x64"; $toolchain = "documented MSVC x64"; $crt = "dynamic-/MD"; $providers = "schannel"
        $capabilities = "TLS1.2,CUSTOM_TRUST,SYSTEM_TRUST,HOSTNAME_VERIFY,ALPN,CLIENT_AUTH,PEER_INFO,NONBLOCKING,BACKEND_WAIT"
        $linkLibraries = "papinho_secure_transport.lib,ws2_32.lib,secur32.lib,crypt32.lib,ncrypt.lib"
    } elseif ($id -eq "windows-x64-msvc-openssl-3.5.8") {
        $build = Join-Path $repo "build\win64-modern-msvc-openssl"; $architecture = "x64"; $toolchain = "documented MSVC x64"; $crt = "dynamic-/MD"; $providers = "openssl"
        $capabilities = "TLS1.2,TLS1.3,CUSTOM_TRUST,SYSTEM_TRUST,HOSTNAME_VERIFY,ALPN,CLIENT_AUTH,PEER_INFO,NONBLOCKING,BACKEND_WAIT"
        $linkLibraries = "papinho_secure_transport.lib,libssl.lib,libcrypto.lib,ws2_32.lib,crypt32.lib"
        foreach ($file in @("libssl.lib","libcrypto.lib")) { Copy-Required (Join-Path $repo "third_party\openssl\prebuilt\win64-msvc-3.5.8\lib\$file") (Join-Path $stage "lib\$id\$file") }
        foreach ($file in @("libssl-3-x64.dll","libcrypto-3-x64.dll")) { Copy-Required (Join-Path $repo "third_party\openssl\prebuilt\win64-msvc-3.5.8\runtime\$file") (Join-Path $stage "runtime\$id\$file") }
        Copy-Required (Join-Path $repo "third_party\openssl\LICENSE.txt") (Join-Path $stage "licenses\openssl\LICENSE-APACHE-2.0.txt")
        $runtimeFiles = "libssl-3-x64.dll,libcrypto-3-x64.dll"; $thirdParty = "OpenSSL 3.5.8 LTS"
    } else {
        $build = Join-Path $repo "build\win64-modern-msvc-combined"; $architecture = "x64"; $toolchain = "documented MSVC x64"; $crt = "dynamic-/MD"; $providers = "schannel,openssl"
        $capabilities = "TLS1.2,TLS1.3,CUSTOM_TRUST,SYSTEM_TRUST,HOSTNAME_VERIFY,ALPN,CLIENT_AUTH,PEER_INFO,NONBLOCKING,BACKEND_WAIT"
        $linkLibraries = "papinho_secure_transport.lib,libssl.lib,libcrypto.lib,ws2_32.lib,secur32.lib,crypt32.lib,ncrypt.lib"
        foreach ($file in @("libssl.lib","libcrypto.lib")) { Copy-Required (Join-Path $repo "third_party\openssl\prebuilt\win64-msvc-3.5.8\lib\$file") (Join-Path $stage "lib\$id\$file") }
        foreach ($file in @("libssl-3-x64.dll","libcrypto-3-x64.dll")) { Copy-Required (Join-Path $repo "third_party\openssl\prebuilt\win64-msvc-3.5.8\runtime\$file") (Join-Path $stage "runtime\$id\$file") }
        Copy-Required (Join-Path $repo "third_party\openssl\LICENSE.txt") (Join-Path $stage "licenses\openssl\LICENSE-APACHE-2.0.txt")
        $runtimeFiles = "libssl-3-x64.dll,libcrypto-3-x64.dll"; $thirdParty = "OpenSSL 3.5.8 LTS"
    }
    Copy-Required (Join-Path $build "papinho_secure_transport.lib") (Join-Path $stage "lib\$id\papinho_secure_transport.lib")
    Write-Utf8NoBom (Join-Path $stage "VERSION") "package_version=0.4.0`nlibrary_version=0.4.0`napi_version=1.3.0`nspi_version=2.4`n"
    Write-Utf8NoBom (Join-Path $stage "consumer-link.ini") "target_id=$id`nlink_libraries=$linkLibraries`nruntime_files=$runtimeFiles`n"
    Write-Utf8NoBom (Join-Path $stage "manifest.ini") "format_version=1`npackage_name=PapinhoSecureTransport`npackage_version=0.4.0`nlibrary_version=0.4.0`napi_version=1.3.0`nspi_version=2.4`ntarget_id=$id`narchitecture=$architecture`ntoolchain=$toolchain`ncrt=$crt`nlinkage=static`nprovider_ids=$providers`ncapabilities=$capabilities`nruntime_files=$runtimeFiles`nthird_party_components=$thirdParty`nlicense_id=MPL-2.0`nsource_package=papinho-secure-transport-0.4.0-src.zip`nlicense_file=LICENSE`nprovenance_reference=docs/release-packaging.md`nthird_party_notice=THIRD_PARTY_NOTICES.md`n"
    $hashLines = Get-ChildItem $stage -File -Recurse | Where-Object { $_.Name -ne "SHA256SUMS.txt" } | Sort-Object FullName | ForEach-Object { $relative = $_.FullName.Substring($stage.Length + 1).Replace("\", "/"); "{0}  {1}" -f (Get-FileHash -Algorithm SHA256 -LiteralPath $_.FullName).Hash.ToLowerInvariant(), $relative }
    Write-Utf8NoBom (Join-Path $stage "SHA256SUMS.txt") (($hashLines -join "`n") + "`n")
    Write-Host "STAGED $id"
}
