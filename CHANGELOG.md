# Changelog

## [5.212] - 2026-02-01

### Added - LID API Extensions

**New SWMM5 API Functions** (requires rebuilding SWMM5 from source):
- `swmm_getLidUCount()` - Get number of LID units in a subcatchment
- `swmm_getLidUName()` - Get LID control name for a unit
- `swmm_getLidUStorageVolume()` - Get current storage volume
- `swmm_getLidUSurfaceInflow()` - Get inflow rate to LID
- `swmm_getLidUSurfaceOutflow()` - Get overflow rate from LID
- `swmm_getLidUDrainFlow()` - Get underdrain flow rate

**Bridge Enhancements**:
- Support for LID outputs using composite ID format (`Subcatchment/LIDControl`)
- Four LID properties: `STORAGE_VOLUME`, `SURFACE_INFLOW`, `SURFACE_OUTFLOW`, `DRAIN_FLOW`
- Complete water balance tracking for LID treatment trains
- Updated documentation with LID examples

**Files Added**:
- `SWMM5_LID_API_CODE.c` - Implementation code to add to SWMM5 source
- `SWMM5_LID_API_PROTOTYPES.h` - Function prototypes for SWMM5 header
- `ADD_LID_INFLOW.md` - Instructions for SWMM5 developers
- `DRAIN_FLOW_DIAGNOSTIC.md` - Troubleshooting guide

**Use Cases**:
- Contaminant transport modeling through LID treatment trains
- Detailed mass balance for rain barrels, infiltration trenches, planters, green roofs
- Real-time coupling with GoldSim Contaminant Transport Module

### Changed
- Updated README with LID support documentation
- Enhanced `SwmmGoldSimBridge.cpp` to handle LID properties
- Updated `swmm5.def` with new API exports

### Notes
- **Breaking Change**: Requires custom SWMM5 build with LID API extensions
- Standard EPA SWMM5 DLL will not work with LID outputs
- See `SWMM5_LID_API_CODE.c` for code to add to SWMM5 source

---

## [5.202] - Previous Version

- Original GoldSim-SWMM bridge functionality
- Support for standard SWMM outputs (nodes, links, subcatchments)
- JSON-based configuration
- Example models included
