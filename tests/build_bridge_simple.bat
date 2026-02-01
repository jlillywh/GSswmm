@echo off
REM Simple build script for bridge DLL with LID API stub

echo Building Bridge DLL with LID API Stub...
echo.

REM Compile LID API stub
echo [1/3] Compiling LID API stub...
cl /c /EHsc /W3 /MD /DDLLEXPORT=__declspec(dllexport) /I.. swmm_lid_api_stub.cpp
if %ERRORLEVEL% NEQ 0 exit /b 1

REM Compile MappingLoader
echo [2/3] Compiling MappingLoader...
cl /c /EHsc /W3 /MD /I.. ..\MappingLoader.cpp
if %ERRORLEVEL% NEQ 0 exit /b 1

REM Compile SwmmGoldSimBridge
echo [2/3] Compiling SwmmGoldSimBridge...
cl /c /EHsc /W3 /MD /I.. ..\SwmmGoldSimBridge.cpp
if %ERRORLEVEL% NEQ 0 exit /b 1

REM Link DLL
echo [3/3] Linking bridge DLL...
link /DLL /OUT:GSswmm.dll SwmmGoldSimBridge.obj MappingLoader.obj swmm_lid_api_stub.obj ..\lib\swmm5.lib
if %ERRORLEVEL% NEQ 0 exit /b 1

echo.
echo Build Complete! GSswmm.dll created.
