---
inclusion: always
---

# Project Organization Guidelines

## Directory Structure

This project follows a specific directory structure. Always respect these conventions:

### Source Files
- **Root directory**: Only source files that are part of the main build
  - `SwmmGoldSimBridge.cpp` - Main bridge implementation
  - `MappingLoader.cpp` - Mapping loader implementation
  - `generate_mapping.py` - Parser script
  - Build configuration files (`.sln`, `.vcxproj`, etc.)
  - Documentation (`README.md`)

### Test Files
- **tests/ directory**: ALL test-related files belong here
  - Test source files (`test_*.cpp`, `test_*.py`)
  - Test executables (`test_*.exe`)
  - Test object files (`test_*.obj`)
  - Test input files (`test_model_*.inp`)
  - Test build artifacts (`.exp`, `.lib` for test executables)
  - Test output files (`model.out`, `model.rpt` generated during tests)

### Build Artifacts
- **x64/ directory**: Build output from Visual Studio
- **GSswmm/ directory**: Intermediate build files
- **Root .obj files**: Should be avoided - configure build to output to x64/ or GSswmm/

### Other Directories
- **.kiro/**: Kiro configuration, specs, and steering documents
- **include/**: Header files
- **lib/**: External libraries
- **scripts/**: Build and utility scripts
- **docs/**: Documentation files

## Rules for File Creation

### When Creating Test Files

1. **Always create test source files in tests/ directory**
   ```
   ✅ CORRECT: tests/test_feature.cpp
   ❌ WRONG: test_feature.cpp (in root)
   ```

2. **Always compile test executables to tests/ directory**
   ```
   ✅ CORRECT: cl /Fe tests\test_feature.exe tests\test_feature.cpp ...
   ❌ WRONG: cl /Fe test_feature.exe tests\test_feature.cpp ...
   ```

3. **Test object files should go in tests/ directory**
   - Use `/Fo` flag to specify output directory: `/Fo tests\`

4. **Run tests from the tests/ directory when possible**
   - This keeps generated files (logs, output files) in the tests directory

### When Creating Temporary Files

1. **Use tests/ directory for test-related temporary files**
   - Test model files: `tests/test_model_*.inp`
   - Test mapping files: `tests/SwmmGoldSimBridge.json` (for testing)
   - Test logs: `tests/bridge_debug.log`

2. **Clean up temporary files after testing**
   - Remove generated `.obj`, `.exp`, `.lib` files after successful test runs
   - Keep only the `.exe` and source files

### Build Artifacts

1. **Main DLL output**: Root directory is acceptable
   - `GSswmm.dll` - Main output
   - `swmm5.dll` - Dependency

2. **Intermediate build files**: Should be in build directories
   - Object files: `x64/` or `GSswmm/`
   - Not in root directory

## Cleanup Checklist

Before committing or finishing work, verify:

- [ ] No test executables in root directory (should be in `tests/`)
- [ ] No test object files in root directory (should be in `tests/`)
- [ ] No test `.exp` or `.lib` files in root directory (should be in `tests/`)
- [ ] No orphaned `.obj` files in root directory
- [ ] Test input files are in `tests/` directory
- [ ] Temporary test output files are in `tests/` directory

## Common Mistakes to Avoid

1. **Compiling tests without specifying output directory**
   ```bash
   # ❌ WRONG - creates test_feature.exe in root
   cl /EHsc test_feature.cpp
   
   # ✅ CORRECT - creates test_feature.exe in tests/
   cl /EHsc /Fe tests\test_feature.exe tests\test_feature.cpp
   ```

2. **Creating test files in root then moving them**
   ```bash
   # ❌ WRONG - creates in root first
   echo "test" > test_model.inp
   move test_model.inp tests\
   
   # ✅ CORRECT - creates directly in tests/
   echo "test" > tests\test_model.inp
   ```

3. **Running tests from root without cleanup**
   ```bash
   # ❌ WRONG - leaves artifacts in root
   .\test_feature.exe
   
   # ✅ CORRECT - run from tests/ or specify paths
   .\tests\test_feature.exe
   ```

## File Organization by Type

### C++ Test Files
- **Location**: `tests/`
- **Naming**: `test_*.cpp`
- **Executables**: `tests/test_*.exe`
- **Objects**: `tests/test_*.obj` (or omit with proper build flags)

### Python Test Files
- **Location**: `tests/`
- **Naming**: `test_*.py`
- **Cache**: `tests/__pycache__/` (auto-generated)

### Test Input Files
- **Location**: `tests/`
- **Naming**: `test_model_*.inp` or `test_*.inp`

### Test Output Files
- **Location**: `tests/`
- **Examples**: `model.out`, `model.rpt`, `bridge_debug.log`

## Quick Reference

| File Type | Correct Location | Wrong Location |
|-----------|-----------------|----------------|
| Test source | `tests/test_*.cpp` | `test_*.cpp` |
| Test executable | `tests/test_*.exe` | `test_*.exe` |
| Test object | `tests/test_*.obj` | `test_*.obj` |
| Test input | `tests/test_*.inp` | `test_*.inp` |
| Main source | `SwmmGoldSimBridge.cpp` | `tests/SwmmGoldSimBridge.cpp` |
| Main DLL | `GSswmm.dll` | `tests/GSswmm.dll` |

## Enforcement

When you notice files in the wrong location:
1. Move them to the correct location
2. Update any references in build scripts or tests
3. Add the incorrect location to `.gitignore` if needed
4. Document the correction for future reference
