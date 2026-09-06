# SPDX-License-Identifier: MPL-2.0
param(
    [ValidateSet("0.4.0")]
    [string]$Version = "0.4.0",
    [string]$OutputDirectory
)

$ErrorActionPreference = "Stop"
$repo = Split-Path -Parent $PSScriptRoot
$dist = Join-Path $repo "dist"
$stageRoot = Join-Path $dist "staging\$Version"
if (-not $OutputDirectory) { $OutputDirectory = Join-Path $dist "packages\$Version" }
$output = [IO.Path]::GetFullPath($OutputDirectory)
$distFull = [IO.Path]::GetFullPath($dist).TrimEnd('\') + '\'
if (-not $output.StartsWith($distFull, [StringComparison]::OrdinalIgnoreCase)) {
    throw "OutputDirectory must be below the repository dist directory: $output"
}

$targets = @(
    "win32-x86-vc6-retrozilla-nss",
    "win32-x64-msvc-19.51-schannel",
    "win32-x64-msvc-19.51-openssl3",
    "win32-x64-msvc-19.51-schannel-openssl3"
)
$packages = @(
    @{ Name = "papinho-secure-transport-$Version-src.zip"; Stage = (Join-Path $stageRoot "source") },
    @{ Name = "papinho-secure-transport-$Version-$($targets[0]).zip"; Stage = (Join-Path $stageRoot $targets[0]) },
    @{ Name = "papinho-secure-transport-$Version-$($targets[1]).zip"; Stage = (Join-Path $stageRoot $targets[1]) },
    @{ Name = "papinho-secure-transport-$Version-$($targets[2]).zip"; Stage = (Join-Path $stageRoot $targets[2]) },
    @{ Name = "papinho-secure-transport-$Version-$($targets[3]).zip"; Stage = (Join-Path $stageRoot $targets[3]) }
)

function Require-File($Root, $RelativePath) {
    $path = Join-Path $Root $RelativePath
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { throw "Required package input is missing: $path" }
}

function New-DeterministicZip($SourceDirectory, $DestinationPath) {
    Add-Type -AssemblyName System.IO.Compression
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $source = [IO.Path]::GetFullPath($SourceDirectory).TrimEnd('\')
    $stream = [IO.File]::Open($DestinationPath, [IO.FileMode]::CreateNew)
    try {
        $archive = New-Object IO.Compression.ZipArchive($stream, [IO.Compression.ZipArchiveMode]::Create, $false)
        try {
            Get-ChildItem -LiteralPath $source -Recurse -File | ForEach-Object {
                [PSCustomObject]@{ File = $_; Relative = $_.FullName.Substring($source.Length + 1).Replace('\', '/') }
            } | Sort-Object Relative | ForEach-Object {
                if ([IO.Path]::IsPathRooted($_.Relative) -or $_.Relative -match '(^|/)\.\.(/|$)' -or -not $_.Relative) { throw "Unsafe ZIP entry path: $($_.Relative)" }
                $entry = $archive.CreateEntry($_.Relative, [IO.Compression.CompressionLevel]::Optimal)
                $entry.LastWriteTime = New-Object DateTimeOffset(2000, 1, 1, 0, 0, 0, ([TimeSpan]::Zero))
                $input = [IO.File]::OpenRead($_.File.FullName)
                try { $destination = $entry.Open(); try { $input.CopyTo($destination) } finally { $destination.Dispose() } } finally { $input.Dispose() }
            }
        } finally { $archive.Dispose() }
    } finally { $stream.Dispose() }
}

& (Join-Path $PSScriptRoot "stage-release-source.ps1") -Clean
& (Join-Path $PSScriptRoot "stage-release-sdk.ps1") -Target all -Clean

Require-File $packages[0].Stage "LICENSE"
Require-File $packages[0].Stage "THIRD_PARTY_NOTICES.md"
Require-File $packages[0].Stage "SHA256SUMS.txt"
Require-File $packages[0].Stage "third_party\retrozilla-nss\source\retrozilla-2f274574d3c6ee8769914046920d649bbae9f81b-patched.zip"
Require-File $packages[0].Stage "third_party\retrozilla-nss\patches\0001-win32-secure-rng-fail-closed-nt4.patch"
foreach ($package in $packages | Select-Object -Skip 1) {
    foreach ($required in @("LICENSE", "THIRD_PARTY_NOTICES.md", "VERSION", "manifest.ini", "consumer-link.ini", "SHA256SUMS.txt", "include\papinho_secure_transport.h", "include\papinho_secure_transport_win32.h")) { Require-File $package.Stage $required }
    if (Test-Path -LiteralPath (Join-Path $package.Stage "include\pst_backend.h")) { throw "Private SPI leaked into SDK: $($package.Stage)" }
}

if (Test-Path -LiteralPath $output) { Remove-Item -LiteralPath $output -Recurse -Force }
New-Item -ItemType Directory -Path $output -Force | Out-Null
foreach ($package in $packages) { $destination = Join-Path $output $package.Name; New-DeterministicZip $package.Stage $destination; Write-Output "PACKAGE=$($package.Name)" }
$produced = Get-ChildItem -LiteralPath $output -Filter "*.zip" -File | Sort-Object Name
if ($produced.Count -ne 5) { throw "Expected exactly five ZIP packages; found $($produced.Count)" }
$expectedNames = $packages.Name | Sort-Object
if (Compare-Object $expectedNames $produced.Name) { throw "Unexpected package name in output" }
$sumLines = $produced | ForEach-Object { "{0}  {1}" -f (Get-FileHash -Algorithm SHA256 -LiteralPath $_.FullName).Hash.ToLowerInvariant(), $_.Name }
[IO.File]::WriteAllText((Join-Path $output "SHA256SUMS-packages.txt"), (($sumLines -join "`n") + "`n"), (New-Object Text.UTF8Encoding($false)))
Write-Output "PACKAGE_COUNT=5"
Write-Output "CHECKSUM=SHA256SUMS-packages.txt"
Write-Output "RESULT=PASS"
