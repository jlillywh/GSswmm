# PowerShell script to build and run the end-to-end LID integration test

Write-Host "=== End-to-End LID Integration Test ===" -ForegroundColor Cyan
Write-Host ""

# Find MSBuild
Write-Host "Looking for MSBuild..." -ForegroundColor Yellow
$msbuild = Get-ChildItem "C:\Program Files\Microsoft Visual Studio" -Recurse -Filter "MSBuild.exe" -ErrorAction SilentlyContinue | 
           Select-Object -First 1 -ExpandProperty FullName

if (-not $msbuild) {
    Write-Host "ERROR: MSBuild not found. Please install Visual Studio 2022." -ForegroundColor Red
    exit 1
}

Write-Host "Found MSBuild: $msbuild" -ForegroundColor Green
Write-Host ""

# Find vcvars64.bat
Write-Host "Looking for Visual Studio environment setup..." -ForegroundColor Yellow
$vsInstallPath = Split-Path (Split-Path (Split-Path (Split-Path $msbuild)))
$vcvars = Join-Path $vsInstallPath "VC\Auxiliary\Build\vcvars64.bat"

if (-not (Test-Path $vcvars)) {
    Write-Host "ERROR: vcvars64.bat not found at $vcvars" -ForegroundColor Red
    exit 1
}

Write-Host "Found vcvars64.bat: $vcvars" -ForegroundColor Green
Write-Host ""

# Check if DLL exists
Write-Host "Checking for GSswmm.dll..." -ForegroundColor Yellow
if (-not (Test-Path "GSswmm.dll")) {
    Write-Host "ERROR: GSswmm.dll not found in tests directory" -ForegroundColor Red
    Write-Host "Please build the DLL first" -ForegroundColor Red
    exit 1
}

Write-Host "Found GSswmm.dll" -ForegroundColor Green
Write-Host ""

# Check if LID example model exists
Write-Host "Checking for LID example model..." -ForegroundColor Yellow
$lidModel = "..\examples\LID Treatment\LID_Model.inp"
if (-not (Test-Path $lidModel)) {
    Write-Host "ERROR: LID_Model.inp not found at $lidModel" -ForegroundColor Red
    exit 1
}

Write-Host "Found LID_Model.inp" -ForegroundColor Green
Write-Host ""

# Check if generate_mapping.py exists
Write-Host "Checking for mapping generator..." -ForegroundColor Yellow
if (-not (Test-Path "..\generate_mapping.py")) {
    Write-Host "ERROR: generate_mapping.py not found" -ForegroundColor Red
    exit 1
}

Write-Host "Found generate_mapping.py" -ForegroundColor Green
Write-Host ""

# Compile the test
Write-Host "Compiling end-to-end test..." -ForegroundColor Yellow
Write-Host ""

# Create a temporary batch file to set up environment and compile
$tempBat = "temp_compile_e2e.bat"
@"
@echo off
call "$vcvars" >nul 2>&1
cl /EHsc /W3 /MD /I..\include test_e2e_lid_integration.cpp /link /OUT:test_e2e_lid_integration.exe
exit /b %ERRORLEVEL%
"@ | Out-File -FilePath $tempBat -Encoding ASCII

# Run the compilation
$compileResult = & cmd /c $tempBat
$compileExitCode = $LASTEXITCODE

# Clean up temp file
Remove-Item $tempBat -ErrorAction SilentlyContinue

if ($compileExitCode -ne 0) {
    Write-Host "ERROR: Compilation failed" -ForegroundColor Red
    exit 1
}

Write-Host "Compilation successful" -ForegroundColor Green
Write-Host ""

# Run the test
Write-Host "=== Running End-to-End Test ===" -ForegroundColor Cyan
Write-Host ""

$testResult = & .\test_e2e_lid_integration.exe
$testExitCode = $LASTEXITCODE

Write-Host ""
Write-Host "=== Test Complete ===" -ForegroundColor Cyan

if ($testExitCode -eq 0) {
    Write-Host "ALL TESTS PASSED!" -ForegroundColor Green
    exit 0
} else {
    Write-Host "SOME TESTS FAILED!" -ForegroundColor Red
    exit 1
}
