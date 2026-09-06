@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64 10.0.19041.0
if errorlevel 1 exit /b 1
set PATH
set INCLUDE
set LIB
where cl.exe
where link.exe
cl.exe /nologo /W4 /MD /TC /showIncludes /I"C:\pse-clean-code\clean-validation\combined\include" /Fo"C:\pse-clean-code\clean-validation\consumers\combined\consumer.obj" /Fe"C:\pse-clean-code\clean-validation\consumers\combined\consumer.exe" "C:\pse-clean-code\clean-validation\consumers\consumer.c" /link /MACHINE:X64 /VERBOSE:LIB "C:\pse-clean-code\clean-validation\combined\lib\windows-x64-msvc-schannel-openssl-3.5.8\papinho_secure_transport.lib" "C:\pse-clean-code\clean-validation\combined\lib\windows-x64-msvc-schannel-openssl-3.5.8\libssl.lib" "C:\pse-clean-code\clean-validation\combined\lib\windows-x64-msvc-schannel-openssl-3.5.8\libcrypto.lib" ws2_32.lib secur32.lib crypt32.lib ncrypt.lib
exit /b %errorlevel%
