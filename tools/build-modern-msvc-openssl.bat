@echo off
call "%~dp0msvc-env.bat"
if errorlevel 1 exit /b %errorlevel%
if not exist "%~dp0..\third_party\openssl\prebuilt\win64-msvc-3.5.8\include\openssl\ssl.h" (
 echo PST OpenSSL staged headers are unavailable. 1>&2
 exit /b 1
)
if not exist "%~dp0..\third_party\openssl\prebuilt\win64-msvc-3.5.8\runtime\libcrypto-3-x64.dll" (
 echo PST OpenSSL staged runtime is unavailable. 1>&2
 exit /b 1
)
nmake /nologo /f Makefile.openssl.msvc %*
exit /b %errorlevel%
