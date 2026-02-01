# GS-SWMM: GoldSim-SWMM Bridge

**Version 5.212** - Real-time coupling between GoldSim and EPA SWMM5

## Overview

GS-SWMM is a bridge DLL that enables real-time coupling between [GoldSim](https://www.goldsim.com/) dynamic simulation software and [EPA SWMM5](https://www.epa.gov/water-research/storm-water-management-model-swmm) stormwater model. This integration allows you to:

- **Control SWMM from GoldSim** - Override rainfall, pump settings, and lateral flows during simulation
- **Access SWMM results in real-time** - Read flows, depths, volumes, and storage at each timestep
- **Model contaminant transport** - Track pollutant fate through stormwater systems and LID treatment trains
- **Couple models** - Integrate SWMM hydraulics with GoldSim's contaminant transport, economics, or decision models

### Key Features

- **JSON-based configuration** - Simple mapping between SWMM elements and GoldSim inputs/outputs
- **Automatic mapping generation** - Python script creates configuration from your SWMM model
- **LID treatment train support** - Access individual LID unit storage, inflow, overflow, and drain flows
- **Complete examples included** - Three working models to get you started quickly
- **No SWMM GUI needed** - Runs entirely through the API during GoldSim simulation

### Use Cases

- **Contaminant fate modeling** - Track pollutants through stormwater systems with detailed hydraulics
- **Real-time control** - Implement adaptive control strategies for pumps and gates
- **Treatment train analysis** - Model water quality through series of LID practices
- **Integrated modeling** - Combine stormwater hydraulics with watershed processes, economics, or risk analysis

## What's New in v5.212

**LID API Extensions** - Access individual LID unit data for contaminant transport modeling:
- Track storage volumes in rain barrels, infiltration trenches, planters, etc.
- Monitor inflow, overflow, and drain flows from each LID unit
- Model treatment trains with detailed mass balance
- Complete water balance tracking for all LID types

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

**Specify controllable inputs:**
```bash
# Control ONLY rainfall from rain gage R1 (plus ElapsedTime)
python generate_mapping.py model.inp --input R1

# Control ONLY these specific elements (plus ElapsedTime)
python generate_mapping.py model.inp --input R1 --input PUMP1 --input J2
```

**Important:** The `--input` flag specifies exactly which elements you want to control. If you don't use `--input`, you'll only have ElapsedTime as an input (no controllable elements).

**Specify exact outputs you need:**
```bash
# Only monitor specific elements
python generate_mapping.py model.inp --input R1 --output SUB1 --output POND1 --output OUT1
```

**Available arguments:**
- `--input` or `-i` : Specify controllable input by element name (repeatable)
  - Only the elements you specify will be controllable inputs
  - ElapsedTime is always included automatically as input[0]
  - Rain gages: Controls rainfall intensity
  - Pumps/Orifices/Weirs: Controls setting (0.0 to 1.0)
  - Junctions/Storage: Controls lateral inflow
- `--output` or `-o` : Specify output by element name (repeatable)
  - Only the elements you specify will be outputs
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
- **LID unit data** (LID) - Storage volume (cu ft), inflow rate (CFS), overflow rate (CFS), drain flow rate (CFS)
  - Enables detailed treatment train modeling
  - See [LID Support](#lid-low-impact-development-support) section below

**Complete reference**: See input/output property codes in `include/swmm5.h`

## LID (Low Impact Development) Support

The bridge supports accessing storage volumes and flow rates from individual LID units deployed in subcatchments. This enables detailed contaminant transport modeling through LID treatment trains.

### Composite ID Format

LID outputs use a composite ID format to reference specific LID units:

```
SubcatchmentName/LIDControlName
```

**Example:** `S1/InfilTrench` refers to the "InfilTrench" LID control deployed in subcatchment "S1".

### Generating LID Outputs

Use the `--lid-outputs` flag with the mapping generator:

```bash
python generate_mapping.py model.inp --lid-outputs
```

This automatically discovers all LID deployments from the `[LID_USAGE]` section and creates output entries for each one.

**Example Generated JSON:**
```json
{
  "outputs": [
    {
      "index": 0,
      "name": "S1/InfilTrench",
      "object_type": "LID",
      "property": "STORAGE_VOLUME"
    },
    {
      "index": 1,
      "name": "S1/InfilTrench",
      "object_type": "LID",
      "property": "SURFACE_INFLOW"
    },
    {
      "index": 2,
      "name": "S1/InfilTrench",
      "object_type": "LID",
      "property": "SURFACE_OUTFLOW"
    }
  ]
}
```

### LID Output Properties

Supported LID properties:
- **STORAGE_VOLUME** - Total water storage volume across all LID layers (surface + soil + storage + pavement) in cubic feet or cubic meters
- **SURFACE_INFLOW** - Inflow rate from subcatchment runoff entering the LID in CFS or CMS
- **SURFACE_OUTFLOW** - Overflow rate when LID exceeds capacity in CFS or CMS
- **DRAIN_FLOW** - Flow rate through underdrain in CFS or CMS

Units match the model's flow units configuration.

### How to Access LID Outputs

**Step 1: Generate mapping with LID outputs**
```bash
python generate_mapping.py model.inp --lid-outputs
```

**Step 2: Customize properties in JSON**

Edit the generated `SwmmGoldSimBridge.json` to specify which properties you need:

```json
{
  "outputs": [
    {
      "index": 0,
      "name": "S4/Planters",
      "object_type": "LID",
      "property": "SURFACE_INFLOW"
    },
    {
      "index": 1,
      "name": "S4/Planters",
      "object_type": "LID",
      "property": "STORAGE_VOLUME"
    },
    {
      "index": 2,
      "name": "S4/Planters",
      "object_type": "LID",
      "property": "DRAIN_FLOW"
    },
    {
      "index": 3,
      "name": "S4/Planters",
      "object_type": "LID",
      "property": "SURFACE_OUTFLOW"
    }
  ]
}
```

**Step 3: Use in GoldSim**

The outputs appear in order in your GoldSim External element:
- Output[0] = Planter inflow rate
- Output[1] = Planter storage volume
- Output[2] = Planter drain flow
- Output[3] = Planter overflow

### Example: Complete LID Water Balance

For a planter with underdrain, track the complete water balance:

```json
{
  "outputs": [
    {
      "name": "S4/Planters",
      "object_type": "LID",
      "property": "SURFACE_INFLOW"
    },
    {
      "name": "S4/Planters",
      "object_type": "LID",
      "property": "STORAGE_VOLUME"
    },
    {
      "name": "S4/Planters",
      "object_type": "LID",
      "property": "DRAIN_FLOW"
    },
    {
      "name": "S4/Planters",
      "object_type": "LID",
      "property": "SURFACE_OUTFLOW"
    }
  ]
}
```

This tracks:
- **Inflow** - Runoff entering the LID from the subcatchment
- **Storage** - Water stored in the planter
- **Drain Flow** - Treated water leaving through the underdrain
- **Overflow** - Water overflowing when capacity is exceeded

Use for mass balance: `Change in Storage = Inflow - Drain Flow - Overflow - Infiltration - Evaporation`

For contaminant transport: `Load = Flow Rate × Concentration`

### Example: LID Treatment Train

For a subcatchment with multiple LID units in series:

```
[LID_USAGE]
;;Subcatchment  LID_Process      Number  Area      Width
S1              Bioswale         1       5000      10
S1              DetentionPond    1       10000     50
S1              RetentionPond    1       8000      40
```

Generate mapping:
```bash
python generate_mapping.py model.inp --input R1 --lid-outputs --output OUT1
```

This creates outputs for:
- `S1/Bioswale` - Storage, inflow, overflow
- `S1/DetentionPond` - Storage, inflow, overflow
- `S1/RetentionPond` - Storage, inflow, overflow
- `OUT1` - Final discharge flow

In GoldSim, you can track water (and contaminant mass) through each treatment stage.

### LID Types and Their Outputs

Different LID types have different flow paths:

| LID Type | Inflow | Storage | Drain Flow | Overflow |
|----------|--------|---------|------------|----------|
| Rain Barrel | ✓ | ✓ | ✓ (slow) | ✓ |
| Infiltration Trench | ✓ | ✓ | - | ✓ |
| Bioretention/Planter | ✓ | ✓ | ✓ | ✓ |
| Porous Pavement | ✓ | ✓ | ✓ | rare |
| Green Roof | ✓ | ✓ | ✓ | ✓ |
| Vegetative Swale | ✓ | ✓ | - | ✓ |

**Notes:**
- Rain barrels drain slowly between storms (low drain flow during events)
- Porous pavement rarely overflows (designed to infiltrate everything)
- Infiltration trenches and swales don't have underdrains (infiltrate to native soil)
- Use `SURFACE_OUTFLOW` for overflow, `DRAIN_FLOW` for underdrain discharge

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

### For Developers: Custom SWMM5 Build for LID Support

**⚠️ Note for Developers:** The LID output features in v5.212 require modifications to EPA SWMM5 source code. End users can use pre-built DLLs, but if you need to rebuild SWMM5:

This version includes extensions to the EPA SWMM5 API that are **not in the standard SWMM5 release**. To use LID outputs, you must:

1. Download EPA SWMM5 source code from https://github.com/USEPA/Stormwater-Management-Model
2. Add the LID API functions from `swmm5_integration/SWMM5_LID_API_CODE.c` to `src/lid.c`
3. Add the prototypes from `swmm5_integration/SWMM5_LID_API_PROTOTYPES.h` to `src/swmm5.h`
4. Rebuild SWMM5 to generate a custom `swmm5.dll`
5. Regenerate `swmm5.lib` using the provided `swmm5.def` file
6. Rebuild GSswmm.dll with the updated library

**What's Added:**
- 6 new API functions: `swmm_getLidUCount()`, `swmm_getLidUName()`, `swmm_getLidUStorageVolume()`, `swmm_getLidUSurfaceInflow()`, `swmm_getLidUSurfaceOutflow()`, `swmm_getLidUDrainFlow()`
- These expose existing SWMM internal data - no new calculations needed
- See `swmm5_integration/` folder for complete code and instructions

**For End Users:** Pre-built DLLs with LID support are included in releases. You don't need to rebuild SWMM5 unless you're modifying the source code.

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


## License

MIT License - see [LICENSE](LICENSE) file for details.

This software integrates with EPA SWMM5 (public domain) and GoldSim (commercial software). The bridge itself is open source under the MIT License.
