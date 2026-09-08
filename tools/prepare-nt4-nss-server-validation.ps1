# SPDX-License-Identifier: MPL-2.0
$ErrorActionPreference = "Stop"
$repo = Split-Path -Parent $PSScriptRoot
$build = Join-Path $repo "build\win32-x86-vc6-retrozilla-nss"
$runtime = Join-Path $repo "third_party\retrozilla-nss\prebuilt\win32-x86-vc6\runtime"
$fixtures = Join-Path $repo "build\fixtures\interoperability-pki"
$destination = Join-Path $repo "build\nt4-nss-server-validation"

if (-not (Test-Path (Join-Path $build "test_nss_server_direct.exe"))) {
    throw "Missing test_nss_server_direct.exe; run tools\build-vc6.bat nss-server-direct"
}
if (Test-Path $destination) { Remove-Item -Recurse -Force -LiteralPath $destination }
New-Item -ItemType Directory -Path $destination | Out-Null

Copy-Item (Join-Path $build "test_nss_server_direct.exe") $destination
Copy-Item (Join-Path $runtime "*") $destination
foreach ($name in @("server.der", "intermediate.der", "server.pk8", "root.der")) {
    Copy-Item (Join-Path $fixtures $name) $destination
}

$runners = @{
    "run_tls12.bat" = @(
        "@echo off", 'if "%1"=="" goto usage', "set PST_NSS_TRACE_FILE=tls12-backend.log",
        "test_nss_server_direct.exe %1 12 0 server.der intermediate.der server.pk8 root.der",
        "if errorlevel 1 goto fail", "echo PAPINHOSECURETRANSPORT SS-5 NT4 NSS SERVER TLS 1.2 PASS", "goto end",
        ":usage", "echo Usage: run_tls12.bat PORT", "goto fail_end", ":fail",
        "echo PAPINHOSECURETRANSPORT SS-5 NT4 NSS SERVER TLS 1.2 FAIL", ":fail_end", "verify other 2>nul", ":end")
    "run_tls13_mtls.bat" = @(
        "@echo off", 'if "%1"=="" goto usage', "set PST_NSS_TRACE_FILE=tls13-mtls-backend.log",
        "test_nss_server_direct.exe %1 13 2 server.der intermediate.der server.pk8 root.der",
        "if errorlevel 1 goto fail", "echo PAPINHOSECURETRANSPORT SS-5 NT4 NSS SERVER TLS 1.3 MTLS PASS", "goto end",
        ":usage", "echo Usage: run_tls13_mtls.bat PORT", "goto fail_end", ":fail",
        "echo PAPINHOSECURETRANSPORT SS-5 NT4 NSS SERVER TLS 1.3 MTLS FAIL", ":fail_end", "verify other 2>nul", ":end")
    "run_truncation.bat" = @(
        "@echo off", 'if "%1"=="" goto usage', "set PST_NSS_TRACE_FILE=truncation-backend.log",
        "test_nss_server_direct.exe %1 12 0 server.der intermediate.der server.pk8 root.der 0 0 0 - - raw-abrupt",
        "if errorlevel 1 goto fail", "echo PAPINHOSECURETRANSPORT SS-5 NT4 NSS SERVER TRUNCATION PASS", "goto end",
        ":usage", "echo Usage: run_truncation.bat PORT", "goto fail_end", ":fail",
        "echo PAPINHOSECURETRANSPORT SS-5 NT4 NSS SERVER TRUNCATION FAIL", ":fail_end", "verify other 2>nul", ":end")
}
foreach ($entry in $runners.GetEnumerator()) {
    [IO.File]::WriteAllLines((Join-Path $destination $entry.Key), $entry.Value,
        [Text.Encoding]::ASCII)
}

$readme = @(
    "PAPINHOSECURETRANSPORT SS-5 NT4 NSS SERVER VALIDATION",
    "=====================================================",
    "Test-only package. It is not a release package.",
    "Run one server runner at a time on Windows NT 4.0 SP6 x86.",
    "The modern OpenSSL client commands are documented in docs/backend-nss.md.",
    "Return console output and every *-backend.log file.",
    "Prepared evidence is not PASS until executed on real NT4."
)
[IO.File]::WriteAllLines((Join-Path $destination "README.txt"), $readme,
    [Text.Encoding]::ASCII)

Get-ChildItem -File $destination | Sort-Object Name |
    Get-FileHash -Algorithm SHA256 | ForEach-Object {
        "{0} *{1}" -f $_.Hash.ToLowerInvariant(), (Split-Path -Leaf $_.Path)
    } | Set-Content -Encoding ASCII (Join-Path $destination "MANIFEST.sha256")

Write-Output "NT4_NSS_SERVER_PACKAGE=$destination"
Write-Output "NT4_NSS_SERVER_EXECUTION=PENDING_REAL_NT4"
