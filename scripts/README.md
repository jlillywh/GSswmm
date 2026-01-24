# Build and Test Scripts

Essential scripts for building, testing, and releasing the GoldSim-SWMM Bridge.

## Quick Reference

| Script | Purpose | Usage |
|--------|---------|-------|
| `test.bat` | Run all C++ tests | `scripts\test.bat` |
| `build.bat` | Build DLL only | `scripts\build.bat` |
| `build-and-test.bat` | Build + test | `scripts\build-and-test.bat` |
| `release.bat` | Full release pipeline | `scripts\release.bat [version]` |
| `clean.bat` | Remove artifacts | `scripts\clean.bat` |
| `set-version.bat` | Update version | `scripts\set-version.bat 1.01` |
| `rebuild-tests.bat` | Rebuild test executables | `scripts\rebuild-tests.bat` |
| `rebuild-tests-with-env.bat` | Rebuild tests (auto-setup VS env) | `scripts\rebuild-tests-with-env.bat` |
| `full-validation.bat` | Run all tests (Python + C++) | `scripts\full-validation.bat` |

## Detailed Usage

### test.bat
Runs all 8 C++ test suites and reports results.
```batch
scripts\test.bat
```
- Changes to tests/ directory
- Runs all test executables (lifecycle, calculate, error handling, file validation, subcatchment validation, out-of-range, JSON parsing, integration E2E)
- Returns to original directory
- Exit code 0 = all passed, 1 = some failed

### build.bat
Builds GSswmm.dll in Release x64 configuration.
```batch
scripts\build.bat
```
- Uses MSBuild
- Output: `x64\Release\GSswmm.dll`
- Exit code 0 = success, 1 = failed

### build-and-test.bat
Complete build and test cycle.
```batch
scripts\build-and-test.bat
```
Steps:
1. Build DLL
2. Copy to tests/ directory
3. Run all tests

### release.bat
Full release pipeline with optional version update.
```batch
scripts\release.bat          # Release without version change
scripts\release.bat 1.01     # Release with version 1.01
```
Steps:
1. Update version (if specified)
2. Clean test artifacts
3. Build DLL
4. Copy to tests
5. Run all tests

### clean.bat
Remove generated files and build artifacts.
```batch
scripts\clean.bat
```
Removes:
- Test artifacts (*.obj, model.out, model.rpt)
- Debug logs (bridge_debug.log)

### set-version.bat
Update version number in source code and documentation.
```batch
scripts\set-version.bat 1.01
```
Updates:
- SwmmGoldSimBridge.cpp (VERSION constant)
- README.md (version header and references)
- tests\test_lifecycle.cpp (version check)

**Important:** After updating the version, you must rebuild:
- DLL: `scripts\build.bat`
- Tests: `scripts\rebuild-tests-with-env.bat`

Or use `scripts\release.bat <version>` which does everything automatically.

### rebuild-tests.bat
Rebuild all test executables from source.
```batch
scripts\rebuild-tests.bat
```
**Requirements:** Must run from Developer Command Prompt or after vcvars64.bat

### rebuild-tests-with-env.bat
Rebuild tests with automatic Visual Studio environment setup.
```batch
scripts\rebuild-tests-with-env.bat
```
Automatically detects and sets up VS 2022 environment (BuildTools, Community, Professional, or Enterprise) and rebuilds all 8 test executables.

### full-validation.bat
Complete validation suite including Python parser tests, C++ DLL tests, and model validation.
```batch
scripts\full-validation.bat
```
Runs:
- Python tests (test_parser.py, test_hash.py, test_roundtrip_property.py)
- All C++ test executables
- Multiple model validation (test_model.inp, test_model_dummy.inp, etc.)
- Error case testing

Use this for comprehensive pre-release validation.

## Best Practices

### Development Workflow
1. Make code changes
2. Run `scripts\build-and-test.bat`
3. Fix any failures
4. Repeat until all tests pass

### Release Workflow
1. Update version: `scripts\set-version.bat 1.XX`
2. Run release: `scripts\release.bat`
3. Verify all tests pass
4. Distribute `x64\Release\GSswmm.dll`

### Testing Only
```batch
scripts\test.bat
```
Use when DLL is already built and you just want to verify tests.

### Clean Build
```batch
scripts\clean.bat
scripts\build.bat
```

## Notes

- All scripts use `setlocal` and return to original directory
- Scripts can be run from any directory
- MSBuild must be in PATH (Visual Studio required)
- Test executables must exist in tests/ directory

## Troubleshooting

**"MSBuild not found"**
- Install Visual Studio Build Tools
- Or run from Developer Command Prompt

**"Test executables not found"**
- Tests must be compiled first
- Check tests/ directory for *.exe files

**"DLL not found in tests"**
- Run `scripts\build-and-test.bat` to copy DLL
- Or manually copy: `xcopy x64\Release\GSswmm.dll tests\`

**Tests fail with "model.inp not found"**
- Normal for some tests (they create temporary files)
- Check tests/model.inp exists for full test suite
