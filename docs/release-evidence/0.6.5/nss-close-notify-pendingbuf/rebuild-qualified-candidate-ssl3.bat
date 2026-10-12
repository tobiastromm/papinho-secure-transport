@echo off
setlocal
rem Governed RetroZilla NSS candidate rebuild. Run after applying the canonical patch.
call C:\MSVC600-master\VC98\Bin\VCVARS32.BAT
if errorlevel 1 exit /b 1
set "PATH=C:\Projetos\PapinhoSecureTransport\build\nss-shutdown-baseline\vc6-pp-toolchain\VC98\Bin;C:\mozilla-build\moztools-180compat\bin;%PATH%"
if exist C:\projects\RetroZilla\obj-rzSuite-release\nss\ssl rmdir /s /q C:\projects\RetroZilla\obj-rzSuite-release\nss\ssl
if exist C:\projects\RetroZilla\obj-rzSuite-release\dist\lib\ssl3.lib del /q C:\projects\RetroZilla\obj-rzSuite-release\dist\lib\ssl3.lib
if exist C:\projects\RetroZilla\obj-rzSuite-release\dist\bin\ssl3.dll del /q C:\projects\RetroZilla\obj-rzSuite-release\dist\bin\ssl3.dll
C:\mozilla-build\msys\bin\bash.exe --login -c "cd /c/projects/RetroZilla/obj-rzSuite-release/security/manager && /usr/local/bin/make libs"
exit /b %ERRORLEVEL%
