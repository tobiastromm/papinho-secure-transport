@echo off
if not "%PST_VC6_ROOT%"=="" goto vc6_root_set
if exist C:\MSVC600-master\VC98\Bin\cl.exe set PST_VC6_ROOT=C:\MSVC600-master
:vc6_root_set
if "%PST_VC6_ROOT%"=="" goto vc6_missing
if not exist "%PST_VC6_ROOT%\VC98\Bin\cl.exe" goto vc6_missing
if not exist "%PST_VC6_ROOT%\VC98\Bin\link.exe" goto vc6_missing
if not exist "%PST_VC6_ROOT%\VC98\Bin\nmake.exe" goto vc6_missing
call "%PST_VC6_ROOT%\VC98\Bin\VCVARS32.BAT"
if errorlevel 1 goto vc6_setup_failed
set LIB=%PST_VC6_ROOT%\VC98\Lib;%PST_VC6_ROOT%\VC98\MFC\Lib
set LDFLAGS=/link /LIBPATH:%PST_VC6_ROOT%\VC98\Lib /LIBPATH:%PST_VC6_ROOT%\VC98\MFC\Lib
if not "%PST_NSS_DIST%"=="" goto nss_dist_set
if exist C:\PSTW\pr\projects\RetroZilla\obj-rzSuite-release\dist\public\nss\ssl.h set PST_NSS_DIST=C:\PSTW\pr\projects\RetroZilla\obj-rzSuite-release\dist
:nss_dist_set
if not "%PST_NSS_DIST%"=="" set NSS_DIST=%PST_NSS_DIST%
for %%I in ("%~dp0..") do set PST_REPO_ROOT=%%~fI
set PST_NSS_RUNTIME=%PST_REPO_ROOT%\third_party\retrozilla-nss\prebuilt\win32-x86-vc6\runtime
ver >nul
goto end
:vc6_missing
echo VC6 environment unavailable. Set PST_VC6_ROOT according to docs\build-vc6.md.
verify other 2>nul
goto end
:vc6_setup_failed
echo VC6 environment setup failed. Check PST_VC6_ROOT according to docs\build-vc6.md.
verify other 2>nul
:end