# SPDX-License-Identifier: MPL-2.0
param(
    [ValidateSet("0.6.4")]
    [string]$Version = "0.6.4",
    [string]$PackageDirectory = "dist\packages\0.6.4",
    [Alias("OutputRoot")]
    [string]$Destination = "build\pst-0.6.4-idle-close"
)

$ErrorActionPreference = "Stop"
$repo = Split-Path -Parent $PSScriptRoot
if (-not [IO.Path]::IsPathRooted($PackageDirectory)) { $PackageDirectory = Join-Path $repo $PackageDirectory }
if (-not [IO.Path]::IsPathRooted($Destination)) { $Destination = Join-Path $repo $Destination }
$packages = [IO.Path]::GetFullPath($PackageDirectory)
$root = [IO.Path]::GetFullPath($Destination)
$work = Join-Path $root "package-build"
$fixtures = Join-Path $repo "build\fixtures\interoperability-pki"
$sourceZip = Join-Path $packages "papinho-secure-transport-0.6.4-src.zip"
$variantZips = @{
    "ml" = Join-Path $packages "papinho-secure-transport-0.6.4-win32-x86-vc6-retrozilla-nss-ml.zip"
    "md" = Join-Path $packages "papinho-secure-transport-0.6.4-win32-x86-vc6-retrozilla-nss-md.zip"
}

foreach ($path in @($sourceZip, $variantZips.ml, $variantZips.md,
        (Join-Path $fixtures "root.der"), (Join-Path $fixtures "server-chain.pem"))) {
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { throw "Missing bundle input: $path" }
}
if (Test-Path -LiteralPath $root) { Remove-Item -LiteralPath $root -Recurse -Force }
New-Item -ItemType Directory -Path $work -Force | Out-Null
Add-Type -AssemblyName System.IO.Compression.FileSystem
$source = Join-Path $work "source"
[IO.Compression.ZipFile]::ExtractToDirectory($sourceZip, $source)

function Write-AsciiLines([string]$Path, [string[]]$Lines) {
    [IO.File]::WriteAllLines($Path, $Lines, [Text.Encoding]::ASCII)
}

function Set-DeterministicPeTimestamp([string]$Path) {
    $bytes = [IO.File]::ReadAllBytes($Path)
    if ($bytes.Length -lt 64) { throw "Invalid PE image: $Path" }
    $peOffset = [BitConverter]::ToInt32($bytes, 0x3c)
    if ($peOffset -lt 0 -or $peOffset + 12 -gt $bytes.Length -or
        $bytes[$peOffset] -ne 0x50 -or $bytes[$peOffset + 1] -ne 0x45 -or
        $bytes[$peOffset + 2] -ne 0 -or $bytes[$peOffset + 3] -ne 0) {
        throw "Invalid PE signature: $Path"
    }
    for ($i = 0; $i -lt 4; $i++) { $bytes[$peOffset + 8 + $i] = 0 }
    [IO.File]::WriteAllBytes($Path, $bytes)
}

function Build-Variant([string]$Variant) {
    $sdk = Join-Path $work ("sdk-" + $Variant)
    [IO.Compression.ZipFile]::ExtractToDirectory($variantZips[$Variant], $sdk)
    $target = "win32-x86-vc6-retrozilla-nss-" + $Variant
    $output = Join-Path $work ("bin-" + $Variant)
    New-Item -ItemType Directory -Path $output -Force | Out-Null
    $crt = if ($Variant -eq "ml") { "/ML" } else { "/MD" }
    $nss = Join-Path $source "third_party\retrozilla-nss\prebuilt\win32-x86-vc6\sdk"
    $command = 'call "' + (Join-Path $source 'tools\vc6-env.bat') + '" >nul && cl /nologo /W4 /O2 /TC ' + $crt +
        ' /I"' + (Join-Path $sdk 'include') + '" /I"' + (Join-Path $source 'src') +
        '" /I"' + (Join-Path $nss 'include\nspr') + '" /I"' + (Join-Path $nss 'public\nss') +
        '" /Fo"' + (Join-Path $output 'idle-close.obj') + '" /Fe"' + (Join-Path $output 'idle-close.exe') +
        '" "' + (Join-Path $source 'tests\test_connection_failures.c') + '" /link /LIBPATH:"' +
        (Join-Path $sdk ("lib\" + $target)) + '" papinho_secure_transport.lib wsock32.lib'
    $compileOutput = @(cmd.exe /d /c $command 2>&1)
    $compileOutput | ForEach-Object { Write-Host $_ }
    if ($LASTEXITCODE -ne 0) { throw "Package-only idle-close harness build failed for $Variant" }
    Set-DeterministicPeTimestamp (Join-Path $output "idle-close.exe")
    Remove-Item -LiteralPath (Join-Path $output "idle-close.obj") -Force
    Get-ChildItem -LiteralPath (Join-Path $sdk ("runtime\" + $target)) -File |
        Copy-Item -Destination $output
    return $output
}

$built = @{ "ml" = Build-Variant "ml"; "md" = Build-Variant "md" }
$nt4 = Join-Path $root "nt4"
$clean = Join-Path $root "clean-machine"
New-Item -ItemType Directory -Path $nt4, $clean -Force | Out-Null

function Populate-Bundle([string]$Bundle) {
    Copy-Item -LiteralPath $sourceZip, $variantZips.ml, $variantZips.md -Destination $Bundle
    Copy-Item -LiteralPath (Join-Path $source "tests\connection_failure_server.py") -Destination $Bundle
    $pem = Join-Path $Bundle "fixtures"
    New-Item -ItemType Directory -Path $pem -Force | Out-Null
    foreach ($name in @("root.pem", "client-chain.pem", "client.key", "server-chain.pem", "server.key")) {
        Copy-Item -LiteralPath (Join-Path $fixtures $name) -Destination $pem
    }
    foreach ($variant in @("ml", "md")) {
        $dir = Join-Path $Bundle $variant
        New-Item -ItemType Directory -Path $dir -Force | Out-Null
        Get-ChildItem -LiteralPath $built[$variant] -File | Copy-Item -Destination $dir
        foreach ($name in @("root.der", "client.der", "client.pk8")) {
            Copy-Item -LiteralPath (Join-Path $fixtures $name) -Destination $dir
        }
        Copy-Item -LiteralPath (Join-Path $fixtures "root.der") -Destination (Join-Path $dir "ca.der")
        foreach ($mode in @("healthy_idle", "clean_close", "idle_reset")) {
            $label = $mode.Replace("_", "-")
            Write-AsciiLines (Join-Path $dir ("run-" + $label + ".bat")) @(
                "@echo off", 'if "%2"=="" goto usage',
                "set PST_NSS_TRACE_FILE=$mode-backend.log",
                "idle-close.exe %1 %2 localhost ca.der client.der client.pk8 13 fixture/1 $mode 0",
                "if errorlevel 1 goto fail", "echo PST 0.6.4 $($variant.ToUpperInvariant()) $mode PASS", "goto end",
                ":usage", "echo Usage: run-$label.bat HOST PORT", ":fail", "echo PST 0.6.4 $($variant.ToUpperInvariant()) $mode FAIL", "verify other 2>nul", ":end"
            )
        }
    }
    $readme = @(
        "PapinhoSecureTransport 0.6.4 targeted idle-close validation", "",
        "Validate TRANSFER-SHA256SUMS.txt before running any executable.",
        "Each ML/MD executable was compiled against the extracted canonical 0.6.4 SDK library.",
        "Do not install or replace MSVCRT.DLL and do not copy runtime DLLs between variants.", "",
        "Start the modern fixture first from this bundle root:",
        "  python connection_failure_server.py 0.0.0.0 PORT MODE fixtures/server-chain.pem fixtures/server.key fixtures/root.pem 13 fixture/1 required", "",
        "Use distinct ports and then run on the target:",
        "  ML healthy: fixture MODE=healthy_idle PORT=8640; ml\run-healthy-idle.bat HOST 8640",
        "  ML clean:   fixture MODE=clean_close  PORT=8641; ml\run-clean-close.bat HOST 8641",
        "  ML reset:   fixture MODE=idle_reset   PORT=8642; ml\run-idle-reset.bat HOST 8642",
        "  MD healthy: fixture MODE=healthy_idle PORT=8650; md\run-healthy-idle.bat HOST 8650",
        "  MD clean:   fixture MODE=clean_close  PORT=8651; md\run-clean-close.bat HOST 8651",
        "  MD reset:   fixture MODE=idle_reset   PORT=8652; md\run-idle-reset.bat HOST 8652", "",
        "Expected healthy markers: HEALTHY_IDLE APP_BYTES=0 FINAL_RESULT=WAIT_TIMEOUT PASS=1, then EXPECT_READY=1 PASS=1 and final CLOSED/CLEAN.",
        "Expected clean marker: EXPECT_READY=1 PASS=1 and final CLOSED/CLEAN.",
        "Expected reset marker: EXPECT_READY=1 PASS=1 and final TRUNCATED.",
        "Return all console output, fixture output, and *-backend.log files."
    )
    Write-AsciiLines (Join-Path $Bundle "README.txt") $readme
    $lines = Get-ChildItem -LiteralPath $Bundle -File -Recurse | Where-Object Name -ne "TRANSFER-SHA256SUMS.txt" |
        Sort-Object FullName | ForEach-Object {
            "{0}  {1}" -f (Get-FileHash -Algorithm SHA256 -LiteralPath $_.FullName).Hash.ToLowerInvariant(),
                $_.FullName.Substring($Bundle.Length + 1).Replace('\', '/')
        }
    [IO.File]::WriteAllText((Join-Path $Bundle "TRANSFER-SHA256SUMS.txt"), (($lines -join "`n") + "`n"), (New-Object Text.UTF8Encoding($false)))
}

Populate-Bundle $nt4
Populate-Bundle $clean

function New-FixedZip([string]$SourceDirectory, [string]$ZipPath) {
    Add-Type -AssemblyName System.IO.Compression
    $stream = [IO.File]::Open($ZipPath, [IO.FileMode]::Create)
    try {
        $archive = New-Object IO.Compression.ZipArchive($stream, [IO.Compression.ZipArchiveMode]::Create, $false)
        try {
            Get-ChildItem -LiteralPath $SourceDirectory -File -Recurse | ForEach-Object {
                [PSCustomObject]@{ File = $_; Relative = $_.FullName.Substring($SourceDirectory.Length + 1).Replace('\', '/') }
            } | Sort-Object Relative | ForEach-Object {
                $entry = $archive.CreateEntry($_.Relative, [IO.Compression.CompressionLevel]::Optimal)
                $entry.LastWriteTime = New-Object DateTimeOffset(2000, 1, 1, 0, 0, 0, ([TimeSpan]::Zero))
                $input = [IO.File]::OpenRead($_.File.FullName)
                try { $target = $entry.Open(); try { $input.CopyTo($target) } finally { $target.Dispose() } } finally { $input.Dispose() }
            }
        } finally { $archive.Dispose() }
    } finally { $stream.Dispose() }
}

$nt4Zip = Join-Path $root "pst-0.6.4-idle-close-nt4.zip"
$cleanZip = Join-Path $root "pst-0.6.4-idle-close-clean-machine.zip"
New-FixedZip $nt4 $nt4Zip
New-FixedZip $clean $cleanZip
Write-Output "NT4_BUNDLE_PATH=$nt4Zip"
Write-Output "NT4_BUNDLE_SHA256=$((Get-FileHash $nt4Zip).Hash.ToLowerInvariant())"
Write-Output "NT4_TRANSFER_MANIFEST_SHA256=$((Get-FileHash (Join-Path $nt4 'TRANSFER-SHA256SUMS.txt')).Hash.ToLowerInvariant())"
Write-Output "NT4_TRANSFER_MANIFEST_ENTRIES=$((Get-Content (Join-Path $nt4 'TRANSFER-SHA256SUMS.txt')).Count)"
Write-Output "CLEAN_BUNDLE_PATH=$cleanZip"
Write-Output "CLEAN_BUNDLE_SHA256=$((Get-FileHash $cleanZip).Hash.ToLowerInvariant())"
Write-Output "CLEAN_TRANSFER_MANIFEST_SHA256=$((Get-FileHash (Join-Path $clean 'TRANSFER-SHA256SUMS.txt')).Hash.ToLowerInvariant())"
Write-Output "CLEAN_TRANSFER_MANIFEST_ENTRIES=$((Get-Content (Join-Path $clean 'TRANSFER-SHA256SUMS.txt')).Count)"
