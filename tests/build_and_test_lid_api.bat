@echo off
REM Build and test LID API extensions
REM This script compiles and runs tests for the new SWMM5 LID API functions

echo ========================================
echo Building LID API Tests
echo ========================================

REM Compile test executable
cl /EHsc /std:c++17 /I..\include test_lid_api.cpp /link /LIBPATH:..\lib swmm5.lib /OUT:test_lid_api.exe

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
test_lid_api.exe

if %ERRORLEVEL% NEQ 0 (
    echo Tests failed!
    exit /b 1
)

echo.
echo ========================================
echo All LID API tests passed!
echo ========================================
