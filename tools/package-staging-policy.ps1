# SPDX-License-Identifier: MPL-2.0

function Write-PstGitBlob($Repo, $ObjectId, $Destination) {
    $parent = Split-Path -Parent $Destination
    if (-not (Test-Path -LiteralPath $parent)) { New-Item -ItemType Directory -Path $parent -Force | Out-Null }
    $processInfo = New-Object Diagnostics.ProcessStartInfo
    $processInfo.FileName = "git"
    $processInfo.Arguments = "cat-file blob $ObjectId"
    $processInfo.WorkingDirectory = $Repo
    $processInfo.UseShellExecute = $false
    $processInfo.CreateNoWindow = $true
    $processInfo.RedirectStandardOutput = $true
    $processInfo.RedirectStandardError = $true
    $process = New-Object Diagnostics.Process
    $process.StartInfo = $processInfo
    if (-not $process.Start()) { throw "Unable to start git cat-file" }
    $output = [IO.File]::Open($Destination, [IO.FileMode]::Create, [IO.FileAccess]::Write, [IO.FileShare]::None)
    try { $process.StandardOutput.BaseStream.CopyTo($output) }
    finally { $output.Dispose() }
    $errorText = $process.StandardError.ReadToEnd()
    $process.WaitForExit()
    if ($process.ExitCode -ne 0) { throw "git cat-file failed for $ObjectId`: $errorText" }
    $process.Dispose()
}

function Copy-PstPackageInput($Repo, $Source, $Destination) {
    if (-not (Test-Path -LiteralPath $Source -PathType Leaf)) { throw "Required staging input is missing: $Source" }
    $repoFull = [IO.Path]::GetFullPath($Repo).TrimEnd("\")
    $sourceFull = [IO.Path]::GetFullPath($Source)
    $insideRepo = $sourceFull.StartsWith($repoFull + "\", [StringComparison]::OrdinalIgnoreCase)
    if ($insideRepo) {
        $relative = $sourceFull.Substring($repoFull.Length + 1).Replace("\", "/")
        & git -C $repoFull ls-files --error-unmatch -- $relative *> $null
        if ($LASTEXITCODE -eq 0) {
            $objectId = (& git -C $repoFull rev-parse --verify ("HEAD:" + $relative)).Trim()
            if ($LASTEXITCODE -ne 0 -or -not $objectId) { throw "Unable to resolve canonical Git blob: $relative" }
            Write-PstGitBlob $repoFull $objectId $Destination
            return
        }
    }
    $parent = Split-Path -Parent $Destination
    if (-not (Test-Path -LiteralPath $parent)) { New-Item -ItemType Directory -Path $parent -Force | Out-Null }
    Copy-Item -LiteralPath $Source -Destination $Destination -Force
}

function Write-PstUtf8Lf($Path, $Text) {
    $parent = Split-Path -Parent $Path
    if (-not (Test-Path -LiteralPath $parent)) { New-Item -ItemType Directory -Path $parent -Force | Out-Null }
    $canonical = $Text.Replace("`r`n", "`n").Replace("`r", "`n")
    [IO.File]::WriteAllText($Path, $canonical, (New-Object Text.UTF8Encoding($false)))
}
