@echo off
REM ============================================================================
REM set-version.bat - Update version number in source code
REM Usage: set-version.bat <major.minor> (e.g., set-version.bat 1.01)
REM ============================================================================
setlocal

if "%~1"=="" (
    echo Usage: set-version.bat ^<version^>
    echo Example: set-version.bat 1.01
    echo.
    echo Current version in SwmmGoldSimBridge.cpp:
    findstr /C:"constexpr double VERSION" "%~dp0..\SwmmGoldSimBridge.cpp"
    endlocal
    exit /b 1
)

set NEW_VERSION=%~1
cd /d "%~dp0.."

echo ========================================
echo Updating Version to %NEW_VERSION%
echo ========================================
echo.

REM Update VERSION constant in SwmmGoldSimBridge.cpp
powershell -Command "(Get-Content SwmmGoldSimBridge.cpp) -replace 'constexpr double VERSION = [0-9.]+;', 'constexpr double VERSION = %NEW_VERSION%;' | Set-Content SwmmGoldSimBridge.cpp"

REM Update version header in README.md
powershell -Command "(Get-Content README.md) -replace '## Version [0-9.]+ -', '## Version %NEW_VERSION% -' | Set-Content README.md"

REM Update version reference in README.md (Version: field)
powershell -Command "(Get-Content README.md) -replace 'Version:        [0-9.]+', 'Version:        %NEW_VERSION%' | Set-Content README.md"

REM Update version in validation section of README.md
powershell -Command "(Get-Content README.md) -replace 'External function version: [0-9.]+', 'External function version: %NEW_VERSION%' | Set-Content README.md"

REM Update hardcoded version in test_lifecycle.cpp
powershell -Command "(Get-Content tests\test_lifecycle.cpp) -replace 'outargs\[0\] == [0-9.]+\)', 'outargs[0] == %NEW_VERSION%)' | Set-Content tests\test_lifecycle.cpp"
powershell -Command "(Get-Content tests\test_lifecycle.cpp) -replace 'Expected version [0-9.]+', 'Expected version %NEW_VERSION%' | Set-Content tests\test_lifecycle.cpp"

echo Version updated to %NEW_VERSION%
echo.
echo Updated files:
echo   - SwmmGoldSimBridge.cpp (VERSION constant)
echo   - README.md (version header and references)
echo   - tests\test_lifecycle.cpp (version check)
echo.
echo IMPORTANT: You must rebuild the DLL and test executables for changes to take effect!
echo   Run: scripts\build.bat
echo   Run: scripts\rebuild-tests-with-env.bat
endlocal
