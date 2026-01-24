@echo off
REM ============================================================================
REM build.bat - Build GSswmm.dll (Release x64)
REM ============================================================================
setlocal
cd /d "%~dp0.."

echo ========================================
echo Building GSswmm.dll (Release x64)
echo ========================================
echo.

msbuild GSswmm.sln /p:Configuration=Release /p:Platform=x64 /t:Rebuild /v:minimal

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ========================================
    echo Build Successful!
    echo ========================================
    echo.
    echo Copying swmm5.dll dependency to output directory...
    echo F | xcopy /Y swmm5.dll x64\Release\swmm5.dll >nul
    echo.
    echo Output: x64\Release\GSswmm.dll
    echo.
    endlocal
    exit /b 0
) else (
    echo.
    echo ========================================
    echo Build Failed!
    echo ========================================
    echo.
    endlocal
    exit /b 1
)
