# Add LID Inflow Function to SWMM5 API

## Request

Please add one more function to the existing LID API extensions:

### Function: swmm_getLidUSurfaceInflow()

**Purpose:** Returns the current surface inflow rate to an LID unit (runoff entering from the subcatchment).

---

## Code to Add

### In lid.c (add after the other LID API functions):

```c
/**
 * @brief Get the current surface inflow rate to an LID unit
 * @param subcatchIndex Zero-based subcatchment index
 * @param lidIndex Zero-based LID unit index
 * @return Current surface inflow rate in flow units (CFS or CMS)
 */
double DLLEXPORT swmm_getLidUSurfaceInflow(int subcatchIndex, int lidIndex)
{
    // Validate subcatchment index
    if (subcatchIndex < 0 || subcatchIndex >= Nobjects[SUBCATCH]) {
        report_writeErrorMsg(ERR_API_OBJECT_INDEX, "Subcatchment");
        return 0.0;
    }
    
    TSubcatch* subcatch = &Subcatch[subcatchIndex];
    
    // Validate LID index
    if (lidIndex < 0 || lidIndex >= subcatch->lidCount) {
        report_writeErrorMsg(ERR_API_OBJECT_INDEX, "LID Unit");
        return 0.0;
    }
    
    // Get LID unit
    TLidUnit* lidUnit = subcatch->lidList + lidIndex;
    
    // Return surface inflow rate
    return lidUnit->surfaceInflow;
}
```

### In include/swmm5.h (add with the other LID API prototypes):

```c
double DLLEXPORT swmm_getLidUSurfaceInflow(int subcatchIndex, int lidIndex);
```

---

## Notes

- Follows the exact same pattern as `swmm_getLidUSurfaceOutflow()`
- Simply exposes the existing `lidUnit->surfaceInflow` field
- Enables complete water balance tracking: Inflow → Storage → Overflow
- No new calculations or data structures needed

## Use Case

This completes the LID water balance API, allowing external models to track:
- **Inflow** (new) - Water entering the LID
- **Storage** (existing) - Water stored in the LID  
- **Overflow** (existing) - Water overflowing from the LID

Essential for contaminant transport modeling through LID treatment trains.
