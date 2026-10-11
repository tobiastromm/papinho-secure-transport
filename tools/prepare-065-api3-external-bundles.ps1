# SPDX-License-Identifier: MPL-2.0
param(
    [string]$PackageDirectory = "dist\packages\0.6.5",
    [string]$FixtureDirectory = "build\fixtures\interoperability-pki",
    [string]$Destination = "build\pst-0.6.5-api3-external-bundles"
)
$ErrorActionPreference = "Stop"
$repo = Split-Path -Parent $PSScriptRoot
foreach ($name in @("PackageDirectory", "FixtureDirectory", "Destination")) {
    $value = Get-Variable -Name $name -ValueOnly
    if (-not [IO.Path]::IsPathRooted($value)) {
        Set-Variable -Name $name -Value (Join-Path $repo $value)
    }
}
$packages = [IO.Path]::GetFullPath($PackageDirectory)
$fixtures = [IO.Path]::GetFullPath($FixtureDirectory)
$root = [IO.Path]::GetFullPath($Destination)
$work = Join-Path $root "_work"
$sourceName = "papinho-secure-transport-0.6.5-src.zip"
$assetNames = @(
    $sourceName,
    "papinho-secure-transport-0.6.5-win32-x86-vc6-retrozilla-nss-ml.zip",
    "papinho-secure-transport-0.6.5-win32-x86-vc6-retrozilla-nss-md.zip",
    "papinho-secure-transport-0.6.5-win32-x64-msvc-19.51-schannel.zip",
    "papinho-secure-transport-0.6.5-win32-x64-msvc-19.51-openssl3.zip",
    "papinho-secure-transport-0.6.5-win32-x64-msvc-19.51-schannel-openssl3.zip",
    "SHA256SUMS-packages.txt"
)
function Require-File([string]$Path,[string]$Reason) {
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) { throw "$Reason`: $Path" }
}
function Write-Utf8([string]$Path,[string[]]$Lines) {
    [IO.File]::WriteAllText($Path,(($Lines -join "`n") + "`n"),(New-Object Text.UTF8Encoding($false)))
}
function Assert-Identity([string]$Version,[string]$Manifest,[string]$Target,[string]$Crt) {
    if ($Version -notmatch '(?m)^library_version=0\.6\.5$' -or $Version -notmatch '(?m)^api_version=3\.0\.0$' -or $Version -notmatch '(?m)^spi_version=4\.0$') { throw "version identity mismatch" }
    if ($Manifest -notmatch '(?m)^library_version=0\.6\.5$' -or $Manifest -notmatch '(?m)^api_version=3\.0\.0$' -or $Manifest -notmatch '(?m)^spi_version=4\.0$') { throw "manifest version mismatch" }
    if ($Manifest -notmatch ('(?m)^target_id=' + [regex]::Escape($Target) + '$')) { throw "target mismatch" }
    if ($Crt -and $Manifest -notmatch ('(?m)^crt=' + $Crt + '$')) { throw "CRT mismatch" }
}
function Expect-Rejection([scriptblock]$Action,[string]$Name) {
    $rejected=$false; try { & $Action } catch { $rejected=$true }
    if (-not $rejected) { throw "negative gate did not reject: $Name" }
}
function Write-TransferManifest([string]$Directory) {
    $lines=Get-ChildItem -LiteralPath $Directory -File -Recurse | Where-Object Name -ne "TRANSFER-SHA256SUMS.txt" | Sort-Object FullName | ForEach-Object {
        "{0}  {1}" -f (Get-FileHash -Algorithm SHA256 -LiteralPath $_.FullName).Hash.ToLowerInvariant(),$_.FullName.Substring($Directory.Length+1).Replace('\','/')
    }
    Write-Utf8 (Join-Path $Directory "TRANSFER-SHA256SUMS.txt") $lines
}
function Set-DeterministicPeTimestamp([string]$Path) {
    $bytes=[IO.File]::ReadAllBytes($Path);$offset=[BitConverter]::ToInt32($bytes,0x3c)
    if ($offset -lt 0 -or $offset+12 -gt $bytes.Length -or $bytes[$offset] -ne 0x50 -or $bytes[$offset+1] -ne 0x45) { throw "invalid PE: $Path" }
    0..3 | ForEach-Object { $bytes[$offset+8+$_]=0 };[IO.File]::WriteAllBytes($Path,$bytes)
}
function New-FixedZip([string]$Source,[string]$DestinationPath) {
    Add-Type -AssemblyName System.IO.Compression
    $stream=[IO.File]::Open($DestinationPath,[IO.FileMode]::CreateNew)
    try {
        $archive=New-Object IO.Compression.ZipArchive($stream,[IO.Compression.ZipArchiveMode]::Create,$false)
        try {
            Get-ChildItem -LiteralPath $Source -File -Recurse | ForEach-Object { [pscustomobject]@{File=$_;Relative=$_.FullName.Substring($Source.Length+1).Replace('\','/')} } | Sort-Object Relative | ForEach-Object {
                $entry=$archive.CreateEntry($_.Relative,[IO.Compression.CompressionLevel]::Optimal)
                $entry.LastWriteTime=New-Object DateTimeOffset(2000,1,1,0,0,0,[TimeSpan]::Zero)
                $input=[IO.File]::OpenRead($_.File.FullName);try{$output=$entry.Open();try{$input.CopyTo($output)}finally{$output.Dispose()}}finally{$input.Dispose()}
            }
        } finally { $archive.Dispose() }
    } finally { $stream.Dispose() }
}
foreach($name in $assetNames){Require-File (Join-Path $packages $name) "missing candidate asset"}
$unexpected=Get-ChildItem -LiteralPath $packages -File | Where-Object Name -notin $assetNames
if($unexpected){throw "contaminated package directory: $($unexpected.Name -join ',')"}
$expected=@{}
Get-Content (Join-Path $packages "SHA256SUMS-packages.txt") | ForEach-Object { if($_ -match '^([0-9a-f]{64})  (.+)$'){$expected[$matches[2]]=$matches[1]} }
foreach($name in $assetNames | Where-Object {$_ -like '*.zip'}){
    $actual=(Get-FileHash -Algorithm SHA256 (Join-Path $packages $name)).Hash.ToLowerInvariant()
    if(-not $expected.ContainsKey($name) -or $actual -ne $expected[$name]){throw "checksum mismatch: $name"}
}
if(Test-Path -LiteralPath $root){Remove-Item -LiteralPath $root -Recurse -Force}
New-Item -ItemType Directory -Path $work | Out-Null
Add-Type -AssemblyName System.IO.Compression.FileSystem
$source=Join-Path $work "source"
[IO.Compression.ZipFile]::ExtractToDirectory((Join-Path $packages $sourceName),$source)
foreach($name in @("LICENSE","THIRD_PARTY_NOTICES.md","docs\provider-spi-4.0.md","docs\adr\ADR-0008-lifecycle-failure-aware-de-fechamento-nativo.md","tests\test_connection_failures.c","tests\test_release.h")){Require-File (Join-Path $source $name) "missing source contract"}
$header=Get-Content -Raw (Join-Path $source "include\papinho_secure_transport.h")
if($header -notmatch 'pst_connection_try_release' -or $header -match '\bpst_connection_release\s*\('){throw "Model E public symbols mismatch"}
$goodVersion="library_version=0.6.5`napi_version=3.0.0`nspi_version=4.0";$goodManifest="library_version=0.6.5`napi_version=3.0.0`nspi_version=4.0`ntarget_id=x`ncrt=md"
Expect-Rejection {Assert-Identity "library_version=0.6.4`napi_version=3.0.0`nspi_version=4.0" $goodManifest "x" "md"} "wrong library"
Expect-Rejection {Assert-Identity "library_version=0.6.5`napi_version=2.2.0`nspi_version=4.0" $goodManifest "x" "md"} "wrong API"
Expect-Rejection {Assert-Identity "library_version=0.6.5`napi_version=3.0.0`nspi_version=3.0" $goodManifest "x" "md"} "wrong SPI"
Expect-Rejection {Assert-Identity $goodVersion $goodManifest "wrong" "md"} "wrong target"
Expect-Rejection {Assert-Identity $goodVersion $goodManifest "x" "ml"} "wrong CRT"
Expect-Rejection {Require-File (Join-Path $work "missing-spi4-doc.md") "missing SPI4 document"} "missing SPI4 document"

$sdkRoots=@{}
function Build-Vc6Variant([string]$Variant){
    $target="win32-x86-vc6-retrozilla-nss-$Variant"
    $zip=Join-Path $packages "papinho-secure-transport-0.6.5-$target.zip"
    $sdk=Join-Path $work "sdk-$Variant";$sdkRoots[$Variant]=$sdk
    [IO.Compression.ZipFile]::ExtractToDirectory($zip,$sdk)
    Assert-Identity (Get-Content -Raw (Join-Path $sdk "VERSION")) (Get-Content -Raw (Join-Path $sdk "manifest.ini")) $target $Variant
    foreach($doc in @("docs\provider-spi-4.0.md","docs\adr\ADR-0008-lifecycle-failure-aware-de-fechamento-nativo.md")){Require-File (Join-Path $sdk $doc) "missing SPI4/Model E SDK document"}
    $output=Join-Path $work "bin-$Variant";New-Item -ItemType Directory $output|Out-Null
    $crt=if($Variant -eq "ml"){"/ML"}else{"/MD"}
    $nss=Join-Path $source "third_party\retrozilla-nss\prebuilt\win32-x86-vc6\sdk"
    $command='call "'+(Join-Path $source 'tools\vc6-env.bat')+'" >nul && cl /nologo /W4 /O2 /TC '+$crt
    $command+=' /I"'+(Join-Path $sdk 'include')+'" /I"'+(Join-Path $source 'tests')+'" /I"'+(Join-Path $source 'src')+'"'
    $command+=' /I"'+(Join-Path $nss 'include\nspr')+'" /I"'+(Join-Path $nss 'public\nss')+'"'
    $command+=' /Fo"'+(Join-Path $output 'idle-close.obj')+'" /Fe"'+(Join-Path $output 'idle-close.exe')+'"'
    $command+=' "'+(Join-Path $source 'tests\test_connection_failures.c')+'" /link /LIBPATH:"'+(Join-Path $sdk "lib\$target")+'" papinho_secure_transport.lib wsock32.lib'
    $compile=@(cmd.exe /d /c $command 2>&1);$compile|ForEach-Object{Write-Host $_}
    if($LASTEXITCODE -ne 0){throw "VC6 package-only harness build failed: $Variant"}
    Set-DeterministicPeTimestamp (Join-Path $output "idle-close.exe")
    Remove-Item -LiteralPath (Join-Path $output "idle-close.obj") -Force
}
Build-Vc6Variant "ml"
Build-Vc6Variant "md"
foreach($name in @("root.der","client.der","client.pk8","root.pem","server-chain.pem","server.key")){
    Require-File (Join-Path $fixtures $name) "missing governed fixture"
}
$specs=@(
    @{Name="real-nt4-vc6-ml";Variant="ml";Kind="REAL_NT4_VC6_ML"},
    @{Name="real-nt4-vc6-md";Variant="md";Kind="REAL_NT4_VC6_MD"},
    @{Name="clean-machine-vc6-ml";Variant="ml";Kind="CLEAN_MACHINE_VC6_ML"},
    @{Name="clean-machine-vc6-md";Variant="md";Kind="CLEAN_MACHINE_VC6_MD"}
)
$bundleDirs=@()
foreach($spec in $specs){
    $dir=Join-Path $root $spec.Name;New-Item -ItemType Directory $dir|Out-Null;$bundleDirs+=$dir
    $variant=$spec.Variant;$target="win32-x86-vc6-retrozilla-nss-$variant"
    $sdkName="papinho-secure-transport-0.6.5-$target.zip"
    Copy-Item (Join-Path $packages $sourceName),(Join-Path $packages $sdkName) -Destination $dir
    Copy-Item (Join-Path $work "bin-$variant\idle-close.exe") -Destination $dir
    Get-ChildItem (Join-Path $sdkRoots[$variant] "runtime\$target") -File|Copy-Item -Destination $dir
    foreach($name in @("root.der","client.der","client.pk8")){Copy-Item (Join-Path $fixtures $name) -Destination $dir}
    Copy-Item (Join-Path $source "tests\connection_failure_server.py") -Destination $dir
    $fixtureDir=Join-Path $dir "fixtures";New-Item -ItemType Directory $fixtureDir|Out-Null
    foreach($name in @("root.pem","server-chain.pem","server.key")){Copy-Item (Join-Path $fixtures $name) -Destination $fixtureDir}
    foreach($mode in @("healthy_idle","clean_close","idle_reset")){
        $label=$mode.Replace("_","-")
        Write-Utf8 (Join-Path $dir "run-$label.bat") @("@echo off",'if "%2"=="" goto usage',"set PST_NSS_TRACE_FILE=$mode-backend.log","idle-close.exe %1 %2 localhost root.der client.der client.pk8 13 fixture/1 $mode 0","exit /b %errorlevel%",":usage","echo Usage: run-$label.bat HOST PORT","exit /b 2")
    }
    Write-Utf8 (Join-Path $dir "README.txt") @(
        "PST 0.6.5 API3/SPI4 $($spec.Kind)","SDK_TARGET=$target","SDK_SHA256=$($expected[$sdkName])",
        "Validate TRANSFER-SHA256SUMS.txt. Never install DLLs globally.",
        "Fixture: python connection_failure_server.py 0.0.0.0 PORT MODE fixtures/server-chain.pem fixtures/server.key fixtures/root.pem 13 fixture/1 required",
        "Ports: healthy_idle=8751, clean_close=8752, idle_reset=8753. Start one fixture, then the matching BAT.",
        "Expected: healthy APP_BYTES=0/no spin; clean CLOSED/CLEAN; reset TRUNCATED; exit 0.",
        "Return console, fixture and *-backend.log only. Never return server.key or client.pk8.",
        "DNS/TCP/firewall/toolchain failures are ENVIRONMENT_FAILURE. Record loaded NSS/NSPR module paths and hashes.",
        "Synthetic close-failure evidence is not proof of a real NSPR/Winsock close failure."
    )
    Write-TransferManifest $dir
}
$consumers=@(
    @{Name="papinhobrowser-nss-md";Kind="PAPINHOBROWSER_NSS_MD";Package="papinho-secure-transport-0.6.5-win32-x86-vc6-retrozilla-nss-md.zip";Target="win32-x86-vc6-retrozilla-nss-md"},
    @{Name="papinhoaccelerator-openssl-x64";Kind="PAPINHOACCELERATOR_OPENSSL_X64";Package="papinho-secure-transport-0.6.5-win32-x64-msvc-19.51-openssl3.zip";Target="win32-x64-msvc-19.51-openssl3"}
)
foreach($consumer in $consumers){
    $dir=Join-Path $root $consumer.Name;New-Item -ItemType Directory $dir|Out-Null;$bundleDirs+=$dir
    Copy-Item (Join-Path $packages $consumer.Package),(Join-Path $packages $sourceName) -Destination $dir
    Write-Utf8 (Join-Path $dir "README.txt") @(
        "PST 0.6.5 API3/SPI4 $($consumer.Kind) PACKAGE-ONLY HANDOFF","SDK_TARGET=$($consumer.Target)","SDK_SHA256=$($expected[$consumer.Package])",
        "Use a fresh isolated consumer worktree. Do not change the production pin or existing worktree.",
        "Migrate lifetime code to pst_connection_try_release and initialize 28-byte PST_RELEASE_INFO.",
        "Discard a reference only for DESTROYED; retain it for PRECONDITION_RETAINED or CLOSE_UNRESOLVED_RETAINED.",
        "Accelerator P11/P12 executes first. Browser modes A-E/Mode B follow accepted Accelerator evidence.",
        "Require real TLS/mTLS/ALPN, idle no-spin, reciprocal shutdown, FIN/RST truncation, wait-set unbind, no resurrection, no provider switch and exactly-once teardown.",
        "Record package-local module paths/hashes. Return redacted logs only; never return private keys.",
        "No consumer commit, production pin update, Mode C/C1 work or PST publication is authorized."
    )
    Write-TransferManifest $dir
}
$bundleOutput=Join-Path $root "bundles";New-Item -ItemType Directory $bundleOutput|Out-Null
$master=@()
foreach($dir in $bundleDirs){
    $zip=Join-Path $bundleOutput ((Split-Path $dir -Leaf)+".zip")
    New-FixedZip $dir $zip
    $master+="{0}  {1}" -f (Get-FileHash -Algorithm SHA256 $zip).Hash.ToLowerInvariant(),(Split-Path $zip -Leaf)
}
Write-Utf8 (Join-Path $bundleOutput "R7-MASTER-SHA256SUMS.txt") $master
Write-Utf8 (Join-Path $bundleOutput "R8-OWNER-RUNBOOK.txt") @(
    "PST 0.6.5 API3/SPI4 owner execution order",
    "1. Validate each ZIP against R7-MASTER-SHA256SUMS.txt and its extracted TRANSFER-SHA256SUMS.txt.",
    "2. Accelerator P11/P12 and affected gates first using papinhoaccelerator-openssl-x64.zip.",
    "3. Browser x Accelerator modes A-E / Mode B only after Accelerator evidence passes.",
    "4. Execute real NT4 ML then MD, and separate clean-machine ML then MD.",
    "5. Return redacted console/fixture/backend logs and module hashes; never return private keys.",
    "BUNDLE_READY is preparation only. External PASS requires independent evidence audit."
)
Write-Output "GENERATOR_NEGATIVE_GATES=PASS"
Write-Output "R7_BUNDLE_COUNT=6"
Get-ChildItem $bundleOutput -File|Sort-Object Name|ForEach-Object{Write-Output "ARTIFACT=$($_.Name) SHA256=$((Get-FileHash -Algorithm SHA256 $_.FullName).Hash.ToLowerInvariant())"}
Write-Output "RESULT=PASS"
