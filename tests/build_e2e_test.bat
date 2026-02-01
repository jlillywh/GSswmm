@echo off
REM Build script for end-to-end LID integration test

echo ========================================
echo Building End-to-End LID Integration Test
echo ========================================
echo.

REM Find Visual Studio
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "%VSWHERE%" (
    for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
        set "VSINSTALLDIR=%%i"
    )
)

if not defined VSINSTALLDIR (
    set "VSINSTALLDIR=C:\Program Files\Microsoft Visual Studio\18\Community"
)

echo Using Visual Studio at: %VSINSTALLDIR%
echo Initializing build environment...
call "%VSINSTALLDIR%\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1

echo.
echo Compiling test...
cl /EHsc /W3 /MD /I..\include test_e2e_lid_integration.cpp /link /OUT:test_e2e_lid_integration.exe

if %ERRORLEVEL% NEQ 0 (
    echo [FAIL] Compilation failed
    exit /b 1
)

echo [PASS] Compilation successful
echo.
echo Executable: test_e2e_lid_integration.exe
echo.
echo To run the test:
echo   test_e2e_lid_integration.exe
echo.
