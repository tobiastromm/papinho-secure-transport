@echo off
call "%~dp0vc6-env.bat"
if errorlevel 1 exit /b 1
nmake /f Makefile.vc6 %*
exit /b %errorlevel%