# SPDX-License-Identifier: MPL-2.0
param(
    [switch]$Clean
)

$ErrorActionPreference = "Stop"
$repo = Split-Path -Parent $PSScriptRoot
$version = "0.4.0"
$stage = Join-Path $repo "dist\staging\$version\source"

function Copy-RequiredFile($RelativePath) {
    $source = Join-Path $repo $RelativePath
    $destination = Join-Path $stage $RelativePath
    if (-not (Test-Path -LiteralPath $source -PathType Leaf)) { throw "Required source-package input is missing: $RelativePath" }
    $parent = Split-Path -Parent $destination
    if (-not (Test-Path -LiteralPath $parent)) { New-Item -ItemType Directory -Path $parent -Force | Out-Null }
    Copy-Item -LiteralPath $source -Destination $destination -Force
}

function Copy-RequiredTree($RelativePath) {
    $source = Join-Path $repo $RelativePath
    if (-not (Test-Path -LiteralPath $source -PathType Container)) { throw "Required source-package tree is missing: $RelativePath" }
    Get-ChildItem -LiteralPath $source -Recurse -File | ForEach-Object {
        $relative = $_.FullName.Substring($repo.Length + 1)
        Copy-RequiredFile $relative
    }
}

if ($Clean -and (Test-Path -LiteralPath $stage)) { Remove-Item -LiteralPath $stage -Recurse -Force }
if (Test-Path -LiteralPath $stage) { Remove-Item -LiteralPath $stage -Recurse -Force }
New-Item -ItemType Directory -Path $stage -Force | Out-Null

foreach ($file in @("README.md", "LICENSE", "THIRD_PARTY_NOTICES.md", "Makefile.vc6", "Makefile.msvc", "Makefile.openssl.msvc", "Makefile.combined.msvc")) {
    Copy-RequiredFile $file
}
foreach ($tree in @("include", "src", "tests", "examples", "tools", "packaging", "third_party")) {
    Copy-RequiredTree $tree
}
Get-ChildItem -LiteralPath (Join-Path $repo "docs") -Recurse -File | Where-Object {
    $_.FullName -notlike (Join-Path $repo "docs\codex\*")
} | ForEach-Object {
    Copy-RequiredFile $_.FullName.Substring($repo.Length + 1)
}

$required = @(
    "include\papinho_secure_transport.h",
    "src\pst_runtime.c",
    "tools\build-vc6.bat",
    "tools\build-modern-msvc.bat",
    "third_party\retrozilla-nss\source\retrozilla-2f274574d3c6ee8769914046920d649bbae9f81b-patched.zip",
    "third_party\retrozilla-nss\patches\0001-win32-secure-rng-fail-closed-nt4.patch",
    "third_party\retrozilla-nss\PROVENANCE.md",
    "third_party\openssl\source\openssl-3.5.8.tar.gz",
    "third_party\openssl\PROVENANCE.md",
    "THIRD_PARTY_NOTICES.md"
)
foreach ($relative in $required) {
    if (-not (Test-Path -LiteralPath (Join-Path $stage $relative) -PathType Leaf)) { throw "Source-package verification failed: $relative" }
}
if (Test-Path -LiteralPath (Join-Path $stage "docs\codex")) { throw "Internal docs leaked into source package" }
foreach ($excluded in @(".git", "build", "dist\staging", ".vs", ".vscode")) {
    if (Test-Path -LiteralPath (Join-Path $stage $excluded)) { throw "Excluded path leaked into source package: $excluded" }
}

$licenseStatus = "present"
[IO.File]::WriteAllText((Join-Path $stage "SOURCE-PACKAGE-STATUS.txt"), "package_version=0.4.0`nlicense=$licenseStatus`npolicy=allowlist`ninternal_docs=excluded`n", (New-Object Text.UTF8Encoding($false)))
$hashLines = Get-ChildItem $stage -File -Recurse | Where-Object { $_.Name -ne "SHA256SUMS.txt" } | Sort-Object FullName | ForEach-Object {
    $relative = $_.FullName.Substring($stage.Length + 1).Replace("\", "/")
    "{0}  {1}" -f (Get-FileHash -Algorithm SHA256 -LiteralPath $_.FullName).Hash.ToLowerInvariant(), $relative
}
[IO.File]::WriteAllText((Join-Path $stage "SHA256SUMS.txt"), (($hashLines -join "`n") + "`n"), (New-Object Text.UTF8Encoding($false)))
Write-Host "STAGED source $stage LICENSE=$licenseStatus"