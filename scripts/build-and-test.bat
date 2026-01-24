@echo off
REM ============================================================================
REM build-and-test.bat - Build DLL, copy to tests, and run all tests
REM ============================================================================
setlocal
cd /d "%~dp0"

echo ========================================
echo Build and Test Pipeline
echo ========================================
echo.

REM Step 1: Build
echo [1/3] Building GSswmm.dll...
call build.bat
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo Build failed. Aborting.
    exit /b 1
)

REM Step 2: Copy DLL to tests
echo.
echo [2/3] Copying DLL to tests directory...
cd /d "%~dp0.."
echo F | xcopy /Y x64\Release\GSswmm.dll tests\GSswmm.dll >nul
if %ERRORLEVEL% NEQ 0 (
    echo Failed to copy DLL
    exit /b 1
)
echo DLL copied successfully

REM Step 3: Run tests
echo.
echo [3/3] Running tests...
cd /d "%~dp0"
call test.bat
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo Tests failed.
    exit /b 1
)

echo.
echo ========================================
echo Build and Test Complete - All Passed!
echo ========================================
endlocal
exit /b 0
