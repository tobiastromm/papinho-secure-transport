@echo off
if not "%PST_MODERN_VCVARS64%"=="" goto have_path
set PST_MODERN_VCVARS64=C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat
:have_path
if not exist "%PST_MODERN_VCVARS64%" goto missing
call "%PST_MODERN_VCVARS64%"
if errorlevel 1 exit /b 1
if /I not "%VSCMD_ARG_TGT_ARCH%"=="x64" goto wrong_arch
exit /b 0
:missing
echo Modern MSVC x64 bootstrap missing: %PST_MODERN_VCVARS64%
echo Set PST_MODERN_VCVARS64 to a valid vcvars64.bat.
exit /b 2
:wrong_arch
echo Modern MSVC bootstrap did not select x64.
exit /b 3