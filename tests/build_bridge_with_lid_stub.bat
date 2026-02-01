@echo off
REM Build bridge DLL with LID API stub for integration testing

echo ========================================
echo Building Bridge DLL with LID API Stub
echo ========================================
echo.

REM Find Visual Studio
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "%VSWHERE%" (
    for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
        set "VSINSTALLDIR=%%i"
    )
)

if not defined VSINSTALLDIR (
    set "VSINSTALLDIR=C:\Program Files\Microsoft Visual Studio\18\Community"
)

echo Using Visual Studio at: %VSINSTALLDIR%
echo Initializing build environment...
call "%VSINSTALLDIR%\VC\Auxiliary\Build\vcvars64.bat"

echo.
echo [1/3] Compiling LID API stub...
cl /c /EHsc /W3 /MD /DDLLEXPORT=__declspec(dllexport) /I.. swmm_lid_api_stub.cpp

if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Failed to compile LID API stub
    exit /b 1
)

echo [OK] LID API stub compiled
echo.

echo [2/3] Compiling bridge components...
cl /c /EHsc /W3 /MD /I.. ..\MappingLoader.cpp >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Failed to compile MappingLoader.cpp
    exit /b 1
)

cl /c /EHsc /W3 /MD /I.. ..\SwmmGoldSimBridge.cpp >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Failed to compile SwmmGoldSimBridge.cpp
    exit /b 1
)

echo [OK] Bridge components compiled
echo.

echo [3/3] Linking bridge DLL...
link /DLL /OUT:GSswmm.dll SwmmGoldSimBridge.obj MappingLoader.obj swmm_lid_api_stub.obj ..\lib\swmm5.lib >nul 2>&1

if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Failed to link bridge DLL
    exit /b 1
)

echo [OK] Bridge DLL created: GSswmm.dll
echo.
echo ========================================
echo Build Complete!
echo ========================================
