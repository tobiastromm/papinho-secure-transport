# SPDX-License-Identifier: MPL-2.0
param([string]$ValidationDirectory,[string]$OutputDirectory)
$ErrorActionPreference="Stop"
$repo=Split-Path -Parent $PSScriptRoot
if(-not $ValidationDirectory){$ValidationDirectory=Join-Path $repo "dist\validation\0.4.0"}
if(-not $OutputDirectory){$OutputDirectory=Join-Path $ValidationDirectory "nt4-transfer"}
$ValidationDirectory=[IO.Path]::GetFullPath($ValidationDirectory)
$OutputDirectory=[IO.Path]::GetFullPath($OutputDirectory)
$source=Join-Path $ValidationDirectory "source\tests\test_tls_runtime_integration.c"
$sdk=Join-Path $ValidationDirectory "windows-nt4-x86-vc6-retrozilla-nss"
foreach($path in @($source,(Join-Path $sdk "include\papinho_secure_transport.h"),(Join-Path $sdk "lib\windows-nt4-x86-vc6-retrozilla-nss\papinho_secure_transport.lib"))){if(-not(Test-Path -LiteralPath $path -PathType Leaf)){throw("Run validate-release-packages.ps1 first; missing "+$path)}}
if(Test-Path -LiteralPath $OutputDirectory){Remove-Item -LiteralPath $OutputDirectory -Recurse -Force}
New-Item -ItemType Directory -Path $OutputDirectory -Force|Out-Null
$exe=Join-Path $OutputDirectory "test_tls_runtime_integration.exe";$obj=Join-Path $OutputDirectory "test_tls_runtime_integration.obj"
$include=Join-Path $sdk "include";$lib=Join-Path $sdk "lib\windows-nt4-x86-vc6-retrozilla-nss";$envBat=Join-Path $repo "tools\vc6-env.bat"
$command='call "'+$envBat+'" >nul && cl /nologo /W4 /I"'+$include+'" /Fo"'+$obj+'" /Fe"'+$exe+'" /Tc"'+$source+'" /link /LIBPATH:"'+$lib+'" papinho_secure_transport.lib wsock32.lib'
cmd.exe /d /c $command
if($LASTEXITCODE -ne 0){throw "NT4 validation client compile/link failed"}
Get-ChildItem -LiteralPath (Join-Path $sdk "runtime\windows-nt4-x86-vc6-retrozilla-nss") -File|Copy-Item -Destination $OutputDirectory -Force
$run12=@('@echo off','if "%3"=="" goto usage','set PST_NSS_TRACE_FILE=tls12-modules.log','test_tls_runtime_integration.exe %1 %2 %3 ca.der client.der client.pk8 12 12 fixture/1','if errorlevel 1 goto fail','echo PAPINHOSECURETRANSPORT RELEASE 0.4.0 NT4 TLS 1.2 MTLS ALPN PASS','goto pass',':usage','echo Usage: run_tls12.bat HOST PORT HOSTNAME','goto fail_end',':fail','echo PAPINHOSECURETRANSPORT RELEASE 0.4.0 NT4 TLS 1.2 MTLS ALPN FAIL','goto fail_end',':pass','ver >nul','goto end',':fail_end','verify other 2>nul',':end')
$run13=($run12 -join [Environment]::NewLine).Replace('tls12','tls13').Replace('TLS 1.2','TLS 1.3').Replace(' 12 12 ',' 13 13 ')
$readme=@('PAPINHOSECURETRANSPORT 0.4.0 FINAL NT4 RELEASE VALIDATION','========================================================','This directory was built from the extracted release ZIPs, not dist staging.','Copy test-only ca.der, client.der and client.pk8 generated for the canonical','fixture into this directory. Never use production credentials.','Run on Windows NT 4.0 SP6 x86:','  run_tls12.bat HOST PORT localhost','  run_tls13.bat HOST PORT localhost','Return console output, tls12-modules.log, tls13-modules.log, server output,','the original NSS ZIP SHA-256, OS/service pack, architecture and module paths.','Preparation is not PASS. Both real TLS runs are mandatory.')
$utf8=New-Object Text.UTF8Encoding($false)
[IO.File]::WriteAllText((Join-Path $OutputDirectory "run_tls12.bat"),(($run12-join [Environment]::NewLine)+[Environment]::NewLine),$utf8)
[IO.File]::WriteAllText((Join-Path $OutputDirectory "run_tls13.bat"),($run13+[Environment]::NewLine),$utf8)
[IO.File]::WriteAllText((Join-Path $OutputDirectory "README-NT4.txt"),(($readme-join [Environment]::NewLine)+[Environment]::NewLine),$utf8)
$lines=Get-ChildItem -LiteralPath $OutputDirectory -File|Where-Object Name -ne "MANIFEST-SHA256.txt"|Sort-Object Name|ForEach-Object{'{0}  {1}' -f (Get-FileHash -Algorithm SHA256 -LiteralPath $_.FullName).Hash.ToLowerInvariant(),$_.Name}
[IO.File]::WriteAllText((Join-Path $OutputDirectory "MANIFEST-SHA256.txt"),(($lines-join [Environment]::NewLine)+[Environment]::NewLine),$utf8)
Write-Output ("NT4_TRANSFER_DIRECTORY="+$OutputDirectory)
Write-Output "COMPILE=PASS"
Write-Output "LINK=PASS"
Write-Output "RUNTIME_SOURCE=EXTRACTED_NSS_RELEASE_ZIP"
Write-Output "NT4_EXECUTION=PENDING_REAL_NT4"
