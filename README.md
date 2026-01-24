# GoldSim-SWMM Bridge

A DLL bridge that enables GoldSim simulation software to control EPA SWMM hydraulic simulations, pass rainfall inputs, and receive runoff calculations.

## Version 1.04 - Enhanced Validation and Visualization

**New in 1.04:**
- ✅ **Input file validation** in `generate_mapping.py` catches common errors before runtime
- ✅ **Visual diagram generation** with `generate_diagram.py` creates Mermaid.js diagrams
- ✅ **Performance optimization** - logging disabled by default for faster simulations
- ✅ **Enhanced error reporting** - detailed SWMM error messages in logs

**Previous (1.03):** Dynamic interface configuration through mapping files

## Quick Start

### Required Files

**All files in your GoldSim model directory:**
- `YourModel.gsm` - Your GoldSim model
- `model.inp` - SWMM input file
- `SwmmGoldSimBridge.json` - Interface mapping file (generated from model.inp)
- `GSswmm.dll` - Bridge DLL (from x64/Release/)
- `swmm5.dll` - SWMM engine (MUST be in same directory as GSswmm.dll)

**Output files** (model.out, model.rpt, bridge_debug.log) are automatically created in the same directory during simulation.

### Generating the Mapping File

Before using the bridge, generate the mapping file from your SWMM model:

```batch
python generate_mapping.py model.inp
```

This creates `SwmmGoldSimBridge.json` which defines:
- Number of inputs (elapsed time + rain gages with TIMESERIES DUMMY)
- Number of outputs (storage nodes, outfalls, orifices, weirs, subcatchments)
- Element names and types for dynamic interface

**The script also validates your .inp file** and will report any errors that would prevent SWMM from running:
- Missing required sections
- Invalid XSECTIONS parameters (e.g., weirs with wrong parameter count)
- References to non-existent nodes
- Missing outfall nodes

**Current Input Support:**
- Elapsed time (always included)
- Rain gages with `TIMESERIES DUMMY` in the [RAINGAGES] section

**Note:** The current version only supports rain gages as dynamic inputs. To control other SWMM elements (pumps, valves, etc.) from GoldSim, you would need to extend the parser and bridge implementation.

**Important:** Regenerate the mapping file whenever you modify your SWMM model structure.

#### Python Script Usage

**Basic Usage:**
```batch
python generate_mapping.py <path_to_inp_file>
```

**Examples:**
```batch
# Generate mapping from model.inp in current directory
python generate_mapping.py model.inp

# Generate mapping from model in subdirectory
python generate_mapping.py models\watershed.inp

# Generate mapping with full path
python generate_mapping.py C:\Projects\SWMM\model.inp
```

**Output:**
The script will display:
```
Processing: model.inp

Validating .inp file...
✓ No validation issues found

Successfully parsed 19 sections
  [CONDUITS]: 1 entries
  [JUNCTIONS]: 1 entries
  ...

Content hash: b3abaeafb29f61b34bbdfc37b672c616

Discovered 2 input(s):
  [0] ElapsedTime (SYSTEM)
  [1] RG1 (GAGE)

Discovered 6 output(s):
  [0] POND1 (STORAGE - VOLUME)
  [1] OUT1 (OUTFALL - FLOW)
  ...

Generating mapping file: SwmmGoldSimBridge.json
Successfully generated: SwmmGoldSimBridge.json
Input count: 2
Output count: 6
```

**Validation Errors:**
If the script finds errors, it will stop and display them:
```
Validation Issues Found:
  ERROR: Weir 'W1' with RECT_OPEN has 3 parameters - SWMM expects 2 or 4
    Current line: W1 RECT_OPEN 10 0 0
    Fix: Either use 2 params (height width) or 4 params (height width slope_left slope_right)
  ERROR: No outfalls defined - SWMM requires at least one outlet node

❌ CRITICAL ERRORS FOUND - Model will likely fail to run in SWMM
Please fix the errors above before using this model with GoldSim
```

**Requirements:**
- Python 3.6 or higher
- No external dependencies (uses standard library only)
- SWMM .inp file must be valid and readable

**When to Regenerate:**
- After adding/removing rain gages with TIMESERIES DUMMY
- After adding/removing storage nodes, outfalls, orifices, weirs, or subcatchments
- After renaming any SWMM elements
- When switching to a different SWMM model
- After fixing validation errors

### Visualizing Your Model

Generate a visual diagram of your SWMM model structure:

```batch
python generate_diagram.py model.inp
```

This creates a Mermaid.js diagram file (`model.mmd`) showing:
- **Subcatchments** (green trapezoids) with runoff connections
- **Junctions** (gray stadium shapes)
- **Storage nodes** (orange cylinders)
- **Outfalls** (blue hexagons)
- **Conduits** (solid arrows)
- **Orifices** (dotted arrows labeled "orifice")
- **Weirs** (dotted arrows labeled "weir")
- **Pumps** (thick arrows labeled "pump")

**Viewing the Diagram:**
1. **In VS Code**: Install "Markdown Preview Mermaid Support" extension
2. **On GitHub/GitLab**: Commit the `.mmd` file - renders automatically
3. **Online**: Copy/paste to https://mermaid.live

**Example Output:**

The diagram below shows a storage pond model with two subcatchments (S1, S2) draining to a junction (J1), which flows via conduit to a storage node (POND1). The pond releases water through an orifice (OR1) and weir (W1) to an outfall (OUT1).

```mermaid
graph TD
    %% SWMM Model Structure Diagram

    %% Subcatchments
    S1[/S1\]
    S1 -->|runoff| J1
    S2[/S2\]
    S2 -->|runoff| J1

    %% Nodes
    J1([J1])
    POND1[(POND1)]
    OUT1[["OUT1"]]

    %% Conduits
    J1 -->|C1| POND1

    %% Orifices
    POND1 -.->|OR1<br/>orifice| OUT1

    %% Weirs
    POND1 -.->|W1<br/>weir| OUT1

    %% Styling
    classDef subcatchment fill:#e1f5e1,stroke:#4caf50,stroke-width:2px
    classDef storage fill:#fff3e0,stroke:#ff9800,stroke-width:2px
    classDef outfall fill:#e3f2fd,stroke:#2196f3,stroke-width:2px
    classDef junction fill:#f5f5f5,stroke:#757575,stroke-width:2px

    class S1,S2 subcatchment
    class POND1 storage
    class OUT1 outfall
    class J1 junction
```

This helps you:
- Understand model structure at a glance
- Document your model for reports
- Verify connections are correct
- Share model design with team members

### DLL Configuration

The interface is now **dynamic** based on your SWMM model:

```
Function Name:  SwmmGoldSimBridge  (case-sensitive!)
Inputs:         Determined by mapping file
                - inargs[0]: Elapsed time (seconds) - always present
                - inargs[1+]: Rain gages with TIMESERIES DUMMY
                  (Note: Currently only rain gages supported; future versions
                   could support pumps, valves, or other dynamic inputs)
Outputs:        Determined by mapping file
                - Storage node volumes
                - Outfall flows
                - Orifice flows
                - Weir flows
                - Subcatchment runoff
Version:        1.04
```

**Example:** A model with 2 DUMMY rain gages and 3 subcatchments would have:
- Inputs: 3 (elapsed time + 2 rain gages)
- Outputs: 3 (3 subcatchment runoff values)

#### Configuring GoldSim External Element

1. **Check Mapping File First:**
   - Open `SwmmGoldSimBridge.json` in a text editor
   - Note the `input_count` and `output_count` values
   - Review the `inputs` and `outputs` arrays to see element names

2. **Create External Element:**
   - In GoldSim, insert an External element
   - Set DLL file path to `GSswmm.dll` (or full path)
   - Set function name to `SwmmGoldSimBridge` (case-sensitive!)

3. **Configure Inputs:**
   - Set number of inputs to match `input_count` from mapping file
   - Input 0: Connect to GoldSim's ETime element (elapsed time in seconds)
   - Input 1+: Connect to rain gage data sources
     - **Units determined by SWMM model's FLOW_UNITS:**
       - US units (CFS, GPM, MGD): inches/hour
       - SI units (CMS, LPS): mm/hour
     - Check your model.inp `[OPTIONS]` section for `FLOW_UNITS`
   - Input order must match the `inputs` array in the mapping file
   - Currently only rain gages are supported as dynamic inputs

4. **Configure Outputs:**
   - Set number of outputs to match `output_count` from mapping file
   - Outputs will be populated in the order specified in mapping file
   - Check mapping file for element names and types

5. **Set Options:**
   - ☑ Run Cleanup after each realization (required)
   - ☐ Unload DLL after each use (optional)
   - ☑ Run in separate process space (required)

6. **Verify Configuration:**
   - Input count in GoldSim = `input_count` in mapping file
   - Output count in GoldSim = `output_count` in mapping file
   - All required files in same directory as .gsm file

### Setup Checklist
1. Copy DLLs and SWMM model to your GoldSim model directory
2. **Generate mapping file:** `python generate_mapping.py model.inp`
3. Create External element in GoldSim
4. Set DLL file to `GSswmm.dll`
5. Set function name to `SwmmGoldSimBridge`
6. Add inputs/outputs as specified in mapping file
7. **Critical**: Set GoldSim time step = SWMM ROUTING_STEP
8. Enable "Run Cleanup after each realization"
9. Link inputs to data sources
10. Run simulation (F5)

## Example Setup

### SWMM Model Requirements
Your `model.inp` must have:
- At least one subcatchment defined
- Valid ROUTING_STEP (e.g., `0:05:00` for 5 minutes)
- Consistent FLOW_UNITS (CFS, CMS, etc.)

### GoldSim External Element Settings
```
- Unload DLL after each use
- Run Cleanup after each realization  (required)
- Run in separate process space (required)
```

### Simple Test Model
1. **Inputs**: 
   - Elapsed Time: Use GoldSim's built-in ETime element
   - Rainfall: Constant 2.5 in/hr or time-varying storm
2. **Duration**: 2 hours minimum
3. **Time Step**: Must match SWMM routing step exactly
4. **Realizations**: Start with 1 for testing

Expected behavior:
- Runoff starts at 0
- Increases during rainfall
- Peaks after rainfall (lag time)
- Gradually recedes

## Common Issues

| Problem | Solution |
|---------|----------|
| "Cannot load DLL" | Copy DLL to model directory |
| "Cannot find function" | Check spelling: `SwmmGoldSimBridge` |
| "Argument mismatch" | Check mapping file for correct input/output counts |
| "Mapping file not found" | Run `python generate_mapping.py model.inp` |
| "Invalid JSON" | Regenerate mapping file from model.inp |
| Validation errors in mapping script | Fix .inp file errors reported by script, then regenerate |
| Runoff always zero | Match GoldSim time step to SWMM ROUTING_STEP |
| Simulation crashes | Enable "Run Cleanup after each realization" |
| Slow performance | Disable logging (see Performance Optimization section) |

## Performance Optimization

### Disabling Debug Logging

By default, logging is **disabled** for maximum performance. The DLL writes no log files during normal operation.

**To enable logging for troubleshooting:**

1. Open `SwmmGoldSimBridge.cpp`
2. Find the Logger class (around line 88)
3. Change:
   ```cpp
   static constexpr bool ENABLE_LOGGING = false;
   ```
   to:
   ```cpp
   static constexpr bool ENABLE_LOGGING = true;
   ```
4. Rebuild the DLL:
   ```batch
   scripts\build.bat
   copy /Y x64\Release\GSswmm.dll tests\GSswmm.dll
   ```

**Performance Impact:**
- **Logging disabled** (default): Maximum speed, no file I/O overhead
- **Logging enabled**: Useful for debugging but slower, especially for long simulations

**When to enable logging:**
- Troubleshooting initialization errors
- Debugging unexpected output values
- Investigating model behavior
- Reporting issues

**When to disable logging:**
- Production runs
- Long simulations (many timesteps)
- Multiple realizations
- Performance-critical applications

The log file (`bridge_debug.log`) is created in the same directory as the DLL and contains:
- Version information
- Initialization details
- First timestep values
- Error messages with SWMM report file contents

## Troubleshooting Mapping Errors

### Error: "Mapping file not found: SwmmGoldSimBridge.json"

**Cause:** The DLL cannot find the mapping file in the expected location.

**Solutions:**
1. Ensure `SwmmGoldSimBridge.json` is in the same directory as your GoldSim model (.gsm file)
2. Regenerate the mapping file: `python generate_mapping.py model.inp`
3. Check that the file name is exactly `SwmmGoldSimBridge.json` (case-sensitive on some systems)
4. Verify file permissions allow reading

### Error: "Invalid mapping file format"

**Cause:** The JSON file is corrupted or malformed.

**Solutions:**
1. Regenerate the mapping file: `python generate_mapping.py model.inp`
2. Open the JSON file in a text editor and check for syntax errors
3. Validate JSON format using an online JSON validator
4. Ensure the file wasn't manually edited incorrectly

### Error: "Mapping file missing required field: [field_name]"

**Cause:** The mapping file is incomplete or from an older version.

**Solutions:**
1. Regenerate the mapping file with the current version of `generate_mapping.py`
2. Ensure you're using the correct version of the parser script
3. Check that the parser script completed successfully without errors

### Error: "SWMM element not found: [element_name]"

**Cause:** The mapping file references an element that doesn't exist in the SWMM model.

**Solutions:**
1. Verify that `model.inp` and `SwmmGoldSimBridge.json` are synchronized
2. Regenerate the mapping file from the current `model.inp`
3. Check that the SWMM model file hasn't been modified after generating the mapping
4. Ensure element names in the .inp file match exactly (case-sensitive)

### Warning: "Hash mismatch detected"

**Cause:** The SWMM model has been modified since the mapping file was generated.

**Impact:** The simulation may continue but could produce incorrect results if the interface changed.

**Solutions:**
1. Regenerate the mapping file: `python generate_mapping.py model.inp`
2. Update GoldSim External element if input/output counts changed
3. Always regenerate after modifying the SWMM model structure

### Error: "Invalid SWMM index for element: [element_name]"

**Cause:** The DLL cannot resolve the element name to a valid SWMM API index.

**Solutions:**
1. Verify the element exists in the SWMM model
2. Check that the SWMM model loads successfully in SWMM GUI
3. Regenerate the mapping file
4. Ensure the SWMM model file is not corrupted

### Parser Script Errors

**Error: "File not found: [filename]"**
- Check that the .inp file path is correct
- Use quotes around paths with spaces: `python generate_mapping.py "my model.inp"`
- Verify the file exists and is readable

**Error: "Invalid syntax in .inp file"**
- Open the .inp file in SWMM GUI to check for errors
- Fix any syntax errors in the SWMM model
- Ensure section headers are properly formatted: `[SECTION_NAME]`

**No inputs/outputs discovered:**
- For inputs: Add rain gages with `TIMESERIES DUMMY` in the [RAINGAGES] section
- For outputs: Add at least one storage node, outfall, orifice, weir, or subcatchment
- Verify sections are properly formatted in the .inp file

### Debugging Tips

1. **Check the mapping file contents:**
   ```batch
   type SwmmGoldSimBridge.json
   ```
   Verify input_count, output_count, and element names

2. **Enable debug logging:**
   - Check `bridge_debug.log` for detailed error messages
   - Look for warnings about hash mismatches or element resolution failures

3. **Test SWMM model independently:**
   - Open `model.inp` in SWMM GUI
   - Run the simulation to verify the model is valid
   - Check that all elements referenced in the mapping exist

4. **Verify file locations:**
   ```
   YourGoldSimModel/
   ├── YourModel.gsm                # GoldSim model
   ├── model.inp                    # SWMM input file
   ├── SwmmGoldSimBridge.json       # Mapping file
   ├── GSswmm.dll                   # Bridge DLL
   └── swmm5.dll                    # SWMM engine
   ```

5. **Test with minimal model:**
   - Start with a simple SWMM model (1 subcatchment, 1 outfall)
   - Generate mapping and test in GoldSim
   - Gradually add complexity once basic setup works

## Dynamic Interface Mapping

### How It Works

The bridge uses a JSON mapping file to dynamically configure the interface:

1. **Parser Script** (`generate_mapping.py`):
   - Reads your SWMM .inp file
   - Discovers inputs (rain gages with TIMESERIES DUMMY)
   - Discovers outputs (storage, outfalls, orifices, weirs, subcatchments)
   - Generates `SwmmGoldSimBridge.json`

2. **Bridge DLL** (`GSswmm.dll`):
   - Reads mapping file at runtime
   - Reports correct input/output counts to GoldSim
   - Maps GoldSim arrays to SWMM elements dynamically

### Mapping File Format

Example `SwmmGoldSimBridge.json`:
```json
{
  "version": "1.0",
  "inp_file_hash": "dec4572680eea82ea1b700b11aa6702b",
  "input_count": 1,
  "output_count": 4,
  "inputs": [
    {
      "index": 0,
      "name": "ElapsedTime",
      "object_type": "SYSTEM",
      "property": "ELAPSEDTIME"
    }
  ],
  "outputs": [
    {
      "index": 0,
      "name": "OUT1",
      "object_type": "OUTFALL",
      "property": "FLOW",
      "swmm_index": 0
    },
    {
      "index": 1,
      "name": "S1",
      "object_type": "SUBCATCH",
      "property": "RUNOFF",
      "swmm_index": 0
    }
  ]
}
```

### Benefits

- **No DLL Recompilation**: Change your SWMM model without rebuilding the bridge
- **Flexible Interface**: Support any number of inputs and outputs
- **Multiple Element Types**: Storage, outfalls, orifices, weirs, subcatchments
- **Validation**: Hash checking detects model/mapping mismatches

## Validation

After running:
1. Check `model.rpt` - SWMM report file (should show "Analysis begun")
2. Compare GoldSim runoff to SWMM report values
3. Verify runoff responds logically to rainfall
4. Check GoldSim log for "External function version: 1.04"

## Testing

Run tests from project root:
```batch
scripts\test.bat              # Run all tests
scripts\build-and-test.bat    # Build + test
```

6 test suites with 32 total tests should pass.

See [scripts/README.md](scripts/README.md) for all available scripts.

## Building

### Quick Build

```batch
scripts\build.bat             # Build DLL only
scripts\release.bat           # Build + test everything
```

### Build Requirements

- Visual Studio 2022 (any edition)
- Windows 10/11
- MSBuild (included with Visual Studio)

### Build Process

1. **Build the DLL:**
   ```batch
   scripts\build.bat
   ```
   Output: `x64/Release/GSswmm.dll`

2. **Copy DLL to tests directory (CRITICAL):**
   ```batch
   xcopy /Y x64\Release\GSswmm.dll tests\
   ```
   This is required before running tests.

3. **Run tests:**
   ```batch
   scripts\test.bat
   ```

### Rebuild Test Executables

If you modify test source files, rebuild the test executables:

```powershell
# Easy method (works from any command prompt)
powershell -ExecutionPolicy Bypass -File scripts\rebuild-tests.ps1
```

Or from Developer Command Prompt for VS 2022:
```batch
scripts\rebuild-tests.bat
```

### Complete Build Workflow

```batch
# Option 1: Manual steps
scripts\build.bat
xcopy /Y x64\Release\GSswmm.dll tests\
scripts\test.bat

# Option 2: Automated (recommended)
scripts\release.bat
```

The `release.bat` script handles everything automatically:
- Cleans artifacts
- Builds DLL
- Copies DLL to tests
- Runs all tests

## Testing

### Run All Tests

```batch
scripts\test.bat
```

Expected: 8 test suites, all passing

### Run Individual Tests

Tests must be run from the `tests/` directory:

```batch
pushd tests & test_lifecycle.exe & popd
pushd tests & test_subcatchment_out_of_range.exe & popd
```

### Test Suites

1. **test_lifecycle.exe** - DLL lifecycle (initialize, calculate, cleanup)
2. **test_calculate.exe** - Calculation logic
3. **test_error_handling.exe** - Error handling
4. **test_file_validation.exe** - File validation
5. **test_subcatchment_validation.exe** - Subcatchment validation
6. **test_subcatchment_out_of_range.exe** - Boundary testing
7. **test_json_parsing.exe** - Mapping file parsing
8. **test_integration_e2e.exe** - End-to-end integration

### Python Tests

```batch
cd tests
python test_parser.py
python test_mapping_generation.py
```

## Building

### Visual Studio Build

Open `GSswmm.sln` in Visual Studio:
- Configuration: Release
- Platform: x64
- Build → Rebuild Solution
- Output: `x64/Release/GSswmm.dll`

**After building in Visual Studio:**
```batch
xcopy /Y x64\Release\GSswmm.dll tests\
```

Dependencies:
- `include/swmm5.h`
- `lib/swmm5.lib`

## Project Structure

### Development Structure
```
├── SwmmGoldSimBridge.cpp          # Main bridge implementation
├── MappingLoader.cpp              # JSON mapping file loader
├── include/
│   ├── swmm5.h                    # SWMM API header
│   └── MappingLoader.h            # Mapping loader header
├── generate_mapping.py            # Generate mapping files with validation
├── generate_diagram.py            # Generate Mermaid.js diagrams
├── GSswmm.sln/vcxproj             # Visual Studio project
├── lib/swmm5.lib                  # SWMM import library (for linking)
├── swmm5.dll                      # SWMM runtime (source copy)
├── x64/Release/                   # Build output
│   ├── GSswmm.dll                 # Built bridge DLL
│   └── swmm5.dll                  # Copied dependency
├── tests/                         # Test suite
│   ├── test_*.cpp                 # C++ test files
│   ├── test_*.py                  # Python test files
│   ├── *.inp                      # Test SWMM models
│   ├── *.mmd                      # Generated diagrams
│   ├── GSswmm.dll                 # Test DLL copy
│   ├── swmm5.dll                  # Test dependency copy
│   ├── model.out/rpt              # Test outputs (created here)
│   └── bridge_debug.log           # Test logs (created here, if enabled)
└── scripts/                       # Build scripts
    ├── build.bat                  # Build DLL (copies swmm5.dll to x64/Release)
    ├── release.bat                # Full release pipeline
    └── test.bat                   # Run all tests
```

### Deployment Structure (End Users)
```
YourProject/
├── YourModel.gsm                  # GoldSim model
├── model.inp                      # SWMM input file
├── SwmmGoldSimBridge.json         # Mapping config (generated from model.inp)
├── GSswmm.dll                     # Bridge DLL (from x64/Release/)
├── swmm5.dll                      # SWMM engine (MUST be with GSswmm.dll)
├── model.out                      # Output (created automatically during simulation)
├── model.rpt                      # Report (created automatically during simulation)
└── bridge_debug.log               # Log (created if logging enabled)
```

**Key Points:**
- All files go in the same directory as your GoldSim model (.gsm file)
- `swmm5.dll` must always be in the same directory as `GSswmm.dll`
- Output files (model.out, model.rpt, bridge_debug.log) are created automatically in this directory
- Build scripts automatically copy `swmm5.dll` to the correct locations
- Logging is disabled by default for performance (no log file created)

## Utility Scripts

### generate_mapping.py

Generates the JSON mapping file that defines the interface between GoldSim and SWMM.

**Features:**
- Parses SWMM .inp files
- Discovers inputs (rain gages with TIMESERIES DUMMY)
- Discovers outputs (storage, outfalls, orifices, weirs, subcatchments)
- **Validates .inp file for common errors**
- Computes content hash for change detection
- Generates `SwmmGoldSimBridge.json`

**Validation Checks:**
- Required sections (OPTIONS, RAINGAGES, SUBCATCHMENTS, etc.)
- XSECTIONS parameter counts (catches weir formatting errors)
- Node references (verifies links connect to valid nodes)
- Outfall existence (SWMM requires at least one outlet)

**Usage:**
```batch
python generate_mapping.py model.inp
```

**Output:**
- `SwmmGoldSimBridge.json` - Interface mapping file
- Console output with validation results and discovered elements

### generate_diagram.py

Generates a Mermaid.js diagram visualizing your SWMM model structure.

**Features:**
- Parses SWMM .inp files
- Creates visual diagram with:
  - Subcatchments (green trapezoids)
  - Junctions (gray stadium shapes)
  - Storage nodes (orange cylinders)
  - Outfalls (blue hexagons)
  - Conduits (solid arrows)
  - Orifices (dotted arrows)
  - Weirs (dotted arrows)
  - Pumps (thick arrows)
- Color-coded by element type
- Shows flow connections and element names

**Usage:**
```batch
python generate_diagram.py model.inp
```

**Output:**
- `model.mmd` - Mermaid diagram file
- Can be viewed in VS Code, GitHub, GitLab, or https://mermaid.live

**Example:**
```batch
python generate_diagram.py tests\storage_model.inp
# Creates: tests\storage_model.mmd
```

**Viewing Options:**
1. **VS Code**: Install "Markdown Preview Mermaid Support" extension
2. **GitHub/GitLab**: Commit `.mmd` file - renders automatically in markdown
3. **Online**: Copy/paste content to https://mermaid.live
4. **Documentation**: Include in reports or technical documentation

## Technical Details

### Function Signature
```cpp
extern "C" void __declspec(dllexport) SwmmGoldSimBridge(
    int methodID,
    int* status,
    double* inargs,
    double* outargs
)
```

### Method IDs
- `0` - XF_INITIALIZE: Initialize SWMM
- `1` - XF_CALCULATE: Pass inputs; get outputs (dynamic based on mapping)
- `2` - XF_REP_VERSION: Report version (1.04)
- `3` - XF_REP_ARGUMENTS: Report args (dynamic from mapping file)
- `99` - XF_CLEANUP: Clean up SWMM

### Status Codes
- `0` - Success
- `1` - Fatal error
- `-1` - Error with message

## Units

### Inputs
- **Elapsed Time**: seconds (always)
- **Rain Gages**: Units determined by SWMM model's FLOW_UNITS setting
  - **US Customary units** (CFS, GPM, MGD): **inches/hour**
  - **SI units** (CMS, LPS, MLD): **mm/hour**
  - Check your `model.inp` file's `[OPTIONS]` section for `FLOW_UNITS`

**Example from model.inp:**
```
[OPTIONS]
FLOW_UNITS           CFS
```
This means rainfall inputs should be in **inches/hour**.

**Note:** Currently only rain gages are supported as dynamic inputs. Evaporation is controlled by the [EVAPORATION] section in your SWMM input file and cannot be set dynamically via the API.

### Output - Runoff (from SWMM FLOW_UNITS)
- CFS (cubic feet/sec)
- CMS (cubic meters/sec)
- GPM (gallons/min)
- MGD (million gallons/day)
- LPS (liters/sec)

## Project Maintenance

### Keeping the Project Tidy

The project is configured to stay clean automatically:

**Build artifacts** go to `x64/Release/` (not root)
**Test outputs** go to `tests/` (with the test DLL)
**Documentation** stays in README.md (no separate doc files)

The `.gitignore` prevents:
- Build artifacts in root
- Runtime files (model.out, model.rpt, bridge_debug.log)
- Temporary documentation files (*_SUMMARY.md, *_UPDATE.md, etc.)
- Python cache and IDE files

### Build Scripts Handle Cleanup

- `scripts/build.bat` - Builds DLL and copies swmm5.dll to x64/Release/
- `scripts/release.bat` - Cleans test artifacts and runs full validation
- `scripts/clean.bat` - Removes all build artifacts

## Additional Documentation

- `docs/WORKFLOW_EXAMPLE.md` - Complete step-by-step example workflow
- `docs/goldsim_external.txt` - GoldSim External DLL API reference
- `docs/External (DLL) Elements.pdf` - Official GoldSim documentation
- `.kiro/specs/goldsim-swmm-bridge/` - Original bridge specification
- `.kiro/specs/scripted-interface-mapping/` - Dynamic mapping feature specification
  - `requirements.md` - Feature requirements
  - `design.md` - Technical design
  - `tasks.md` - Implementation tasks

## Implementation Status

### Completed Features
- ✅ Core bridge functionality (XF_INITIALIZE, XF_CALCULATE, XF_CLEANUP)
- ✅ File validation and error handling
- ✅ Subcatchment index validation
- ✅ Python parser script for mapping generation
- ✅ JSON mapping file loader (MappingLoader class)
- ✅ Dynamic XF_REP_ARGUMENTS (reports counts from mapping file)

### In Progress
- 🔄 Dynamic XF_INITIALIZE (load element handles from mapping)
- 🔄 Dynamic XF_CALCULATE (use mapping for inputs/outputs)

### Planned
- ⏳ Hash validation and mismatch warnings
- ⏳ Property-based testing for parser and DLL
- ⏳ End-to-end integration tests

## License

This bridge is provided as-is for use with GoldSim and EPA SWMM.
