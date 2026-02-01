# GSswmm Project Structure

## Root Directory

### Core Files
- **README.md** - Main documentation
- **CHANGELOG.md** - Version history
- **SwmmGoldSimBridge.cpp** - Bridge implementation
- **MappingLoader.cpp** - JSON configuration loader
- **generate_mapping.py** - Mapping generator script
- **swmm5.dll** - SWMM runtime (custom build with LID API)
- **swmm5.def** - DLL export definitions

### Build Files
- **GSswmm.sln** - Visual Studio solution
- **GSswmm.vcxproj** - Project file
- **SwmmGoldSimBridge.json** - Example configuration

## Folders

### `/swmm5_integration/`
Code to add to EPA SWMM5 source for LID API support
- `SWMM5_LID_API_CODE.c` - Function implementations
- `SWMM5_LID_API_PROTOTYPES.h` - Function prototypes
- `ADD_LID_INFLOW.md` - Integration instructions

### `/include/`
Header files
- `swmm5.h` - SWMM API header (with LID extensions)
- `MappingLoader.h` - Mapping loader header

### `/lib/`
Import libraries
- `swmm5.lib` - SWMM import library

### `/examples/`
Example SWMM models with GoldSim integration
- `Simple_Model/` - Basic example
- `SiteDrainageModel/` - Multi-subcatchment
- `PumpControl/` - Structure control
- `LID Treatment/` - LID treatment train

### `/tests/`
Test files and validation scripts

### `/scripts/`
Build and utility scripts

## Key Features

**Version 5.212** includes:
- Standard SWMM outputs (nodes, links, subcatchments)
- LID unit outputs (storage, inflow, outflow, drain)
- JSON-based configuration
- Real-time coupling with GoldSim
- Complete water balance tracking
