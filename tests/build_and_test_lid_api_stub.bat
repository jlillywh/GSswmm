@echo off
REM Build and test LID API extensions with stub implementation
REM This script compiles and runs tests using stub implementations

echo ========================================
echo Building LID API Tests (with stubs)
echo ========================================

REM Set up Visual Studio environment
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
if errorlevel 1 (
    call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
)

REM Compile test executable with stub implementation
cl /EHsc /std:c++17 /I..\include ^
   test_lid_api.cpp ^
   swmm_lid_api_stub.cpp ^
   swmm_mock.cpp ^
   /link /OUT:test_lid_api_stub.exe

if %ERRORLEVEL% NEQ 0 (
    echo Build failed!
    exit /b 1
)

echo.
echo ========================================
echo Running LID API Tests
echo ========================================
echo.

REM Run tests
test_lid_api_stub.exe

if %ERRORLEVEL% NEQ 0 (
    echo Tests failed!
    exit /b 1
)

echo.
echo ========================================
echo All LID API tests passed!
echo ========================================
