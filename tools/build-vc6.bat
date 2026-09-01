@echo off
call "%~dp0vc6-env.bat"
if errorlevel 1 goto fail
nmake /f Makefile.vc6 %*
if errorlevel 1 goto fail
goto success
:fail
verify other 2>nul
goto end
:success
ver >nul
:end