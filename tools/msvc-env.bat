@echo off
if not "%PST_MODERN_VCVARS64%"=="" goto validate_path
set PST_VSWHERE=C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe
if not exist "%PST_VSWHERE%" goto missing_vswhere
for /f "usebackq tokens=*" %%I in (`"%PST_VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set PST_MODERN_VSINSTALL=%%I
if "%PST_MODERN_VSINSTALL%"=="" goto missing_installation
set PST_MODERN_VCVARS64=%PST_MODERN_VSINSTALL%\VC\Auxiliary\Build\vcvars64.bat
:validate_path
if not exist "%PST_MODERN_VCVARS64%" goto missing
call "%PST_MODERN_VCVARS64%"
if errorlevel 1 exit /b 1
if /I not "%VSCMD_ARG_TGT_ARCH%"=="x64" goto wrong_arch
if "%VCToolsInstallDir%"=="" goto incomplete
if "%WindowsSdkDir%"=="" goto incomplete
if "%WindowsSDKVersion%"=="" goto incomplete
if "%UniversalCRTSdkDir%"=="" goto incomplete
if "%LIB%"=="" goto incomplete
if "%INCLUDE%"=="" goto incomplete
exit /b 0
:missing_vswhere
echo Modern MSVC discovery tool missing: %PST_VSWHERE%
exit /b 4
:missing_installation
echo No complete modern Visual Studio C++ x64 Build Tools installation was found.
exit /b 5
:missing
echo Modern MSVC x64 bootstrap missing: %PST_MODERN_VCVARS64%
echo Set PST_MODERN_VCVARS64 to a valid vcvars64.bat.
exit /b 2
:wrong_arch
echo Modern MSVC bootstrap did not select x64.
exit /b 3
:incomplete
echo Modern MSVC bootstrap returned an incomplete compiler/SDK environment.
exit /b 6
