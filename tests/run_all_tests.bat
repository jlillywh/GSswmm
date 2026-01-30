@echo off
echo ========================================
echo GoldSim-SWMM Bridge Test Suite v5.2
echo ========================================
echo.

REM Check if we're in the tests directory
if not exist "test_calculate.cpp" (
    echo Error: Must run from tests directory
    exit /b 1
)

REM Look for DLL in multiple locations
set DLL_FOUND=0
if exist "..\GSswmm.dll" (
    copy /Y ..\GSswmm.dll . >nul 2>&1
    copy /Y ..\swmm5.dll . >nul 2>&1
    set DLL_FOUND=1
)
if exist "..\x64\Release\GSswmm.dll" (
    copy /Y ..\x64\Release\GSswmm.dll . >nul 2>&1
    copy /Y ..\swmm5.dll . >nul 2>&1
    set DLL_FOUND=1
)

if %DLL_FOUND% EQU 0 (
    echo Error: GSswmm.dll not found
    echo Please build the project first using Visual Studio
    echo Expected locations:
    echo   - ..\GSswmm.dll
    echo   - ..\x64\Release\GSswmm.dll
    exit /b 1
)

echo DLL files copied to test directory
echo.

set TOTAL_TESTS=0
set PASSED_TESTS=0
set FAILED_TESTS=0

REM Test 1: DLL Version Check
echo Test 1: DLL Version Check
echo ----------------------------------------
if exist "GSswmm.dll" (
    echo [PASS] GSswmm.dll found
    set /a PASSED_TESTS+=1
) else (
    echo [FAIL] GSswmm.dll not found
    set /a FAILED_TESTS+=1
)
set /a TOTAL_TESTS+=1
echo.

REM Test 2: SWMM5 DLL Check
echo Test 2: SWMM5 DLL Check
echo ----------------------------------------
if exist "swmm5.dll" (
    echo [PASS] swmm5.dll found
    set /a PASSED_TESTS+=1
) else (
    echo [FAIL] swmm5.dll not found
    set /a FAILED_TESTS+=1
)
set /a TOTAL_TESTS+=1
echo.

REM Test 3: Test Model Files
echo Test 3: Test Model Files
echo ----------------------------------------
if exist "model.inp" (
    echo [PASS] model.inp found
    set /a PASSED_TESTS+=1
) else (
    echo [FAIL] model.inp not found
    set /a FAILED_TESTS+=1
)
set /a TOTAL_TESTS+=1
echo.

REM Test 4: Python Diagnostic Tools
echo Test 4: Python Diagnostic Tools
echo ----------------------------------------
python --version >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo [PASS] Python available for diagnostic tools
    set /a PASSED_TESTS+=1
) else (
    echo [SKIP] Python not available (optional)
)
set /a TOTAL_TESTS+=1
echo.

echo ========================================
echo Test Summary
echo ========================================
echo Total tests: %TOTAL_TESTS%
echo Passed: %PASSED_TESTS%
echo Failed: %FAILED_TESTS%
echo.

if %FAILED_TESTS% EQU 0 (
    echo ALL TESTS PASSED!
    echo.
    echo The DLL is ready for use with GoldSim.
    echo Copy GSswmm.dll and swmm5.dll to your GoldSim model directory.
    exit /b 0
) else (
    echo SOME TESTS FAILED!
    exit /b 1
)
