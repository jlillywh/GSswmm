@echo off
REM ============================================================================
REM rebuild-tests-with-env.bat - Setup VS environment and rebuild tests
REM ============================================================================
setlocal enabledelayedexpansion

echo Setting up Visual Studio environment...

REM Try common Visual Studio installation paths
set "VCVARS_FOUND=0"

REM VS 2022 Build Tools
set "VCVARS_PATH=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
if exist "!VCVARS_PATH!" (
    call "!VCVARS_PATH!" >nul 2>&1
    set "VCVARS_FOUND=1"
)

REM VS 2022 Community
if "!VCVARS_FOUND!"=="0" (
    set "VCVARS_PATH=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
    if exist "!VCVARS_PATH!" (
        call "!VCVARS_PATH!" >nul 2>&1
        set "VCVARS_FOUND=1"
    )
)

REM VS 2022 Professional
if "!VCVARS_FOUND!"=="0" (
    set "VCVARS_PATH=C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat"
    if exist "!VCVARS_PATH!" (
        call "!VCVARS_PATH!" >nul 2>&1
        set "VCVARS_FOUND=1"
    )
)

REM VS 2022 Enterprise
if "!VCVARS_FOUND!"=="0" (
    set "VCVARS_PATH=C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
    if exist "!VCVARS_PATH!" (
        call "!VCVARS_PATH!" >nul 2>&1
        set "VCVARS_FOUND=1"
    )
)

if "!VCVARS_FOUND!"=="0" (
    echo ERROR: Could not find Visual Studio 2022 installation
    echo.
    echo Searched locations:
    echo   - C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools
    echo   - C:\Program Files\Microsoft Visual Studio\2022\Community
    echo   - C:\Program Files\Microsoft Visual Studio\2022\Professional
    echo   - C:\Program Files\Microsoft Visual Studio\2022\Enterprise
    echo.
    echo Please run from Developer Command Prompt or install Visual Studio 2022
    endlocal
    exit /b 1
)

echo Environment ready
echo.

REM Call the rebuild script
call "%~dp0rebuild-tests.bat"

endlocal
exit /b %ERRORLEVEL%
