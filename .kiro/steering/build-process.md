---
inclusion: always
---

# Build Process Guidelines

This document describes the complete build process for the GoldSim-SWMM Bridge project. Follow these guidelines whenever building, testing, or releasing the DLL.

## Critical Build Rules

### 1. DLL Distribution to Tests Directory

**CRITICAL:** After every build of `GSswmm.dll`, you MUST copy it to the `tests/` directory.

```batch
# After building the main DLL
copy /Y x64\Release\GSswmm.dll tests\GSswmm.dll
copy /Y swmm5.dll tests\swmm5.dll
```

**Why:** Test executables in the `tests/` directory load `GSswmm.dll` from their local directory. If you don't copy the latest DLL, tests will run against an outdated version, causing version mismatches and incorrect test results.

**When to do this:**
- After running `scripts\build.bat`
- After building through Visual Studio
- Before running `scripts\test.bat`
- Before running any individual test executable

### 2. Build Output Locations

The project uses Visual Studio with MSBuild. Build outputs go to specific locations:

**Main DLL Build:**
- Source: `SwmmGoldSimBridge.cpp`, `MappingLoader.cpp`
- Output: `x64\Release\GSswmm.dll`
- Intermediate: `x64\Release\*.obj`, `GSswmm\x64\Release\*.obj`
- Also produces: `x64\Release\GSswmm.lib`, `x64\Release\GSswmm.pdb`

**Test Executables:**
- Source: `tests\test_*.cpp`
- Output: `tests\test_*.exe`
- Intermediate: `tests\*.obj` (should be cleaned up)

**Dependencies:**
- `swmm5.dll` - EPA SWMM engine (must be in same directory as GSswmm.dll)
- `swmm5.lib` - Link library for SWMM API

## Build Scripts

### scripts/build.bat

Builds the main DLL in Release x64 configuration.

```batch
scripts\build.bat
```

**What it does:**
1. Runs MSBuild on `GSswmm.sln`
2. Builds `x64\Release\GSswmm.dll`
3. Copies `swmm5.dll` to output directory

**What it does NOT do:**
- Does NOT copy DLL to tests directory (you must do this manually)
- Does NOT rebuild test executables

**After running build.bat, you MUST:**
```batch
copy /Y x64\Release\GSswmm.dll tests\GSswmm.dll
copy /Y swmm5.dll tests\swmm5.dll
```

### scripts/rebuild-tests.bat

Rebuilds all test executables from source.

```batch
scripts\rebuild-tests.bat
```

**Requirements:**
- Must run from Visual Studio Developer Command Prompt, OR
- Must have `cl.exe` in PATH

**What it does:**
1. Compiles `swmm_mock.cpp` (mock SWMM API for testing)
2. Compiles each `test_*.cpp` file
3. Links with `swmm_mock.obj` to create `test_*.exe`

**When to run:**
- After modifying any test source file
- After changing version number in code
- After modifying `SwmmGoldSimBridge.cpp` interface

### scripts/rebuild-tests-with-env.bat

Same as `rebuild-tests.bat` but automatically sets up Visual Studio environment.

```batch
scripts\rebuild-tests-with-env.bat
```

**What it does:**
1. Searches for Visual Studio 2022 installation
2. Runs `vcvars64.bat` to set up compiler environment
3. Calls `rebuild-tests.bat`

**Use this when:**
- Running from regular command prompt (not VS Developer Command Prompt)
- You don't have `cl.exe` in PATH

### scripts/test.bat

Runs all test executables and reports results.

```batch
scripts\test.bat
```

**Prerequisites:**
- Test executables must exist in `tests/` directory
- `tests/GSswmm.dll` must be the latest version
- `tests/swmm5.dll` must exist

**What it does:**
1. Runs each `test_*.exe` in sequence
2. Reports pass/fail for each test suite
3. Provides summary of results

### scripts/release.bat

Complete release pipeline: clean, build, test, package.

```batch
scripts\release.bat [version]
```

**What it does:**
1. Updates version number (if provided)
2. Cleans test artifacts
3. Builds main DLL
4. Rebuilds test executables (if version changed)
5. **Copies DLL to tests directory** ✅
6. Runs all tests
7. Reports results

**This is the ONLY script that automatically copies DLL to tests directory.**

### scripts/clean.bat

Removes generated files and build artifacts.

```batch
scripts\clean.bat
```

**What it does:**
- Removes `*.obj` files from tests directory
- Removes test output files (`model.out`, `model.rpt`, `bridge_debug.log`)
- Removes temporary task summary files

**What it does NOT do:**
- Does NOT remove test executables
- Does NOT remove DLL files
- Does NOT clean Visual Studio build directories

## Complete Build Workflows

### Workflow 1: Build and Test (Manual)

```batch
# 1. Build the main DLL
scripts\build.bat

# 2. Copy DLL to tests (CRITICAL!)
copy /Y x64\Release\GSswmm.dll tests\GSswmm.dll
copy /Y swmm5.dll tests\swmm5.dll

# 3. Run tests
scripts\test.bat
```

### Workflow 2: Build and Test (Automated)

```batch
# Use release script - it handles everything
scripts\release.bat
```

### Workflow 3: Rebuild Tests Only

```batch
# If you only changed test code, not the DLL
scripts\rebuild-tests-with-env.bat

# Then run tests
scripts\test.bat
```

### Workflow 4: After Code Changes

```batch
# 1. Build main DLL
scripts\build.bat

# 2. Copy to tests (CRITICAL!)
copy /Y x64\Release\GSswmm.dll tests\GSswmm.dll

# 3. If you changed the interface or version, rebuild tests
scripts\rebuild-tests-with-env.bat

# 4. Run tests
scripts\test.bat
```

### Workflow 5: Version Update

```batch
# Use release script with version number
scripts\release.bat 1.04

# This will:
# - Update version in code
# - Build DLL
# - Rebuild tests with new version
# - Copy DLL to tests
# - Run all tests
```

## Common Build Issues

### Issue 1: Version Mismatch in Tests

**Symptom:** Test reports "expected version X.XX, got version Y.YY"

**Cause:** `tests/GSswmm.dll` is outdated

**Fix:**
```batch
copy /Y x64\Release\GSswmm.dll tests\GSswmm.dll
```

### Issue 2: Tests Fail After Code Changes

**Symptom:** Tests pass in Visual Studio but fail when running test.bat

**Cause:** Test executables are loading old DLL from tests directory

**Fix:**
```batch
# Copy latest DLL
copy /Y x64\Release\GSswmm.dll tests\GSswmm.dll

# If interface changed, rebuild tests
scripts\rebuild-tests-with-env.bat
```

### Issue 3: "cl.exe not found"

**Symptom:** rebuild-tests.bat fails with "cl.exe not found"

**Fix:** Use the environment setup script:
```batch
scripts\rebuild-tests-with-env.bat
```

Or run from Visual Studio Developer Command Prompt.

### Issue 4: Test Executable Out of Date

**Symptom:** Test behavior doesn't match source code changes

**Cause:** Test executable wasn't rebuilt after source changes

**Fix:**
```batch
scripts\rebuild-tests-with-env.bat
```

## Build Checklist

Before committing code or running tests, verify:

- [ ] Main DLL built successfully (`x64\Release\GSswmm.dll` exists)
- [ ] **DLL copied to tests directory** (`tests\GSswmm.dll` matches `x64\Release\GSswmm.dll`)
- [ ] `swmm5.dll` copied to tests directory
- [ ] Test executables rebuilt if interface changed
- [ ] All tests pass (`scripts\test.bat` returns 0)
- [ ] No orphaned `.obj` files in root or tests directory
- [ ] Version number consistent across code, tests, and documentation

## Quick Reference

| Task | Command | Copies DLL to Tests? |
|------|---------|---------------------|
| Build main DLL | `scripts\build.bat` | ❌ NO - Manual copy required |
| Rebuild tests | `scripts\rebuild-tests-with-env.bat` | ❌ NO |
| Run tests | `scripts\test.bat` | ❌ NO |
| Full release | `scripts\release.bat` | ✅ YES |
| Clean artifacts | `scripts\clean.bat` | N/A |

## Remember

**The #1 rule:** After building `GSswmm.dll`, always copy it to `tests/GSswmm.dll` before running tests.

**Why this matters:** Test executables load the DLL from their local directory. If you forget to copy the latest DLL, you'll be testing against an old version, leading to confusing failures and version mismatches.

**Best practice:** Use `scripts\release.bat` for complete build-test cycles. It handles all the copying automatically.
