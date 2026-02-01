@echo off
echo Building bridge LID direct test...

call "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" >nul 2>&1
if errorlevel 1 (
    call "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat" >nul 2>&1
)

cl /EHsc /Fe:test_bridge_lid_direct.exe test_bridge_lid_direct.cpp
if errorlevel 1 (
    echo Build failed!
    exit /b 1
)

echo Build successful!
echo Running test...
echo.
test_bridge_lid_direct.exe
