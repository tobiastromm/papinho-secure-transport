# SPDX-License-Identifier: MPL-2.0
param([ValidateSet("ml", "md")][string]$ActualCrt = "ml")

$ErrorActionPreference = "Stop"
$repo = Split-Path -Parent $PSScriptRoot
. (Join-Path $PSScriptRoot "package-staging-policy.ps1")
$id = "win32-x86-vc6-retrozilla-nss-$ActualCrt"
$falseCrt = if ($ActualCrt -eq "ml") { "md" } else { "ml" }
$falseId = "win32-x86-vc6-retrozilla-nss-$falseCrt"
$stage = Join-Path $repo "dist\staging\0.6.2\$id"
$fixture = Join-Path $repo ("build\vc6-crt-negative-" + [guid]::NewGuid().ToString("N"))
$buildRoot = [IO.Path]::GetFullPath((Join-Path $repo "build")).TrimEnd('\') + '\'
if (-not ([IO.Path]::GetFullPath($fixture)).StartsWith($buildRoot, [StringComparison]::OrdinalIgnoreCase)) { throw "Unsafe fixture path" }
try {
    $libraryDestination = Join-Path $fixture "lib\$falseId"
    New-Item -ItemType Directory -Path $libraryDestination -Force | Out-Null
    Copy-Item -LiteralPath (Join-Path $stage "lib\$id\papinho_secure_transport.lib") -Destination $libraryDestination
    $manifest = [IO.File]::ReadAllText((Join-Path $stage "manifest.ini"))
    $tampered = [regex]::Replace($manifest, '(?m)^crt=(ml|md)$', "crt=$falseCrt")
    $tampered = [regex]::Replace($tampered, '(?m)^target_id=win32-x86-vc6-retrozilla-nss-(ml|md)$', "target_id=$falseId")
    Write-PstUtf8Lf (Join-Path $fixture "manifest.ini") $tampered
    try { Assert-PstVc6PackageBinary $fixture } catch {
        if ($_.Exception.Message -notmatch 'CRT metadata') { throw }
        Write-Output "CRT_METADATA_BINARY_MISMATCH_REJECTED=PASS ACTUAL=$ActualCrt FALSE=$falseCrt"
        exit 0
    }
    throw "CRT metadata/binary mismatch was accepted"
} finally {
    if (Test-Path -LiteralPath $fixture) { Remove-Item -LiteralPath $fixture -Recurse -Force }
}
