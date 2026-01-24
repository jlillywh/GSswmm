@echo off
setlocal enabledelayedexpansion

echo ========================================
echo FULL SYSTEM VALIDATION
echo Scripted Interface Mapping Feature
echo ========================================
echo.

set TOTAL_TESTS=0
set PASSED_TESTS=0
set FAILED_TESTS=0

echo ========================================
echo PART 1: Python Parser Tests
echo ========================================
echo.

echo Running test_parser.py...
python -m unittest tests.test_parser -v > nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo [PASS] test_parser.py
    set /a PASSED_TESTS+=1
) else (
    echo [FAIL] test_parser.py
    set /a FAILED_TESTS+=1
)
set /a TOTAL_TESTS+=1

echo Running test_hash.py...
python -m unittest tests.test_hash -v > nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo [PASS] test_hash.py
    set /a PASSED_TESTS+=1
) else (
    echo [FAIL] test_hash.py
    set /a FAILED_TESTS+=1
)
set /a TOTAL_TESTS+=1

echo Running test_roundtrip_property.py...
python -m unittest tests.test_roundtrip_property -v > nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo [PASS] test_roundtrip_property.py
    set /a PASSED_TESTS+=1
) else (
    echo [FAIL] test_roundtrip_property.py
    set /a FAILED_TESTS+=1
)
set /a TOTAL_TESTS+=1

echo.
echo ========================================
echo PART 2: C++ DLL Tests
echo ========================================
echo.

REM Ensure model.inp exists
if not exist model.inp (
    copy tests\test_model.inp model.inp > nul
)

echo Running test_json_parsing.exe...
tests\test_json_parsing.exe > nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo [PASS] test_json_parsing.exe
    set /a PASSED_TESTS+=1
) else (
    echo [FAIL] test_json_parsing.exe
    set /a FAILED_TESTS+=1
)
set /a TOTAL_TESTS+=1

echo Running test_report_arguments.exe...
tests\test_report_arguments.exe > nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo [PASS] test_report_arguments.exe
    set /a PASSED_TESTS+=1
) else (
    echo [FAIL] test_report_arguments.exe
    set /a FAILED_TESTS+=1
)
set /a TOTAL_TESTS+=1

echo Running test_lifecycle.exe...
tests\test_lifecycle.exe > nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo [PASS] test_lifecycle.exe
    set /a PASSED_TESTS+=1
) else (
    echo [FAIL] test_lifecycle.exe
    set /a FAILED_TESTS+=1
)
set /a TOTAL_TESTS+=1

echo Running test_error_handling.exe...
tests\test_error_handling.exe > nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo [PASS] test_error_handling.exe
    set /a PASSED_TESTS+=1
) else (
    echo [FAIL] test_error_handling.exe
    set /a FAILED_TESTS+=1
)
set /a TOTAL_TESTS+=1

echo Running test_file_validation.exe...
tests\test_file_validation.exe > nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo [PASS] test_file_validation.exe
    set /a PASSED_TESTS+=1
) else (
    echo [FAIL] test_file_validation.exe
    set /a FAILED_TESTS+=1
)
set /a TOTAL_TESTS+=1

echo Running test_subcatchment_validation.exe...
tests\test_subcatchment_validation.exe > nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo [PASS] test_subcatchment_validation.exe
    set /a PASSED_TESTS+=1
) else (
    echo [FAIL] test_subcatchment_validation.exe
    set /a FAILED_TESTS+=1
)
set /a TOTAL_TESTS+=1

echo Running test_subcatchment_out_of_range.exe...
tests\test_subcatchment_out_of_range.exe > nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo [PASS] test_subcatchment_out_of_range.exe
    set /a PASSED_TESTS+=1
) else (
    echo [FAIL] test_subcatchment_out_of_range.exe
    set /a FAILED_TESTS+=1
)
set /a TOTAL_TESTS+=1

echo Running test_calculate.exe...
REM Ensure model.inp exists and regenerate mapping
copy tests\test_model.inp model.inp /Y > nul 2>&1
python generate_mapping.py tests\test_model.inp > nul 2>&1
if !ERRORLEVEL! NEQ 0 (
    echo [FAIL] Failed to generate mapping for test_calculate
    set /a FAILED_TESTS+=1
) else (
    tests\test_calculate.exe > nul 2>&1
    if !ERRORLEVEL! EQU 0 (
        echo [PASS] test_calculate.exe
        set /a PASSED_TESTS+=1
    ) else (
        echo [FAIL] test_calculate.exe
        set /a FAILED_TESTS+=1
    )
)
set /a TOTAL_TESTS+=1

echo Running test_integration_e2e.exe...
tests\test_integration_e2e.exe > nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo [PASS] test_integration_e2e.exe
    set /a PASSED_TESTS+=1
) else (
    echo [FAIL] test_integration_e2e.exe
    set /a FAILED_TESTS+=1
)
set /a TOTAL_TESTS+=1

echo.
echo ========================================
echo PART 3: Multiple Model Validation
echo ========================================
echo.

echo Testing test_model.inp...
python generate_mapping.py tests\test_model.inp > nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo [PASS] Generated mapping for test_model.inp
    set /a PASSED_TESTS+=1
) else (
    echo [FAIL] Failed to generate mapping for test_model.inp
    set /a FAILED_TESTS+=1
)
set /a TOTAL_TESTS+=1

echo Testing test_model_dummy.inp...
python generate_mapping.py tests\test_model_dummy.inp > nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo [PASS] Generated mapping for test_model_dummy.inp
    set /a PASSED_TESTS+=1
) else (
    echo [FAIL] Failed to generate mapping for test_model_dummy.inp
    set /a FAILED_TESTS+=1
)
set /a TOTAL_TESTS+=1

echo Testing test_model_no_raingages.inp...
python generate_mapping.py tests\test_model_no_raingages.inp > nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo [PASS] Generated mapping for test_model_no_raingages.inp
    set /a PASSED_TESTS+=1
) else (
    echo [FAIL] Failed to generate mapping for test_model_no_raingages.inp
    set /a FAILED_TESTS+=1
)
set /a TOTAL_TESTS+=1

echo Testing test_model_mixed_gages.inp...
python generate_mapping.py tests\test_model_mixed_gages.inp > nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo [PASS] Generated mapping for test_model_mixed_gages.inp
    set /a PASSED_TESTS+=1
) else (
    echo [FAIL] Failed to generate mapping for test_model_mixed_gages.inp
    set /a FAILED_TESTS+=1
)
set /a TOTAL_TESTS+=1

echo.
echo ========================================
echo PART 4: Error Case Testing
echo ========================================
echo.

echo Testing nonexistent file...
python generate_mapping.py nonexistent_file.inp > nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo [PASS] Correctly failed for nonexistent file
    set /a PASSED_TESTS+=1
) else (
    echo [FAIL] Should have failed for nonexistent file
    set /a FAILED_TESTS+=1
)
set /a TOTAL_TESTS+=1

echo Testing with no arguments...
python generate_mapping.py > nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo [PASS] Correctly failed with no arguments
    set /a PASSED_TESTS+=1
) else (
    echo [FAIL] Should have failed with no arguments
    set /a FAILED_TESTS+=1
)
set /a TOTAL_TESTS+=1

echo.
echo ========================================
echo FINAL SUMMARY
echo ========================================
echo Total Tests: %TOTAL_TESTS%
echo Passed: %PASSED_TESTS%
echo Failed: %FAILED_TESTS%
echo.

if %FAILED_TESTS% EQU 0 (
    echo ========================================
    echo ALL TESTS PASSED!
    echo ========================================
    exit /b 0
) else (
    echo ========================================
    echo SOME TESTS FAILED
    echo ========================================
    exit /b 1
)
