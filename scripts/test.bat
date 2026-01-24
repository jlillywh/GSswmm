@echo off
REM ============================================================================
REM test.bat - Run all tests and report results
REM ============================================================================
setlocal
cd /d "%~dp0..\tests"

echo ========================================
echo Running All GoldSim-SWMM Bridge Tests
echo ========================================
echo.

set TOTAL_TESTS=0
set PASSED_TESTS=0
set FAILED_TESTS=0

echo Test Suite 1: Lifecycle Tests
echo ----------------------------------------
test_lifecycle.exe
if %ERRORLEVEL% EQU 0 (
    set /a PASSED_TESTS+=1
    echo [PASS] Lifecycle tests passed
) else (
    set /a FAILED_TESTS+=1
    echo [FAIL] Lifecycle tests failed
)
set /a TOTAL_TESTS+=1
echo.

echo Test Suite 2: Calculate Tests
echo ----------------------------------------
test_calculate.exe
if %ERRORLEVEL% EQU 0 (
    set /a PASSED_TESTS+=1
    echo [PASS] Calculate tests passed
) else (
    set /a FAILED_TESTS+=1
    echo [FAIL] Calculate tests failed
)
set /a TOTAL_TESTS+=1
echo.

echo Test Suite 3: Error Handling Tests
echo ----------------------------------------
test_error_handling.exe
if %ERRORLEVEL% EQU 0 (
    set /a PASSED_TESTS+=1
    echo [PASS] Error handling tests passed
) else (
    set /a FAILED_TESTS+=1
    echo [FAIL] Error handling tests failed
)
set /a TOTAL_TESTS+=1
echo.

echo Test Suite 4: File Validation Tests
echo ----------------------------------------
test_file_validation.exe
if %ERRORLEVEL% EQU 0 (
    set /a PASSED_TESTS+=1
    echo [PASS] File validation tests passed
) else (
    set /a FAILED_TESTS+=1
    echo [FAIL] File validation tests failed
)
set /a TOTAL_TESTS+=1
echo.

echo Test Suite 5: Subcatchment Validation Tests
echo ----------------------------------------
test_subcatchment_validation.exe
if %ERRORLEVEL% EQU 0 (
    set /a PASSED_TESTS+=1
    echo [PASS] Subcatchment validation tests passed
) else (
    set /a FAILED_TESTS+=1
    echo [FAIL] Subcatchment validation tests failed
)
set /a TOTAL_TESTS+=1
echo.

echo Test Suite 6: Out-of-Range Subcatchment Tests
echo ----------------------------------------
test_subcatchment_out_of_range.exe
if %ERRORLEVEL% EQU 0 (
    set /a PASSED_TESTS+=1
    echo [PASS] Out-of-range subcatchment tests passed
) else (
    set /a FAILED_TESTS+=1
    echo [FAIL] Out-of-range subcatchment tests failed
)
set /a TOTAL_TESTS+=1
echo.

echo Test Suite 7: JSON Parsing Tests
echo ----------------------------------------
test_json_parsing.exe
if %ERRORLEVEL% EQU 0 (
    set /a PASSED_TESTS+=1
    echo [PASS] JSON parsing tests passed
) else (
    set /a FAILED_TESTS+=1
    echo [FAIL] JSON parsing tests failed
)
set /a TOTAL_TESTS+=1
echo.

echo Test Suite 8: Integration E2E Tests
echo ----------------------------------------
test_integration_e2e.exe
if %ERRORLEVEL% EQU 0 (
    set /a PASSED_TESTS+=1
    echo [PASS] Integration E2E tests passed
) else (
    set /a FAILED_TESTS+=1
    echo [FAIL] Integration E2E tests failed
)
set /a TOTAL_TESTS+=1
echo.

echo ========================================
echo Test Summary
echo ========================================
echo Total test suites: %TOTAL_TESTS%
echo Passed: %PASSED_TESTS%
echo Failed: %FAILED_TESTS%
echo.

if %FAILED_TESTS% EQU 0 (
    echo ALL TEST SUITES PASSED!
    endlocal
    exit /b 0
) else (
    echo SOME TEST SUITES FAILED!
    endlocal
    exit /b 1
)
