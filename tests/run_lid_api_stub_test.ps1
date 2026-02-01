# PowerShell script to build and run LID API stub tests
# This works without requiring Visual Studio environment setup

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Building LID API Tests (with stubs)" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Find Visual Studio installation
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vswhere)) {
    Write-Host "ERROR: Visual Studio not found" -ForegroundColor Red
    exit 1
}

$vsPath = & $vswhere -latest -property installationPath
if (-not $vsPath) {
    Write-Host "ERROR: Could not find Visual Studio installation" -ForegroundColor Red
    exit 1
}

# Set up Visual Studio environment
$vcvarsPath = Join-Path $vsPath "VC\Auxiliary\Build\vcvars64.bat"
if (-not (Test-Path $vcvarsPath)) {
    Write-Host "ERROR: vcvars64.bat not found at $vcvarsPath" -ForegroundColor Red
    exit 1
}

Write-Host "Setting up Visual Studio environment..." -ForegroundColor Yellow

# Create a temporary batch file to set up environment and compile
$tempBat = [System.IO.Path]::GetTempFileName() + ".bat"
@"
@echo off
call "$vcvarsPath" >nul 2>&1
cd /d "$PWD"
cl /EHsc /std:c++17 /I..\include test_lid_api.cpp swmm_lid_api_stub.cpp swmm_mock.cpp /link /OUT:test_lid_api_stub.exe
"@ | Out-File -FilePath $tempBat -Encoding ASCII

# Run the batch file
$process = Start-Process -FilePath "cmd.exe" -ArgumentList "/c `"$tempBat`"" -Wait -PassThru -NoNewWindow

Remove-Item $tempBat

if ($process.ExitCode -ne 0) {
    Write-Host ""
    Write-Host "Build failed!" -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Running LID API Tests" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Run tests
if (Test-Path "test_lid_api_stub.exe") {
    & .\test_lid_api_stub.exe
    $testResult = $LASTEXITCODE
    
    Write-Host ""
    if ($testResult -eq 0) {
        Write-Host "========================================" -ForegroundColor Green
        Write-Host "All LID API tests passed!" -ForegroundColor Green
        Write-Host "========================================" -ForegroundColor Green
    } else {
        Write-Host "========================================" -ForegroundColor Red
        Write-Host "Some tests failed!" -ForegroundColor Red
        Write-Host "========================================" -ForegroundColor Red
    }
    
    exit $testResult
} else {
    Write-Host "ERROR: test_lid_api_stub.exe not found" -ForegroundColor Red
    exit 1
}
