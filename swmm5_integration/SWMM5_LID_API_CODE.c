// =============================================================================
// ADD THIS CODE TO: SWMM5-source/src/lid.c
// Location: At the end of the file, before the last closing brace
// =============================================================================

//=============================================================================
// LID API Extensions
//=============================================================================

/**
 * @brief Get the number of LID units in a subcatchment
 * @param subcatchIndex Zero-based subcatchment index
 * @return Number of LID units (>= 0), or -1 if subcatchment index is invalid
 */
int DLLEXPORT swmm_getLidUCount(int subcatchIndex)
{
    // Validate subcatchment index
    if (subcatchIndex < 0 || subcatchIndex >= Nobjects[SUBCATCH]) {
        report_writeErrorMsg(ERR_API_OBJECT_INDEX, "Subcatchment");
        return -1;
    }
    
    // Return LID unit count
    TSubcatch* subcatch = &Subcatch[subcatchIndex];
    return subcatch->lidCount;
}

/**
 * @brief Get the LID control name for a specific LID unit
 * @param subcatchIndex Zero-based subcatchment index
 * @param lidIndex Zero-based LID unit index
 * @param name Buffer to receive the LID control name
 * @param size Size of the name buffer
 */
void DLLEXPORT swmm_getLidUName(int subcatchIndex, int lidIndex, char* name, int size)
{
    // Initialize output
    if (name && size > 0) {
        name[0] = '\0';
    }
    
    // Validate parameters
    if (!name || size <= 0) {
        report_writeErrorMsg(ERR_API_OUTBOUNDS, "Buffer");
        return;
    }
    
    // Validate subcatchment index
    if (subcatchIndex < 0 || subcatchIndex >= Nobjects[SUBCATCH]) {
        report_writeErrorMsg(ERR_API_OBJECT_INDEX, "Subcatchment");
        return;
    }
    
    TSubcatch* subcatch = &Subcatch[subcatchIndex];
    
    // Validate LID index
    if (lidIndex < 0 || lidIndex >= subcatch->lidCount) {
        report_writeErrorMsg(ERR_API_OBJECT_INDEX, "LID Unit");
        return;
    }
    
    // Get LID unit and copy name
    TLidUnit* lidUnit = subcatch->lidList + lidIndex;
    int lidControlIndex = lidUnit->lidIndex;
    
    if (lidControlIndex >= 0 && lidControlIndex < Nobjects[LID]) {
        strncpy(name, LidProcs[lidControlIndex].ID, size - 1);
        name[size - 1] = '\0';
    }
}

/**
 * @brief Get the current storage volume in an LID unit
 * @param subcatchIndex Zero-based subcatchment index
 * @param lidIndex Zero-based LID unit index
 * @return Current storage volume in cubic feet (or cubic meters)
 */
double DLLEXPORT swmm_getLidUStorageVolume(int subcatchIndex, int lidIndex)
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
    
    // Calculate total storage volume from all layers
    double volume = 0.0;
    double area = lidUnit->area * lidUnit->number;  // Total LID area
    
    // Surface layer storage
    if (lidUnit->surfaceDepth > 0.0) {
        volume += lidUnit->surfaceDepth * area;
    }
    
    // Soil layer storage
    if (lidUnit->soilMoisture > 0.0) {
        TLidProc* lidProc = &LidProcs[lidUnit->lidIndex];
        if (lidProc->soil.thickness > 0.0) {
            volume += lidUnit->soilMoisture * lidProc->soil.thickness * 
                      area * lidProc->soil.porosity;
        }
    }
    
    // Storage layer
    if (lidUnit->storageDepth > 0.0) {
        TLidProc* lidProc = &LidProcs[lidUnit->lidIndex];
        if (lidProc->storage.thickness > 0.0) {
            volume += lidUnit->storageDepth * area * lidProc->storage.voidFrac;
        }
    }
    
    // Pavement layer storage
    if (lidUnit->paveDepth > 0.0) {
        TLidProc* lidProc = &LidProcs[lidUnit->lidIndex];
        if (lidProc->pavement.thickness > 0.0) {
            volume += lidUnit->paveDepth * area * lidProc->pavement.voidFrac;
        }
    }
    
    return volume;
}

/**
 * @brief Get the current surface overflow rate from an LID unit
 * @param subcatchIndex Zero-based subcatchment index
 * @param lidIndex Zero-based LID unit index
 * @return Current surface overflow rate in flow units (CFS or CMS)
 */
double DLLEXPORT swmm_getLidUSurfaceOutflow(int subcatchIndex, int lidIndex)
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
    
    // Return surface overflow rate
    // This represents water that exceeds LID capacity and becomes runoff
    return lidUnit->surfaceOutflow;
}

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
    // This represents runoff entering the LID from the subcatchment
    return lidUnit->surfaceInflow;
}
