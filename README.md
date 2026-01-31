# GS-SWMM: GoldSim-SWMM Bridge

A DLL that connects [GoldSim](https://www.goldsim.com/) with [EPA SWMM5](https://www.epa.gov/water-research/storm-water-management-model-swmm). The interface is defined by a JSON config file you generate from your SWMM model.

## Quick Start

### 1. Get Example Files

Three complete examples are included in the release package ([Download Latest GS-SWMM Release](https://github.com/jlillywh/GSswmm/releases/latest)):

**Example 1 - Simple Model**
- Simple kinematic wave model with precipitation gage
- Includes a pond that drains via orifice
- GoldSim controls rainfall and receives catchment runoff, pond volume, and outfall flow
- Best for learning the basics
<img width="826" height="602" alt="image" src="https://github.com/user-attachments/assets/19133f6d-11ae-4900-8eb8-6d909bb11406" />


**Example 2 - Site Drainage**
- EPA SWMM example model with 7 subcatchments
- Uses dynamic wave routing with pipes leading to outfall
- GoldSim controls precipitation for all subcatchments
- Demonstrates multi-subcatchment coupling
<img width="735" height="634" alt="image" src="https://github.com/user-attachments/assets/5633a031-0575-402e-bd60-abec64c717c8" />


**Example 3 - Pump Control**
- EPA SWMM pump control example
- Overrides SWMM's built-in pump rules with GoldSim controller
- Uses deadband control based on pond water level
- Demonstrates real-time structure control from GoldSim
<img width="856" height="604" alt="image" src="https://github.com/user-attachments/assets/f046993a-eba5-4e53-90df-ebe8bad0feca" />

Extract [Download Source Code (.zip)](https://github.com/jlillywh/GS-SWMM/archive/refs/heads/main.zip) to get started with any of these models.

### 2. Generate Configuration File

The bridge requires a JSON configuration file (`SwmmGoldSimBridge.json`) that maps SWMM elements to GoldSim inputs/outputs. Generate this file using the Python script:

**Basic usage (auto-generates all outputs):**
```bash
python generate_mapping.py model.inp
```

This creates `SwmmGoldSimBridge.json` with:
- **ElapsedTime** as input (always included automatically)
- **All available outputs** from your model:
  - Storage nodes → VOLUME
  - Outfalls → FLOW
  - Pumps, Orifices, Weirs → FLOW
  - Subcatchments → RUNOFF

**Add controllable inputs:**
```bash
# Control rainfall from rain gage R1
python generate_mapping.py model.inp --input R1

# Control multiple elements
python generate_mapping.py model.inp --input R1 --input PUMP1 --input J2
```

**Specify exact outputs you need:**
```bash
# Only monitor specific elements
python generate_mapping.py model.inp --input R1 --output SUB1 --output POND1 --output OUT1
```

**Available arguments:**
- `--input` or `-i` : Add controllable input by element name (repeatable)
  - Rain gages: Controls rainfall intensity
  - Pumps/Orifices/Weirs: Controls setting (0.0 to 1.0)
  - Junctions/Storage: Controls lateral inflow
- `--output` or `-o` : Add output by element name (repeatable)
  - If omitted, all elements are added as outputs
- `--output-file` or `-f` : Specify output filename (default: SwmmGoldSimBridge.json)

**Customizing the JSON:**

After generation, you can manually edit `SwmmGoldSimBridge.json` to:
1. **Remove unwanted outputs** - Delete entries you don't need to monitor
2. **Change properties** - For example, change storage from VOLUME to DEPTH
3. **Adjust logging** - Set `logging_level` to "DEBUG", "INFO", "ERROR", or "OFF"

**Example: Simple Model Configuration**

Here's the actual JSON from the Simple Model example (generated with `python generate_mapping.py model.inp --input R1 --output S1 --output POND --output OUT1`):

```json
{
  "version": "1.0",
  "logging_level": "ERROR",
  "inp_file_hash": "5245d86855c599addf209ec8ff2956ca",
  "input_count": 2,
  "output_count": 3,
  "inputs": [
    {
      "index": 0,
      "name": "ElapsedTime",
      "object_type": "SYSTEM",
      "property": "ELAPSEDTIME"
    },
    {
      "index": 1,
      "name": "R1",
      "object_type": "GAGE",
      "property": "RAINFALL"
    }
  ],
  "outputs": [
    {
      "index": 0,
      "name": "S1",
      "object_type": "SUBCATCH",
      "property": "RUNOFF",
      "swmm_index": 0
    },
    {
      "index": 1,
      "name": "POND",
      "object_type": "STORAGE",
      "property": "VOLUME",
      "swmm_index": 0
    },
    {
      "index": 2,
      "name": "OUT1",
      "object_type": "OUTFALL",
      "property": "FLOW",
      "swmm_index": 0
    }
  ]
}
```

This configuration:
- **Input[0]**: ElapsedTime (automatic)
- **Input[1]**: Rainfall intensity for rain gage R1 (in/hr)
- **Output[0]**: Subcatchment S1 runoff (CFS)
- **Output[1]**: POND storage volume (cubic feet)
- **Output[2]**: OUT1 outfall flow (CFS)

### 3. Copy Files to Your Working Directory

```
Your_Model_Directory/
├── GSswmm.dll                  (from release)
├── swmm5.dll                   (from release)
├── model.inp                   (your SWMM model)
├── SwmmGoldSimBridge.json      (generated in step 2)
└── generate_mapping.py         (optional, for regenerating config)
```

### 4. Configure GoldSim External Element

- **DLL File**: `GSswmm.dll`
- **Function Name**: `SwmmGoldSimBridge` (case-sensitive!)
- **Unload DLL after each use**: ☐ Unchecked
- **Run Cleanup after each realization**: ☑ Checked
- **Run in separate process space**: ☑ Checked

Click "Get Argument Info" to verify the input/output counts match your JSON configuration.

### 5. Match Time Steps

**CRITICAL**: Set GoldSim's Basic Time Step to match SWMM's `ROUTING_STEP` in `model.inp`.

Example models use different timesteps - check each model's `[OPTIONS]` section.

**IMPORTANT**: When using Dynamic Wave (DYNWAVE) routing, you must set `VARIABLE_STEP 0` in your SWMM model options to disable variable timesteps. Variable timesteps cause inconsistent results between standalone SWMM and API coupling. See "Variable Timestep Limitation" section below for details.

### 6. Map Inputs/Outputs

Check the `SwmmGoldSimBridge.json` file in your chosen example to see the input/output mapping. Each example has different elements being monitored and controlled.

The input/output indices in GoldSim must match the `index` values in your JSON file. The "Get Argument Info" button in GoldSim will show you the total counts.

### 7. Run

Press F5 or Simulation → Run.

## How It Works

1. **Config**: Bridge loads `SwmmGoldSimBridge.json` defining input/output mappings
2. **Init**: Opens SWMM model, resolves element names to indices
3. **Step**: Each time step, applies GoldSim inputs → calls `swmm_step()` → returns outputs
4. **Cleanup**: Closes SWMM at end of realization

## Input/Output Mapping

The JSON file defines which SWMM elements map to GoldSim inputs/outputs.

### Supported Inputs (from GoldSim → SWMM)
- **ElapsedTime** (SYSTEM) - Automatically managed
- **Rainfall** (GAGE) - Override timeseries rainfall
- **Pump/Orifice/Weir settings** (LINK) - Control structures (0.0 to 1.0)
- **Node lateral flows** (NODE) - External inflow/outflow

### Supported Outputs (from SWMM → GoldSim)
- **Subcatchment runoff** (SUBCATCH) - Runoff rate (CFS)
- **Storage volume/depth/inflow** (STORAGE) - Volume (cu ft), depth (ft), and inflow rate (CFS)
- **Link flows** (PUMP/ORIFICE/WEIR/CONDUIT) - Flow rate (CFS)
- **Node inflow/depth** (JUNCTION) - Total inflow (CFS), depth (ft)
- **Outfall flow** (OUTFALL) - Discharge rate (CFS)

**Complete reference**: See input/output property codes in `include/swmm5.h`

## Troubleshooting

| Problem | Solution |
|---------|----------|
| "Cannot load DLL" | Copy `GSswmm.dll` and `swmm5.dll` to model directory |
| "Cannot find function" | Function name is `SwmmGoldSimBridge` (case-sensitive) |
| "Mapping file not found" | Copy `SwmmGoldSimBridge.json` from `examples/` to model directory |
| "File not found" error | Copy `model.inp` from `examples/` to model directory |
| Staircase patterns in results | GoldSim timestep must match SWMM ROUTING_STEP. For DYNWAVE routing, set `VARIABLE_STEP 0` in SWMM options |
| Orifice flow oscillations | Switch from DYNWAVE to KINWAVE routing for better stability |
| Runoff always zero | Verify rainfall input is being passed correctly, check `bridge_debug.log` |
| Simulation crashes | Enable "Run Cleanup after each realization" in GoldSim |

## Building from Source

**Requirements**: Visual Studio 2022, Windows SDK

```batch
# Open GSswmm.sln
# Select "Release | x64"
# Build → Build Solution
# Output: x64/Release/GSswmm.dll
```

**Run Tests**:
```batch
cd tests
run_all_tests.bat
```

## API Reference

### Method IDs

| ID | Method | Description |
|----|--------|-------------|
| 0 | XF_INITIALIZE | Initialize SWMM model |
| 1 | XF_CALCULATE | Run one time step |
| 2 | XF_REP_VERSION | Return DLL version (5.202) |
| 3 | XF_REP_ARGUMENTS | Return input/output counts from JSON |
| 99 | XF_CLEANUP | Cleanup and release resources |

### Status Codes

| Code | Meaning |
|------|---------|
| 0 | Success |
| 1 | Failure |
| -1 | Failure with error message |

## Logging

Control log level in `SwmmGoldSimBridge.json`:

```json
{
  "version": "1.0",
  "logging_level": "INFO",
  ...
}
```

**Log Levels:**
- `"OFF"` or `"NONE"` - No logging
- `"ERROR"` - Errors only
- `"INFO"` - Errors + important events (recommended)
- `"DEBUG"` - Everything (verbose)

Logs write to `bridge_debug.log` in your model directory. Change `logging_level` in the JSON and restart your simulation - no rebuild needed!

## Architecture

- **SwmmGoldSimBridge.cpp**: Main bridge, loads JSON, drives simulation
- **MappingLoader.cpp/h**: Parses JSON config
- **generate_mapping.py**: Generates JSON from SWMM `.inp` file
- **swmm5.h**: SWMM API header

## Known Limitations

### Variable Timestep Limitation (DYNWAVE Only)

**Issue**: When using Dynamic Wave routing with variable timesteps (`VARIABLE_STEP > 0`), results from API coupling may not match standalone EPA SWMM. Subcatchment runoff can appear to "stair-step" and peak flows may be underestimated.

**Root Cause**: EPA SWMM with variable timesteps takes many small adaptive internal steps (0.5s, 1s, 2s) based on hydraulic conditions, even when `ROUTING_STEP` is set to minutes. The API only returns the final state after all sub-steps complete, missing intermediate runoff calculations.

**Solution**: Disable variable timesteps for API coupling:

```
VARIABLE_STEP        0
```

This forces SWMM to use fixed timesteps equal to `ROUTING_STEP`, ensuring:
- Consistent behavior at each API call
- Runoff values update predictably
- Results match between API and standalone SWMM
- Peak flows are accurately captured

**Recommended Settings for API Coupling**:
```
ROUTING_STEP         0:00:15    # Fixed timestep (15 seconds recommended)
WET_STEP             00:00:15   # Match routing step for smooth runoff
REPORT_STEP          00:00:15   # Match for consistent reporting
VARIABLE_STEP        0          # REQUIRED: Disable variable stepping
```

**Note**: KINWAVE routing is not affected by this issue and works fine with any timestep settings.

### Water Quality Not Supported

The SWMM5 API doesn't expose pollutant concentrations during live simulation. Only hydraulic properties (flow, depth, volume) are accessible in real-time. Water quality results are only available in the `.out` file after simulation completion.

## Version

**DLL Version**: 5.202  
**Last Updated**: January 2026

## Changelog

### v5.202 (January 2026)
- Cleaned up repository structure (moved batch files to scripts/)
- Removed build artifacts from root directory
- Organized project for cleaner releases

### v5.201 (January 2026)
- Added DEPTH property support for storage nodes and junctions
- Updated examples documentation
- Three example models included in release

### v5.2 (January 2026)
- Documented variable timestep limitation with DYNWAVE routing
- Added VARIABLE_STEP 0 requirement to README
- Consolidated all documentation into single README.md

### v5.1 (January 2026)
- Fixed timing synchronization between inputs and outputs
- Inputs now properly applied before stepping simulation
- Added pending input buffer to ensure correct temporal alignment

### v5.0 (January 2026)
- Config-driven interface via `SwmmGoldSimBridge.json`
- Generate mapping with `python generate_mapping.py model.inp`
- Removed hardcoded interface definitions
- Dynamic input/output counts based on config

### v4.1 (January 2026)
- Documented water quality limitations
- No code changes from v4.0

### v4.0 (January 2026)
- Treatment train support (7 outputs)
- Enhanced validation and logging

### v3.0 (January 2026)
- Multiple storage nodes
- Link flow tracking

### v2.0 (January 2026)
- Storage volume output
- Comprehensive test suite

### v1.0 (January 2026)
- Initial release
- Basic rainfall-runoff coupling

## License

Integrates EPA SWMM (public domain) with GoldSim. See license files for details.


