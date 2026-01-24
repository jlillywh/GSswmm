@echo off
REM ============================================================================
REM rebuild-tests.bat - Rebuild all test executables from source
REM Requires: Visual Studio Build Tools or Developer Command Prompt
REM ============================================================================
setlocal
cd /d "%~dp0..\tests"

echo ========================================
echo Rebuilding All Test Executables
echo ========================================
echo.

REM Check if cl.exe is available
where cl.exe >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: cl.exe not found in PATH
    echo.
    echo Please run this script from:
    echo   - Developer Command Prompt for VS 2022, or
    echo   - After running vcvars64.bat
    echo.
    echo Example:
    echo   "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
    echo   scripts\rebuild-tests.bat
    echo.
    endlocal
    exit /b 1
)

echo Compiler found: 
cl.exe 2>&1 | findstr /C:"Version"
echo.

set FAILED=0

REM Compile swmm_mock.cpp first (needed by all tests)
echo [1/7] Compiling swmm_mock.cpp...
cl /c /EHsc /std:c++17 /W3 /O2 swmm_mock.cpp >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo   [FAIL] swmm_mock.cpp compilation failed
    set FAILED=1
) else (
    echo   [PASS] swmm_mock.obj created
)
echo.

REM Test 1: Lifecycle
echo [2/7] Building test_lifecycle.exe...
cl /EHsc /std:c++17 /W3 /O2 test_lifecycle.cpp swmm_mock.obj /Fe:test_lifecycle.exe >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo   [FAIL] Build failed
    set FAILED=1
) else (
    echo   [PASS] test_lifecycle.exe created
)
echo.

REM Test 2: Calculate
echo [3/7] Building test_calculate.exe...
cl /EHsc /std:c++17 /W3 /O2 test_calculate.cpp swmm_mock.obj /Fe:test_calculate.exe >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo   [FAIL] Build failed
    set FAILED=1
) else (
    echo   [PASS] test_calculate.exe created
)
echo.

REM Test 3: Error Handling
echo [4/7] Building test_error_handling.exe...
cl /EHsc /std:c++17 /W3 /O2 test_error_handling.cpp swmm_mock.obj /Fe:test_error_handling.exe >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo   [FAIL] Build failed
    set FAILED=1
) else (
    echo   [PASS] test_error_handling.exe created
)
echo.

REM Test 4: File Validation
echo [5/7] Building test_file_validation.exe...
cl /EHsc /std:c++17 /W3 /O2 test_file_validation.cpp swmm_mock.obj /Fe:test_file_validation.exe >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo   [FAIL] Build failed
    set FAILED=1
) else (
    echo   [PASS] test_file_validation.exe created
)
echo.

REM Test 5: Subcatchment Validation
echo [6/7] Building test_subcatchment_validation.exe...
cl /EHsc /std:c++17 /W3 /O2 test_subcatchment_validation.cpp swmm_mock.obj /Fe:test_subcatchment_validation.exe >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo   [FAIL] Build failed
    set FAILED=1
) else (
    echo   [PASS] test_subcatchment_validation.exe created
)
echo.

REM Test 6: Out-of-Range Subcatchment
echo [7/8] Building test_subcatchment_out_of_range.exe...
cl /EHsc /std:c++17 /W3 /O2 test_subcatchment_out_of_range.cpp swmm_mock.obj /Fe:test_subcatchment_out_of_range.exe >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo   [FAIL] Build failed
    set FAILED=1
) else (
    echo   [PASS] test_subcatchment_out_of_range.exe created
)
echo.

REM Test 7: JSON Parsing (MappingLoader)
echo [7/8] Building test_json_parsing.exe...
cl /EHsc /std:c++17 /W3 /O2 /I.. test_json_parsing.cpp ..\MappingLoader.cpp /Fe:test_json_parsing.exe >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo   [FAIL] Build failed
    set FAILED=1
) else (
    echo   [PASS] test_json_parsing.exe created
)
echo.

REM Test 8: Integration E2E
echo [8/10] Building test_integration_e2e.exe...
cl /EHsc /std:c++17 /W3 /O2 test_integration_e2e.cpp swmm_mock.obj /Fe:test_integration_e2e.exe >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo   [FAIL] Build failed
    set FAILED=1
) else (
    echo   [PASS] test_integration_e2e.exe created
)
echo.

REM Test 9: Validate Mapping Simple
echo [9/10] Building test_validate_mapping_simple.exe...
cl /EHsc /std:c++17 /W3 /O2 /I.. test_validate_mapping_simple.cpp ..\MappingLoader.cpp /Fe:test_validate_mapping_simple.exe >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo   [FAIL] Build failed
    set FAILED=1
) else (
    echo   [PASS] test_validate_mapping_simple.exe created
)
echo.

REM Test 10: Validate Mapping
echo [10/10] Building test_validate_mapping.exe...
cl /EHsc /std:c++17 /W3 /O2 /I.. test_validate_mapping.cpp ..\MappingLoader.cpp /Fe:test_validate_mapping.exe >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo   [FAIL] Build failed
    set FAILED=1
) else (
    echo   [PASS] test_validate_mapping.exe created
)
echo.

echo ========================================
if %FAILED% EQU 0 (
    echo All Tests Rebuilt Successfully!
    echo ========================================
    echo.
    echo Run tests with: scripts\test.bat
    endlocal
    exit /b 0
) else (
    echo Some Tests Failed to Build
    echo ========================================
    echo.
    echo Check compiler errors above
    endlocal
    exit /b 1
)
