# SWMM5 LID API Integration Files

This folder contains code to add to EPA SWMM5 source to enable LID API functionality.

## Files

- **SWMM5_LID_API_CODE.c** - Function implementations to add to `SWMM5-source/src/lid.c`
- **SWMM5_LID_API_PROTOTYPES.h** - Function prototypes to add to `SWMM5-source/src/swmm5.h`
- **ADD_LID_INFLOW.md** - Instructions for adding the inflow function

## Quick Integration

1. Open your SWMM5 source code
2. Add code from `SWMM5_LID_API_CODE.c` to the end of `src/lid.c`
3. Add prototypes from `SWMM5_LID_API_PROTOTYPES.h` to `src/swmm5.h`
4. Rebuild SWMM5 to generate updated `swmm5.dll`

## Functions Added

- `swmm_getLidUCount()` - Get number of LID units
- `swmm_getLidUName()` - Get LID control name
- `swmm_getLidUStorageVolume()` - Get storage volume
- `swmm_getLidUSurfaceInflow()` - Get inflow rate
- `swmm_getLidUSurfaceOutflow()` - Get overflow rate
- `swmm_getLidUDrainFlow()` - Get drain flow rate

These functions expose existing SWMM internal data through the API - no new calculations needed.
