# LID API Extension - Test Infrastructure

This document describes the test infrastructure for the SWMM5 LID API extensions.

## Test Files

### Test Source Files

- **`test_lid_api.cpp`** - Unit tests for the three new LID API functions
  - Tests `swmm_getLidUCount()`
  - Tests `swmm_getLidUName()`
  - Tests `swmm_getLidUStorageVolume()`
  - Validates all requirements (1-7)

### Test Models

- **`lid_test_model.inp`** - SWMM model with LID units for testing
  - Copied from `examples/LID Treatment/LID_Model.inp`
  - Contains 8 LID deployments across 6 subcatchments
  - Includes various LID types: InfilTrench, RainBarrels, Planters, PorousPave, GreenRoof, Swale

### Build Scripts

- **`build_and_test_lid_api.bat`** - Compiles and runs LID API tests
  - Compiles `test_lid_api.cpp`
  - Links against `swmm5.lib`
  - Runs test executable
  - Reports pass/fail status

## Test Model Details

### LID Deployments in Test Model

| Subcatchment | LID Control | Count | Notes |
|--------------|-------------|-------|-------|
| S1 | InfilTrench | 4 | Has storage layer |
| S1 | RainBarrels | 32 | Has storage layer |
| S4 | Planters | 30 | Has storage layer |
| S5 | PorousPave | 1 | Has storage layer |
| S5 | GreenRoof | 1 | Has storage layer |
| Swale3 | Swale | 1 | No storage layer |
| Swale4 | Swale | 1 | No storage layer |
| Swale6 | Swale | 1 | No storage layer |

### Expected Test Results

**Subcatchment S1:**
- LID count: 2 (InfilTrench and RainBarrels)
- LID names: "InfilTrench", "RainBarrels"
- Storage volumes: > 0.0 (both have storage)

**Subcatchment S2:**
- LID count: 0 (no LID units)

**Subcatchment Swale3:**
- LID count: 1 (Swale)
- LID name: "Swale"
- Storage volume: 0.0 (swales have no storage layer)

## Test Coverage

### Requirements Coverage

| Requirement | Test Cases | Status |
|-------------|------------|--------|
| 1.1 - Count valid subcatchment | `CountLidUnits_MultipleUnits` | Ready |
| 1.2 - Count returns zero for no LIDs | `CountLidUnits_NoUnits` | Ready |
| 1.3 - Invalid index returns -1 | `CountLidUnits_InvalidIndex` | Ready |
| 1.4 - Available after swmm_start() | All tests use fixture | Ready |
| 2.1 - Get LID name valid indices | `GetLidName_ValidIndices` | Ready |
| 2.2 - Invalid indices set error | `GetLidName_InvalidIndices` | Ready |
| 2.3 - Respect buffer size | `GetLidName_BufferSize` | Ready |
| 2.4 - Null-terminate string | `GetLidName_NullTerminated` | Ready |
| 3.1 - Get storage volume | `GetStorageVolume_ValidUnit` | Ready |
| 3.2 - No storage returns zero | `GetStorageVolume_NoStorage` | Ready |
| 3.3 - Invalid indices return zero | `GetStorageVolume_InvalidIndices` | Ready |
| 3.4 - Reflects recent swmm_step() | `GetStorageVolume_ValidUnit` | Ready |
| 3.5 - Units match model config | `GetStorageVolume_UnitsConsistency` | Ready |
| 3.6 - Non-negative volumes | `GetStorageVolume_NonNegative` | Ready |
| 6.1 - Error before swmm_start() | `CallBeforeStart` | Ready |
| 6.2 - Invalid indices return errors | Multiple tests | Ready |
| 6.3 - Error via swmm_getError() | `ErrorMessages_Retrievable` | Ready |
| 7.1 - Naming convention | `NamingConvention` | Ready |
| 7.2 - Parameter ordering | `ParameterOrdering` | Ready |
| 7.3 - Return value conventions | `ReturnValueConventions` | Ready |

### Test Status

**Current Status:** Tests are written but commented out

**Reason:** The LID API functions (`swmm_getLidUCount`, `swmm_getLidUName`, `swmm_getLidUStorageVolume`) have not been implemented yet.

**Next Steps:**
1. Implement the three API functions in SWMM5 source (Task 2)
2. Uncomment the test assertions in `test_lid_api.cpp`
3. Build and run tests
4. Verify all tests pass

## Running Tests

### Prerequisites

1. SWMM5 DLL with LID API extensions built
2. `swmm5.dll` copied to `tests/` directory
3. `swmm5.h` with new function prototypes in `include/`
4. Visual Studio Developer Command Prompt

### Build and Run

```cmd
cd tests
build_and_test_lid_api.bat
```

### Expected Output (After Implementation)

```
========================================
Building LID API Tests
========================================
Microsoft (R) C/C++ Optimizing Compiler Version...
...
Microsoft (R) Incremental Linker Version...
...

========================================
Running LID API Tests
========================================

========================================
SWMM5 LID API Extension Tests
========================================

[==========] Running 18 tests from 3 test suites.
[----------] Global test environment set-up.
[----------] 15 tests from LidApiTest
[ RUN      ] LidApiTest.CountLidUnits_MultipleUnits
[       OK ] LidApiTest.CountLidUnits_MultipleUnits (10 ms)
[ RUN      ] LidApiTest.CountLidUnits_NoUnits
[       OK ] LidApiTest.CountLidUnits_NoUnits (8 ms)
...
[----------] 15 tests from LidApiTest (150 ms total)

[----------] 1 test from LidApiErrorTest
[ RUN      ] LidApiErrorTest.CallBeforeStart
[       OK ] LidApiErrorTest.CallBeforeStart (2 ms)
[----------] 1 test from LidApiErrorTest (2 ms total)

[----------] 3 tests from LidApiConsistencyTest
[ RUN      ] LidApiConsistencyTest.NamingConvention
[       OK ] LidApiConsistencyTest.NamingConvention (0 ms)
[ RUN      ] LidApiConsistencyTest.ParameterOrdering
[       OK ] LidApiConsistencyTest.ParameterOrdering (0 ms)
[ RUN      ] LidApiConsistencyTest.ReturnValueConventions
[       OK ] LidApiConsistencyTest.ReturnValueConventions (0 ms)
[----------] 3 tests from LidApiConsistencyTest (0 ms total)

[----------] Global test environment tear-down
[==========] 18 tests from 3 test suites ran. (152 ms total)
[  PASSED  ] 18 tests.

========================================
All LID API tests passed!
========================================
```

## Test Development Guidelines

### Adding New Tests

1. **Follow existing patterns** - Use the `LidApiTest` fixture for tests requiring SWMM model
2. **Test one thing** - Each test should verify a single behavior
3. **Use descriptive names** - Test names should clearly indicate what is being tested
4. **Document requirements** - Include requirement references in test comments
5. **Check error conditions** - Always test both success and failure paths

### Test Naming Convention

```cpp
TEST_F(LidApiTest, FunctionName_Scenario) {
    // Test implementation
}
```

Examples:
- `CountLidUnits_MultipleUnits` - Tests counting when multiple LIDs exist
- `GetLidName_InvalidIndices` - Tests error handling for invalid indices
- `GetStorageVolume_NonNegative` - Tests property that volume is always >= 0

### Assertion Guidelines

- Use `EXPECT_*` for non-fatal assertions (test continues)
- Use `ASSERT_*` for fatal assertions (test stops if fails)
- Use `ASSERT_*` for setup conditions (e.g., model opens successfully)
- Use `EXPECT_*` for actual test conditions

## Integration with Existing Tests

### Test Suite Organization

```
tests/
├── test_lifecycle.cpp              # Existing: DLL lifecycle tests
├── test_calculate.cpp              # Existing: Calculation tests
├── test_error_handling.cpp         # Existing: Error handling tests
├── test_lid_api.cpp               # NEW: LID API tests
└── run_all_tests.bat              # Run all test suites
```

### Running All Tests

```cmd
cd tests
run_all_tests.bat
```

This will run:
1. Lifecycle tests
2. Calculate tests
3. Error handling tests
4. File validation tests
5. Subcatchment validation tests
6. **LID API tests** (new)

## Troubleshooting

### Common Issues

**Issue:** `test_lid_api.exe` fails to build
- **Cause:** Missing `swmm5.h` with new function prototypes
- **Solution:** Copy updated header: `copy swmm5-source\src\swmm5.h include\`

**Issue:** Linker error: unresolved external symbol `swmm_getLidUCount`
- **Cause:** SWMM5 DLL not rebuilt with new functions
- **Solution:** Rebuild SWMM5 DLL and copy to tests directory

**Issue:** Test model not found
- **Cause:** `lid_test_model.inp` not in tests directory
- **Solution:** `copy "examples\LID Treatment\LID_Model.inp" tests\lid_test_model.inp`

**Issue:** All tests are skipped
- **Cause:** Test assertions are commented out (expected before implementation)
- **Solution:** Uncomment test code after implementing API functions

### Debug Tips

1. **Enable verbose output:** Add `--gtest_verbose` flag
2. **Run specific test:** `test_lid_api.exe --gtest_filter=LidApiTest.CountLidUnits_MultipleUnits`
3. **Check SWMM errors:** Look at generated `.rpt` file for SWMM-specific errors
4. **Verify model:** Run standalone SWMM5 on test model to ensure it's valid

## References

- **Requirements:** `.kiro/specs/lid-api-extension/requirements.md`
- **Design:** `.kiro/specs/lid-api-extension/design.md`
- **Setup Guide:** `tests/LID_API_SETUP.md`
- **Google Test Docs:** https://google.github.io/googletest/
- **SWMM5 API Docs:** https://www.epa.gov/water-research/storm-water-management-model-swmm
