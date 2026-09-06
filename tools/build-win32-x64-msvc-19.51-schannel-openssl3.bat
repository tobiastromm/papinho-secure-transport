@rem SPDX-License-Identifier: MPL-2.0
@echo off
call "%~dp0msvc-env.bat"
if errorlevel 1 exit /b %errorlevel%
nmake /nologo /f Makefile.combined.msvc %*
exit /b %errorlevel%