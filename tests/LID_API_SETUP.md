# LID API Extension - Development Environment Setup

This document describes the setup required to implement and test the SWMM5 LID API extensions.

## Prerequisites

- **Visual Studio 2022** (or Visual Studio 2019 with C++17 support)
- **Windows SDK** (included with Visual Studio)
- **Git** (for cloning SWMM5 source)
- **Python 3.x** (for mapping generator tests)

## 1. SWMM5 Source Code (v5.2.4)

### Download Options

**Option A: Official EPA Release (Recommended)**
```bash
# Clone the official EPA SWMM repository
git clone https://github.com/USEPA/Stormwater-Management-Model.git swmm5-source
cd swmm5-source
git checkout v5.2.4
```

**Option B: Direct Download**
- Visit: https://github.com/USEPA/Stormwater-Management-Model/releases/tag/v5.2.4
- Download: Source code (zip)
- Extract to: `swmm5-source/`

### SWMM5 Directory Structure

After download, you should have:
```
swmm5-source/
├── src/
│   ├── swmm5.h          # API header (we'll modify this)
│   ├── lid.c            # LID module (we'll add functions here)
│   ├── lid.h            # LID internal header
│   ├── globals.h        # Global declarations
│   └── ... (other SWMM source files)
├── build/
│   └── Windows/         # Visual Studio project files
└── README.md
```

## 2. Build Environment Setup

### Visual Studio Configuration

1. **Open Developer Command Prompt**
   - Start Menu → Visual Studio 2022 → Developer Command Prompt for VS 2022
   - Or run: `"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"`

2. **Verify Compiler**
   ```cmd
   cl
   ```
   Should display: Microsoft (R) C/C++ Optimizing Compiler Version...

### SWMM5 DLL Build

Navigate to SWMM5 source and build:

```cmd
cd swmm5-source\build\Windows
msbuild swmm5.sln /p:Configuration=Release /p:Platform=x64
```

**Output:** `swmm5-source\build\Windows\x64\Release\swmm5.dll`

**Copy to project:**
```cmd
copy swmm5-source\build\Windows\x64\Release\swmm5.dll tests\
copy swmm5-source\src\swmm5.h include\
```

## 3. Test Model with LID Units

### Existing Test Model

The project already includes a comprehensive LID test model:

**Location:** `examples/LID Treatment/LID_Model.inp`

**LID Units Included:**
- **S1**: InfilTrench (4 units), RainBarrels (32 units)
- **S4**: Planters (30 units)
- **S5**: PorousPave (1 unit), GreenRoof (1 unit)
- **Swale3, Swale4, Swale6**: Swale (1 unit each)

**Total:** 8 LID deployments across 6 subcatchments

### Copy Test Model to Tests Directory

```cmd
copy "examples\LID Treatment\LID_Model.inp" tests\lid_test_model.inp
```

### Verify Test Model

Run SWMM5 standalone to verify the model works:

```cmd
cd tests
swmm5.exe lid_test_model.inp lid_test.rpt lid_test.out
```

Check `lid_test.rpt` for LID Performance Summary section.

## 4. Google Test Framework

### Current Setup

The project already has Google Test configured:

**Test Infrastructure:**
- `tests/gtest_minimal.h` - Minimal Google Test header
- `tests/rapidcheck_minimal.h` - Property-based testing support
- Multiple existing test files demonstrating patterns

### Test Compilation Pattern

Tests are compiled using this pattern (from existing build scripts):

```cmd
cl /EHsc /std:c++17 /I..\include ^
   test_lid_api.cpp ^
   /link /LIBPATH:..\lib swmm5.lib ^
   /OUT:test_lid_api.exe
```

### Verify Test Framework

Run existing tests to verify setup:

```cmd
cd tests
run_all_tests.bat
```

All tests should pass, confirming the build environment is working.

## 5. Development Workflow

### Typical Development Cycle

1. **Modify SWMM5 Source**
   - Edit `swmm5-source/src/swmm5.h` (add prototypes)
   - Edit `swmm5-source/src/lid.c` (implement functions)

2. **Rebuild SWMM5 DLL**
   ```cmd
   cd swmm5-source\build\Windows
   msbuild swmm5.sln /p:Configuration=Release /p:Platform=x64
   ```

3. **Copy Updated DLL**
   ```cmd
   copy swmm5-source\build\Windows\x64\Release\swmm5.dll tests\
   copy swmm5-source\src\swmm5.h include\
   ```

4. **Write/Update Tests**
   - Create test file in `tests/`
   - Follow existing test patterns

5. **Build and Run Tests**
   ```cmd
   cd tests
   build_and_test_lid_api.bat
   ```

6. **Update Bridge Code**
   - Modify `SwmmGoldSimBridge.cpp`
   - Rebuild bridge DLL

7. **Integration Testing**
   - Test with LID example model
   - Verify end-to-end functionality

## 6. Project Structure After Setup

```
GSswmm/
├── swmm5-source/              # EPA SWMM5 source code (v5.2.4)
│   ├── src/
│   │   ├── swmm5.h           # Modified with LID API
│   │   ├── lid.c             # Modified with new functions
│   │   └── ...
│   └── build/Windows/
│       └── x64/Release/
│           └── swmm5.dll     # Built with LID extensions
├── include/
│   └── swmm5.h               # Copy of modified header
├── tests/
│   ├── swmm5.dll             # Copy of built DLL
│   ├── lid_test_model.inp    # LID test model
│   ├── test_lid_api.cpp      # LID API tests (to be created)
│   └── ...
├── examples/
│   └── LID Treatment/
│       └── LID_Model.inp     # Original LID example
└── ...
```

## 7. Troubleshooting

### Common Issues

**Issue:** `swmm5.dll not found`
- **Solution:** Copy DLL to tests directory: `copy swmm5-source\build\Windows\x64\Release\swmm5.dll tests\`

**Issue:** `Cannot open include file: 'swmm5.h'`
- **Solution:** Verify include path: `/I..\include` in compile command

**Issue:** `unresolved external symbol swmm_getLidUCount`
- **Solution:** Rebuild swmm5.dll after adding new functions

**Issue:** Test model fails to open
- **Solution:** Verify model path and that all required files are present

### Verification Checklist

- [ ] Visual Studio 2022 installed
- [ ] Developer Command Prompt opens successfully
- [ ] SWMM5 v5.2.4 source downloaded
- [ ] SWMM5 DLL builds successfully
- [ ] Test model (LID_Model.inp) copied to tests/
- [ ] Existing tests run successfully
- [ ] swmm5.dll and swmm5.h copied to project

## 8. Next Steps

After completing this setup:

1. **Task 2:** Implement SWMM5 API extensions
   - Add function prototypes to `swmm5.h`
   - Implement functions in `lid.c`
   - Write unit tests

2. **Task 3:** Build and test SWMM5 DLL
   - Compile with new functions
   - Run unit tests
   - Verify with LID example model

3. **Task 5:** Implement bridge composite ID support
   - Extend bridge data structures
   - Add parsing logic
   - Update output retrieval

## References

- **EPA SWMM5 Repository:** https://github.com/USEPA/Stormwater-Management-Model
- **SWMM5 API Documentation:** https://www.epa.gov/water-research/storm-water-management-model-swmm
- **Google Test Documentation:** https://google.github.io/googletest/
- **Project Requirements:** `.kiro/specs/lid-api-extension/requirements.md`
- **Project Design:** `.kiro/specs/lid-api-extension/design.md`
