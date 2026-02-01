@echo off
REM Build script for LID overflow API test

echo Building LID overflow test...

REM Compile the stub
cl /c /EHsc /I..\include swmm_lid_api_stub.cpp /Fo:swmm_lid_api_stub.obj
if errorlevel 1 (
    echo Failed to compile stub
    exit /b 1
)

REM Compile the test
cl /c /EHsc /I..\include test_lid_overflow.cpp /Fo:test_lid_overflow.obj
if errorlevel 1 (
    echo Failed to compile test
    exit /b 1
)

REM Link
link /OUT:test_lid_overflow.exe test_lid_overflow.obj swmm_lid_api_stub.obj
if errorlevel 1 (
    echo Failed to link
    exit /b 1
)

echo Build successful!
echo.
echo Running test...
echo.
test_lid_overflow.exe

exit /b 0
