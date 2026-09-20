@rem SPDX-License-Identifier: MPL-2.0
@echo off
setlocal
if "%~1"=="ml" goto select_ml
if "%~1"=="md" goto select_md
echo Usage: test-vc6-crt-consumer.bat ml^|md
exit /b 2

:select_ml
set "PST_CRT_FLAG=/ML"
goto selected
:select_md
set "PST_CRT_FLAG=/MD"

:selected
set "PST_CRT_VARIANT=%~1"
call "%~dp0vc6-env.bat"
if errorlevel 1 exit /b 1
set "PST_CRT_STAGE=build\vc6-crt-consumer-%PST_CRT_VARIANT%"
set "PST_CRT_PRODUCER=build\win32-x86-vc6-retrozilla-nss-%PST_CRT_VARIANT%"
if not exist "%PST_CRT_PRODUCER%\papinho_secure_transport.lib" exit /b 3
if not exist "%PST_CRT_STAGE%" mkdir "%PST_CRT_STAGE%"
if not exist "%PST_CRT_STAGE%\include" mkdir "%PST_CRT_STAGE%\include"
if not exist "%PST_CRT_STAGE%\lib" mkdir "%PST_CRT_STAGE%\lib"
if not exist "%PST_CRT_STAGE%\runtime" mkdir "%PST_CRT_STAGE%\runtime"
copy /y include\papinho_secure_transport.h "%PST_CRT_STAGE%\include\" >nul
if errorlevel 1 exit /b 4
copy /y include\papinho_secure_transport_win32.h "%PST_CRT_STAGE%\include\" >nul
if errorlevel 1 exit /b 4
copy /y "%PST_CRT_PRODUCER%\papinho_secure_transport.lib" "%PST_CRT_STAGE%\lib\" >nul
if errorlevel 1 exit /b 4
copy /y "%PST_NSS_RUNTIME%\*.dll" "%PST_CRT_STAGE%\runtime\" >nul
if errorlevel 1 exit /b 4
cl /nologo /W4 /O2 /TC %PST_CRT_FLAG% /I"%PST_CRT_STAGE%\include" /Fe"%PST_CRT_STAGE%\consumer.exe" tests\test_vc6_crt_public_consumer.c /link "%PST_CRT_STAGE%\lib\papinho_secure_transport.lib" wsock32.lib /LIBPATH:%PST_VC6_ROOT%\VC98\Lib /LIBPATH:%PST_VC6_ROOT%\VC98\MFC\Lib
if errorlevel 1 exit /b 5
set "PATH=%CD%\%PST_CRT_STAGE%\runtime;%SystemRoot%\System32;%SystemRoot%"
"%PST_CRT_STAGE%\consumer.exe"
if errorlevel 1 exit /b 6
echo VC6_CRT_CONSUMER_%PST_CRT_VARIANT%=PASS
exit /b 0
