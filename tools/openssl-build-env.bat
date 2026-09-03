@echo off
call "%~dp0msvc-env.bat"
if errorlevel 1 exit /b %errorlevel%
if defined PST_OPENSSL_PERL goto perl_check
for %%I in (perl.exe) do set "PST_OPENSSL_PERL=%%~$PATH:I"
if defined PST_OPENSSL_PERL goto perl_check
if exist "C:\Strawberry\perl\bin\perl.exe" set "PST_OPENSSL_PERL=C:\Strawberry\perl\bin\perl.exe"
:perl_check
if not defined PST_OPENSSL_PERL (
 echo Perl was not found. Set PST_OPENSSL_PERL to perl.exe. 1>&2
 exit /b 1
)
if not exist "%PST_OPENSSL_PERL%" (
 echo PST_OPENSSL_PERL does not exist: %PST_OPENSSL_PERL% 1>&2
 exit /b 1
)
if defined PST_OPENSSL_NASM goto nasm_check
for %%I in (nasm.exe) do set "PST_OPENSSL_NASM=%%~$PATH:I"
if defined PST_OPENSSL_NASM goto nasm_check
if exist "%ProgramFiles%\NASM\nasm.exe" set "PST_OPENSSL_NASM=%ProgramFiles%\NASM\nasm.exe"
:nasm_check
if not defined PST_OPENSSL_NASM (
 echo NASM was not found. Set PST_OPENSSL_NASM to nasm.exe. 1>&2
 exit /b 1
)
if not exist "%PST_OPENSSL_NASM%" (
 echo PST_OPENSSL_NASM does not exist: %PST_OPENSSL_NASM% 1>&2
 exit /b 1
)
for %%I in ("%PST_OPENSSL_PERL%") do set "PATH=%%~dpI;%PATH%"
for %%I in ("%PST_OPENSSL_NASM%") do set "PATH=%%~dpI;%PATH%"
echo PST OpenSSL build prerequisites ready.
echo PERL_PATH=%PST_OPENSSL_PERL%
echo NASM_PATH=%PST_OPENSSL_NASM%
exit /b 0
