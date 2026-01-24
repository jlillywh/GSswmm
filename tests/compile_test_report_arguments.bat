@echo off
cd /d "%~dp0"
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
cl /EHsc /I.. test_report_arguments.cpp ..\x64\Release\GSswmm.lib /Fe:test_report_arguments.exe /link /LIBPATH:..\lib
