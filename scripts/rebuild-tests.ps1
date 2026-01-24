# ============================================================================
# rebuild-tests.ps1 - Rebuild all test executables using PowerShell
# Automatically finds and sets up Visual Studio environment
# ============================================================================

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Rebuilding All Test Executables" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Find Visual Studio installation
Write-Host "Searching for Visual Studio 2022..." -ForegroundColor Yellow

$vsPaths = @(
    "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat",
    "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat",
    "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat",
    "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
)

$vcvarsPath = $null
foreach ($path in $vsPaths) {
    if (Test-Path $path) {
        $vcvarsPath = $path
        Write-Host "Found: $path" -ForegroundColor Green
        break
    }
}

if (-not $vcvarsPath) {
    Write-Host "ERROR: Could not find Visual Studio 2022 installation" -ForegroundColor Red
    Write-Host ""
    Write-Host "Searched locations:" -ForegroundColor Yellow
    foreach ($path in $vsPaths) {
        Write-Host "  - $path" -ForegroundColor Gray
    }
    Write-Host ""
    Write-Host "Please install Visual Studio 2022 or run from Developer Command Prompt" -ForegroundColor Yellow
    exit 1
}

# Change to tests directory
$testsDir = Join-Path $PSScriptRoot "..\tests"
Push-Location $testsDir

Write-Host ""
Write-Host "Setting up compiler environment..." -ForegroundColor Yellow

# Create a temporary batch file to set up environment and compile
$tempBatch = [System.IO.Path]::GetTempFileName() + ".bat"

$batchContent = @"
@echo off
call "$vcvarsPath" >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Failed to set up Visual Studio environment
    exit /b 1
)

echo Compiler environment ready
echo.

REM Compile swmm_mock.cpp first
echo [1/8] Compiling swmm_mock.cpp...
cl /c /EHsc /std:c++17 /W3 /O2 swmm_mock.cpp >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo   [FAIL] swmm_mock.cpp compilation failed
    exit /b 1
)
echo   [PASS] swmm_mock.obj created

REM Test 1: Lifecycle
echo [2/8] Building test_lifecycle.exe...
cl /EHsc /std:c++17 /W3 /O2 test_lifecycle.cpp swmm_mock.obj /Fe:test_lifecycle.exe >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo   [FAIL] Build failed
    exit /b 1
)
echo   [PASS] test_lifecycle.exe created

REM Test 2: Calculate
echo [3/8] Building test_calculate.exe...
cl /EHsc /std:c++17 /W3 /O2 test_calculate.cpp swmm_mock.obj /Fe:test_calculate.exe >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo   [FAIL] Build failed
    exit /b 1
)
echo   [PASS] test_calculate.exe created

REM Test 3: Error Handling
echo [4/8] Building test_error_handling.exe...
cl /EHsc /std:c++17 /W3 /O2 test_error_handling.cpp swmm_mock.obj /Fe:test_error_handling.exe >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo   [FAIL] Build failed
    exit /b 1
)
echo   [PASS] test_error_handling.exe created

REM Test 4: File Validation
echo [5/8] Building test_file_validation.exe...
cl /EHsc /std:c++17 /W3 /O2 test_file_validation.cpp swmm_mock.obj /Fe:test_file_validation.exe >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo   [FAIL] Build failed
    exit /b 1
)
echo   [PASS] test_file_validation.exe created

REM Test 5: Subcatchment Validation
echo [6/8] Building test_subcatchment_validation.exe...
cl /EHsc /std:c++17 /W3 /O2 test_subcatchment_validation.cpp swmm_mock.obj /Fe:test_subcatchment_validation.exe >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo   [FAIL] Build failed
    exit /b 1
)
echo   [PASS] test_subcatchment_validation.exe created

REM Test 6: Out-of-Range Subcatchment
echo [7/8] Building test_subcatchment_out_of_range.exe...
cl /EHsc /std:c++17 /W3 /O2 test_subcatchment_out_of_range.cpp swmm_mock.obj /Fe:test_subcatchment_out_of_range.exe >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo   [FAIL] Build failed
    exit /b 1
)
echo   [PASS] test_subcatchment_out_of_range.exe created

REM Test 7: JSON Parsing
echo [8/10] Building test_json_parsing.exe...
cl /EHsc /std:c++17 /W3 /O2 /I.. test_json_parsing.cpp ..\MappingLoader.cpp /Fe:test_json_parsing.exe >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo   [FAIL] Build failed
    exit /b 1
)
echo   [PASS] test_json_parsing.exe created

REM Test 8: Validate Mapping Simple
echo [9/10] Building test_validate_mapping_simple.exe...
cl /EHsc /std:c++17 /W3 /O2 /I.. test_validate_mapping_simple.cpp ..\MappingLoader.cpp /Fe:test_validate_mapping_simple.exe >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo   [FAIL] Build failed
    exit /b 1
)
echo   [PASS] test_validate_mapping_simple.exe created

REM Test 9: Validate Mapping
echo [10/10] Building test_validate_mapping.exe...
cl /EHsc /std:c++17 /W3 /O2 /I.. test_validate_mapping.cpp ..\MappingLoader.cpp /Fe:test_validate_mapping.exe >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo   [FAIL] Build failed
    exit /b 1
)
echo   [PASS] test_validate_mapping.exe created

echo.
echo ========================================
echo All Tests Rebuilt Successfully!
echo ========================================
"@

Set-Content -Path $tempBatch -Value $batchContent

# Execute the batch file
& cmd /c $tempBatch

$exitCode = $LASTEXITCODE

# Clean up
Remove-Item $tempBatch -ErrorAction SilentlyContinue

Pop-Location

if ($exitCode -eq 0) {
    Write-Host ""
    Write-Host "Success! Run tests with: scripts\test.bat" -ForegroundColor Green
} else {
    Write-Host ""
    Write-Host "Build failed. Check errors above." -ForegroundColor Red
}

exit $exitCode
