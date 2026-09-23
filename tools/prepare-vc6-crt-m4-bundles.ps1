# SPDX-License-Identifier: MPL-2.0
param([string]$Destination = "build\vc6-crt-m4")
$ErrorActionPreference = "Stop"
$repo = Split-Path -Parent $PSScriptRoot
if (-not [IO.Path]::IsPathRooted($Destination)) { $Destination = Join-Path $repo $Destination }
$root = [IO.Path]::GetFullPath($Destination)
$repro = Join-Path $repo "dist\reproduction\0.6.2\run1"
$m3 = Join-Path $repo "build\m3-vc6-crt"
$fixtures = Join-Path $repo "build\fixtures\interoperability-pki"
$externalBuild = Join-Path $repo "build\vc6-crt-m4-external-server"
$expected = @{
  "papinho-secure-transport-0.6.2-src.zip" = "906f4c131782c359f6956bde36e8b15f2d5b765611c5a24801f8d4278a0e7558"
  "papinho-secure-transport-0.6.2-win32-x86-vc6-retrozilla-nss-ml.zip" = "a7e1f3aa9ae9aa9e017dadf1c1d8336e7c11b3f7521a9611ef17818f3c2f7249"
  "papinho-secure-transport-0.6.2-win32-x86-vc6-retrozilla-nss-md.zip" = "b3270ccbf3cbe293fef68672d53f4bc7b2ed7eaa7ba71a648006272856e740f8"
}
foreach ($name in $expected.Keys) {
  $actual = (Get-FileHash -Algorithm SHA256 -LiteralPath (Join-Path $repro $name)).Hash.ToLowerInvariant()
  if ($actual -ne $expected[$name]) { throw "M2 input hash mismatch: $name" }
}
foreach ($variant in @("ml", "md")) {
  foreach ($exe in @("client.exe", "contract.exe")) {
    if (-not (Test-Path -LiteralPath (Join-Path $m3 "$variant\$exe") -PathType Leaf)) {
      throw "Run the M3 package-only build first; missing $variant/$exe"
    }
  }
}
if (Test-Path -LiteralPath $externalBuild) { Remove-Item -LiteralPath $externalBuild -Recurse -Force }
New-Item -ItemType Directory -Path $externalBuild | Out-Null
$serverSource = [IO.File]::ReadAllText((Join-Path $repo "dist\validation\0.6.2-m2\source\tests\test_openssl_server_integration.c"))
$serverSource = $serverSource.Replace('#include "backends/openssl/pst_backend_openssl.h"', '')
$serverSource = $serverSource.Replace('pst_backend_registry_reset();', '')
$serverSource = $serverSource.Replace('cc.provider_selection.exact_provider_id="openssl"', 'cc.provider_selection.exact_provider_id="retrozilla-nss"')
$serverSource = $serverSource.Replace('htonl(INADDR_LOOPBACK)', 'htonl(INADDR_ANY)')
foreach ($variant in @("ml", "md")) {
  $variantBuild = Join-Path $externalBuild $variant
  New-Item -ItemType Directory -Path $variantBuild | Out-Null
  $generatedSource = Join-Path $variantBuild "nss_external_server.c"
  [IO.File]::WriteAllText($generatedSource, $serverSource, [Text.Encoding]::ASCII)
  $sdk = Join-Path $repo ("dist\validation\0.6.2-m2\win32-x86-vc6-retrozilla-nss-" + $variant)
  $target = "win32-x86-vc6-retrozilla-nss-" + $variant
  $crt = if ($variant -eq "ml") { "/ML" } else { "/MD" }
  $command = 'call "' + (Join-Path $repo 'tools\vc6-env.bat') + '" >nul && cl /nologo /W4 /O2 /TC ' + $crt +
    ' /I"' + (Join-Path $sdk 'include') + '" /Fo"' + (Join-Path $variantBuild 'server.obj') +
    '" /Fe"' + (Join-Path $variantBuild 'server.exe') + '" "' + $generatedSource +
    '" /link /LIBPATH:"' + (Join-Path $sdk ('lib\' + $target)) + '" papinho_secure_transport.lib wsock32.lib'
  cmd.exe /d /c $command
  if ($LASTEXITCODE -ne 0) { throw "external NSS SERVER build failed for $variant" }
}
if (Test-Path -LiteralPath $root) { Remove-Item -LiteralPath $root -Recurse -Force }
$nt4 = Join-Path $root "nt4"
$clean = Join-Path $root "clean-machine"
New-Item -ItemType Directory -Path $nt4, $clean | Out-Null

function Write-AsciiLines([string]$Path, [string[]]$Lines) {
  [IO.File]::WriteAllLines($Path, $Lines, [Text.Encoding]::ASCII)
}
function Copy-Variant([string]$Destination, [string]$Variant) {
  $d = Join-Path $Destination $Variant
  New-Item -ItemType Directory -Path $d | Out-Null
  Copy-Item -LiteralPath (Join-Path $m3 "$Variant\client.exe") -Destination $d
  Copy-Item -LiteralPath (Join-Path $externalBuild "$Variant\server.exe") -Destination $d
  Copy-Item -LiteralPath (Join-Path $m3 "$Variant\contract.exe") -Destination $d
  Get-ChildItem -LiteralPath (Join-Path $m3 $Variant) -File |
    Where-Object { $_.Extension -in @(".dll", ".chk") } | Copy-Item -Destination $d
  foreach ($name in @("root.der", "client.der", "client.pk8", "server.der", "intermediate.der", "server.pk8")) {
    Copy-Item -LiteralPath (Join-Path $fixtures $name) -Destination $d
  }
  Copy-Item -LiteralPath (Join-Path $fixtures "root.der") -Destination (Join-Path $d "ca.der")
  Write-AsciiLines (Join-Path $d "run_contract.bat") @("@echo off", "contract.exe")
  Write-AsciiLines (Join-Path $d "run_client_tls12.bat") @("@echo off", 'if "%3"=="" goto usage', "set PST_NSS_TRACE_FILE=client-tls12-backend.log", "client.exe %1 %2 %3 ca.der client.der client.pk8 12 12 fixture/1 required 5", "goto end", ":usage", "echo Usage: run_client_tls12.bat HOST PORT HOSTNAME", "verify other 2>nul", ":end")
  Write-AsciiLines (Join-Path $d "run_client_tls13.bat") @("@echo off", 'if "%3"=="" goto usage', "set PST_NSS_TRACE_FILE=client-tls13-backend.log", "client.exe %1 %2 %3 ca.der client.der client.pk8 13 13 fixture/1 required 5", "goto end", ":usage", "echo Usage: run_client_tls13.bat HOST PORT HOSTNAME", "verify other 2>nul", ":end")
  Write-AsciiLines (Join-Path $d "run_server_tls12.bat") @("@echo off", 'if "%1"=="" goto usage', "set PST_NSS_TRACE_FILE=server-tls12-backend.log", "server.exe %1 12 2 server.der intermediate.der server.pk8 root.der exact - echo", "goto end", ":usage", "echo Usage: run_server_tls12.bat PORT", "verify other 2>nul", ":end")
  Write-AsciiLines (Join-Path $d "run_server_tls13_mtls.bat") @("@echo off", 'if "%1"=="" goto usage', "set PST_NSS_TRACE_FILE=server-tls13-backend.log", "echo NSS_SERVER_ALPN_ADVERTISED=NO", "server.exe %1 13 2 server.der intermediate.der server.pk8 root.der exact - echo", "goto end", ":usage", "echo Usage: run_server_tls13_mtls.bat PORT", "verify other 2>nul", ":end")
  Write-AsciiLines (Join-Path $d "run_truncation.bat") @("@echo off", 'if "%1"=="" goto usage', "set PST_NSS_TRACE_FILE=truncation-backend.log", "server.exe %1 12 2 server.der intermediate.der server.pk8 root.der exact - truncate", "goto end", ":usage", "echo Usage: run_truncation.bat PORT", "verify other 2>nul", ":end")
}
foreach ($variant in @("ml", "md")) { Copy-Variant $nt4 $variant; Copy-Variant $clean $variant }

foreach ($bundle in @($nt4, $clean)) {
  foreach ($name in $expected.Keys) { Copy-Item -LiteralPath (Join-Path $repro $name) -Destination $bundle }
  Copy-Item -LiteralPath (Join-Path $repo "tests\nt4_tls_server.py") -Destination $bundle
  Copy-Item -LiteralPath (Join-Path $repo "tests\openssl_server_test_client.py") -Destination $bundle
  $pem = Join-Path $bundle "fixtures"
  New-Item -ItemType Directory -Path $pem | Out-Null
  foreach ($name in @("root.pem", "client-chain.pem", "client.key", "server-chain.pem", "server.key")) {
    Copy-Item -LiteralPath (Join-Path $fixtures $name) -Destination $pem
  }
}

$readme = @(
  "PapinhoSecureTransport VC6 CRT M4 external validation", "",
  "Validate TRANSFER-SHA256SUMS.txt before running anything.",
  "Run every command once from ml and once from md; do not mix their DLLs.",
  "CLIENT fixture on modern host:",
  "  python nt4_tls_server.py 0.0.0.0 PORT fixtures/server-chain.pem fixtures/server.key fixtures/root.pem 12 fixture/1 required localhost",
  "  python nt4_tls_server.py 0.0.0.0 PORT fixtures/server-chain.pem fixtures/server.key fixtures/root.pem 13 fixture/1 required localhost",
  "Client command in target variant directory:",
  "  run_client_tls12.bat MODERN_HOST_IP PORT localhost",
  "  run_client_tls13.bat MODERN_HOST_IP PORT localhost",
  "SERVER commands in target variant directory:",
  "  run_server_tls12.bat PORT",
  "  run_server_tls13_mtls.bat PORT",
  "Modern mTLS client for NSS SERVER (ALPN intentionally disabled):",
  "  python openssl_server_test_client.py TARGET_IP PORT 12 fixtures/root.pem fixtures/client-chain.pem fixtures/client.key - clean -",
  "  python openssl_server_test_client.py TARGET_IP PORT 13 fixtures/root.pem fixtures/client-chain.pem fixtures/client.key - clean -",
  "Truncation: run_truncation.bat PORT, then use data-abrupt instead of clean.",
  "Return console output and every *-backend.log. Do not install DLLs globally."
)
Write-AsciiLines (Join-Path $nt4 "README.txt") $readme
Write-AsciiLines (Join-Path $clean "README.txt") ($readme + @("", "Clean-machine consumer compilation additionally requires an installed VC6 toolchain.", "No /NODEFAULTLIB option is permitted."))

foreach ($bundle in @($nt4, $clean)) {
  $lines = Get-ChildItem -LiteralPath $bundle -File -Recurse |
    Where-Object Name -ne "TRANSFER-SHA256SUMS.txt" | Sort-Object FullName | ForEach-Object {
      "{0}  {1}" -f (Get-FileHash -Algorithm SHA256 -LiteralPath $_.FullName).Hash.ToLowerInvariant(), $_.FullName.Substring($bundle.Length + 1).Replace('\', '/')
    }
  [IO.File]::WriteAllText((Join-Path $bundle "TRANSFER-SHA256SUMS.txt"), (($lines -join "`n") + "`n"), (New-Object Text.UTF8Encoding($false)))
}
Write-Output "M4_NT4_BUNDLE=$nt4"
Write-Output "M4_CLEAN_MACHINE_BUNDLE=$clean"
Write-Output "M4_NT4_MANIFEST_SHA256=$((Get-FileHash (Join-Path $nt4 'TRANSFER-SHA256SUMS.txt')).Hash.ToLowerInvariant())"
Write-Output "M4_CLEAN_MANIFEST_SHA256=$((Get-FileHash (Join-Path $clean 'TRANSFER-SHA256SUMS.txt')).Hash.ToLowerInvariant())"
