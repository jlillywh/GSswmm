@echo off
REM ============================================================================
REM clean.bat - Remove generated files and build artifacts
REM ============================================================================
setlocal
cd /d "%~dp0.."

echo ========================================
echo Cleaning Project
echo ========================================
echo.

REM Clean test artifacts
echo Cleaning tests directory...
cd tests
if exist *.obj del /q *.obj
if exist model.out del /q model.out
if exist model.rpt del /q model.rpt
if exist bridge_debug.log del /q bridge_debug.log
echo   - Removed test artifacts

REM Clean root directory
cd ..
if exist bridge_debug.log del /q bridge_debug.log
echo   - Removed root artifacts

REM Clean temporary task summary files
echo Cleaning temporary task summaries...
if exist task_*_summary.md del /q task_*_summary.md
if exist checkpoint_*.md del /q checkpoint_*.md
echo   - Removed temporary task summaries

REM Clean build directories (optional - uncomment if needed)
REM echo Cleaning build directories...
REM if exist x64\Release\*.obj del /q x64\Release\*.obj
REM if exist GSswmm\x64\Release\*.obj del /q GSswmm\x64\Release\*.obj

echo.
echo ========================================
echo Clean Complete
echo ========================================
endlocal
