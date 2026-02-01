@echo off
REM Build bridge DLL with LID API stub for integration testing
REM This creates a test version of GSswmm.dll that includes the stub

echo ========================================
echo Building Test Bridge DLL with LID Stub
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
echo.

REM Set up build environment
call "%VSINSTALLDIR%\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Failed to initialize Visual Studio environment
    exit /b 1
)

echo [1/4] Compiling LID API stub...
cl /c /EHsc /W3 /MD /DDLLEXPORT=__declspec(dllexport) /I.. swmm_lid_api_stub.cpp >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Failed to compile LID API stub
    exit /b 1
)
echo [OK] LID API stub compiled

echo [2/4] Compiling MappingLoader...
cl /c /EHsc /W3 /MD /I.. ..\MappingLoader.cpp >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Failed to compile MappingLoader
    exit /b 1
)
echo [OK] MappingLoader compiled

echo [3/4] Compiling SwmmGoldSimBridge...
cl /c /EHsc /W3 /MD /I.. ..\SwmmGoldSimBridge.cpp >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Failed to compile SwmmGoldSimBridge
    exit /b 1
)
echo [OK] SwmmGoldSimBridge compiled

echo [4/4] Linking test bridge DLL...
link /DLL /OUT:GSswmm.dll SwmmGoldSimBridge.obj MappingLoader.obj swmm_lid_api_stub.obj ..\lib\swmm5.lib kernel32.lib user32.lib msvcrt.lib msvcprt.lib >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Failed to link bridge DLL
    echo Trying with additional libraries...
    link /DLL /OUT:GSswmm.dll SwmmGoldSimBridge.obj MappingLoader.obj swmm_lid_api_stub.obj ..\lib\swmm5.lib kernel32.lib user32.lib
    if %ERRORLEVEL% NEQ 0 (
        echo ERROR: Link failed
        exit /b 1
    )
)
echo [OK] Bridge DLL created: GSswmm.dll

echo.
echo ========================================
echo Build Complete!
echo ========================================
echo Test DLL: tests\GSswmm.dll
echo.
