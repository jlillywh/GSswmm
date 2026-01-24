# GoldSim-SWMM Bridge Test Suite

This directory contains all test files for the GoldSim-SWMM Bridge DLL.

## Test Files

### Test Source Files (.cpp)
- `test_lifecycle.cpp` - Tests for DLL lifecycle (initialize, cleanup, version reporting)
- `test_calculate.cpp` - Tests for XF_CALCULATE handler and data exchange
- `test_error_handling.cpp` - Tests for error handling and reporting mechanisms
- `test_file_validation.cpp` - Tests for file path validation
- `test_subcatchment_validation.cpp` - Tests for subcatchment index validation
- `test_subcatchment_out_of_range.cpp` - Tests for out-of-range subcatchment indices

### Test Executables (.exe)
Compiled test executables corresponding to each .cpp file above.

### Build Scripts (.bat)
- `build_and_test.bat` - Build and run lifecycle tests
- `build_and_test_calculate.bat` - Build and run calculate tests
- `build_and_test_file_validation.bat` - Build and run file validation tests
- `build_and_test_subcatchment.bat` - Build and run subcatchment validation tests
- `run_all_tests.bat` - Run all test suites (recommended)

### Required Files
- `model.inp` - SWMM model input file required for tests (copied from parent directory)
- `GSswmm.dll` - The bridge DLL being tested (copied from parent directory)
- `swmm5.dll` - SWMM engine DLL (copied from parent directory)

### Generated Files
- `*.obj` - Object files from compilation
- `model.out` - SWMM output file generated during tests
- `model.rpt` - SWMM report file generated during tests

## Running Tests

To run all tests:
```
run_all_tests.bat
```

To run individual test suites:
```
test_lifecycle.exe
test_calculate.exe
test_error_handling.exe
test_file_validation.exe
test_subcatchment_validation.exe
test_subcatchment_out_of_range.exe
```

## Test Results

All 6 test suites with 32 total tests should pass:
- Lifecycle Tests: 7/7 passed
- Calculate Tests: 9/9 passed
- Error Handling Tests: 4/4 passed
- File Validation Tests: 3/3 passed
- Subcatchment Validation Tests: 5/5 passed
- Out-of-Range Subcatchment Tests: 4/4 passed

## Notes

- Some tests create and delete temporary model.inp files. If tests fail due to missing model.inp, copy it from the parent directory.
- Tests should be run from the tests directory.
- The Visual Studio environment must be set up (vcvars64.bat) for compilation.
