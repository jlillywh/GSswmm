@echo off
REM ============================================================================
REM release.bat - Full release pipeline: clean, build, test, package
REM Usage: release.bat [version]
REM ============================================================================
setlocal
cd /d "%~dp0"

echo ========================================
echo Release Pipeline
echo ========================================
echo.

REM Step 1: Update version if provided
if NOT "%~1"=="" (
    echo [1/5] Updating version to %~1...
    call set-version.bat %~1
    if %ERRORLEVEL% NEQ 0 (
        echo Version update failed
        exit /b 1
    )
    echo.
) else (
    echo [1/5] Skipping version update (no version specified)
    echo.
)

REM Step 2: Clean generated test files
echo [2/5] Cleaning test artifacts...
cd /d "%~dp0..\tests"
if exist model.out del /q model.out
if exist model.rpt del /q model.rpt
if exist bridge_debug.log del /q bridge_debug.log
if exist model.inp del /q model.inp
cd /d "%~dp0.."
if exist model.inp del /q model.inp
if exist SwmmGoldSimBridge.json del /q SwmmGoldSimBridge.json
echo Clean complete
echo.

REM Step 3: Build
echo [3/5] Building GSswmm.dll...
cd /d "%~dp0"
call build.bat
if %ERRORLEVEL% NEQ 0 (
    echo Build failed. Aborting release.
    exit /b 1
)
echo.

REM Step 3b: Rebuild test executables if version was updated
if NOT "%~1"=="" (
    echo [3b/5] Rebuilding test executables with new version...
    call rebuild-tests-with-env.bat
    if %ERRORLEVEL% NEQ 0 (
        echo Test rebuild failed. Aborting release.
        exit /b 1
    )
    echo.
)

REM Step 4: Copy DLL to tests
echo [4/5] Copying DLL to tests...
cd /d "%~dp0.."
echo F | xcopy /Y x64\Release\GSswmm.dll tests\GSswmm.dll >nul
echo F | xcopy /Y swmm5.dll tests\swmm5.dll >nul
echo DLL copied
echo.

REM Step 5: Run tests
echo [5/5] Running all tests...
cd /d "%~dp0"
call test.bat
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo Tests failed. Release aborted.
    exit /b 1
)

echo.
echo ========================================
echo Release Complete!
echo ========================================
echo.
echo Release artifacts:
echo   - x64\Release\GSswmm.dll
echo   - x64\Release\GSswmm.lib
echo   - x64\Release\GSswmm.pdb
echo.
echo All tests passed. Ready for distribution.
endlocal
exit /b 0
