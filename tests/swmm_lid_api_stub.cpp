//-----------------------------------------------------------------------------
//   swmm_lid_api_stub.cpp
//
//   Stub implementation of SWMM5 LID API extensions for testing
//   
//   NOTE: This is a stub implementation for the bridge project testing.
//   The actual implementation should be in EPA SWMM5 source code (lid.c)
//   
//   These stubs provide basic functionality to validate the API design
//   and enable integration testing of the bridge components.
//-----------------------------------------------------------------------------

#include "../include/swmm5.h"
#include <string.h>
#include <stdio.h>

// Stub data structures to simulate SWMM internal state
// In real SWMM5, these would be in globals.h and lid.h

struct StubLidUnit {
    char controlName[64];
    double storageVolume;
    double surfaceOutflow;
};

struct StubSubcatch {
    int lidCount;
    StubLidUnit* lidUnits;
};

// Global stub state
static StubSubcatch* g_stubSubcatchments = nullptr;
static int g_stubSubcatchCount = 0;
static bool g_stubInitialized = false;
static char g_stubErrorMsg[256] = "";

//-----------------------------------------------------------------------------
// Stub initialization (called by test setup)
//-----------------------------------------------------------------------------

extern "C" void SwmmLidStub_Cleanup();

extern "C" void SwmmLidStub_Initialize(int subcatchCount) {
    if (g_stubSubcatchments) {
        SwmmLidStub_Cleanup();
    }
    
    g_stubSubcatchCount = subcatchCount;
    g_stubSubcatchments = new StubSubcatch[subcatchCount];
    
    for (int i = 0; i < subcatchCount; i++) {
        g_stubSubcatchments[i].lidCount = 0;
        g_stubSubcatchments[i].lidUnits = nullptr;
    }
    
    g_stubInitialized = true;
    g_stubErrorMsg[0] = '\0';
}

extern "C" void SwmmLidStub_AddLidUnit(int subcatchIndex, const char* controlName, double initialVolume) {
    if (!g_stubInitialized || subcatchIndex < 0 || subcatchIndex >= g_stubSubcatchCount) {
        return;
    }
    
    StubSubcatch* subcatch = &g_stubSubcatchments[subcatchIndex];
    int newCount = subcatch->lidCount + 1;
    
    StubLidUnit* newUnits = new StubLidUnit[newCount];
    
    // Copy existing units
    for (int i = 0; i < subcatch->lidCount; i++) {
        newUnits[i] = subcatch->lidUnits[i];
    }
    
    // Add new unit
    strncpy_s(newUnits[newCount - 1].controlName, sizeof(newUnits[0].controlName), 
              controlName, _TRUNCATE);
    newUnits[newCount - 1].storageVolume = initialVolume;
    newUnits[newCount - 1].surfaceOutflow = 0.0;
    
    // Replace old array
    delete[] subcatch->lidUnits;
    subcatch->lidUnits = newUnits;
    subcatch->lidCount = newCount;
}

extern "C" void SwmmLidStub_SetSurfaceOutflow(int subcatchIndex, int lidIndex, double outflow) {
    if (!g_stubInitialized || subcatchIndex < 0 || subcatchIndex >= g_stubSubcatchCount) {
        return;
    }
    
    StubSubcatch* subcatch = &g_stubSubcatchments[subcatchIndex];
    
    if (lidIndex < 0 || lidIndex >= subcatch->lidCount) {
        return;
    }
    
    subcatch->lidUnits[lidIndex].surfaceOutflow = outflow;
}

extern "C" void SwmmLidStub_Cleanup() {
    if (g_stubSubcatchments) {
        for (int i = 0; i < g_stubSubcatchCount; i++) {
            delete[] g_stubSubcatchments[i].lidUnits;
        }
        delete[] g_stubSubcatchments;
        g_stubSubcatchments = nullptr;
    }
    
    g_stubSubcatchCount = 0;
    g_stubInitialized = false;
    g_stubErrorMsg[0] = '\0';
}

//-----------------------------------------------------------------------------
// LID API Implementation (Requirement 1: LID Unit Enumeration)
//-----------------------------------------------------------------------------

/**
 * @brief Get the number of LID units in a subcatchment
 * @param subcatchIndex Zero-based subcatchment index
 * @return Number of LID units (>= 0), or -1 if subcatchment index is invalid
 * 
 * Validates: Requirements 1.1, 1.2, 1.3, 1.4
 */
extern "C" int DLLEXPORT swmm_getLidUCount(int subcatchIndex)
{
    // Validate initialization
    if (!g_stubInitialized) {
        snprintf(g_stubErrorMsg, sizeof(g_stubErrorMsg), 
                 "LID API Error: Function called before swmm_start()");
        return -1;
    }
    
    // Validate subcatchment index range (Requirement 1.3)
    if (subcatchIndex < 0 || subcatchIndex >= g_stubSubcatchCount) {
        snprintf(g_stubErrorMsg, sizeof(g_stubErrorMsg), 
                 "LID API Error: Invalid subcatchment index %d", subcatchIndex);
        return -1;
    }
    
    // Return LID unit count (Requirements 1.1, 1.2)
    return g_stubSubcatchments[subcatchIndex].lidCount;
}

//-----------------------------------------------------------------------------
// LID API Implementation (Requirement 2: LID Unit Identification)
//-----------------------------------------------------------------------------

/**
 * @brief Get the LID control name for a specific LID unit
 * @param subcatchIndex Zero-based subcatchment index
 * @param lidIndex Zero-based LID unit index
 * @param name Buffer to receive the LID control name
 * @param size Size of the name buffer
 * 
 * Validates: Requirements 2.1, 2.2, 2.3, 2.4
 */
extern "C" void DLLEXPORT swmm_getLidUName(int subcatchIndex, int lidIndex, 
                                            char* name, int size)
{
    // Initialize output buffer
    if (name && size > 0) {
        name[0] = '\0';
    }
    
    // Validate parameters
    if (!name || size <= 0) {
        snprintf(g_stubErrorMsg, sizeof(g_stubErrorMsg), 
                 "LID API Error: NULL buffer provided");
        return;
    }
    
    // Validate initialization
    if (!g_stubInitialized) {
        snprintf(g_stubErrorMsg, sizeof(g_stubErrorMsg), 
                 "LID API Error: Function called before swmm_start()");
        return;
    }
    
    // Validate subcatchment index (Requirement 2.2)
    if (subcatchIndex < 0 || subcatchIndex >= g_stubSubcatchCount) {
        snprintf(g_stubErrorMsg, sizeof(g_stubErrorMsg), 
                 "LID API Error: Invalid subcatchment index %d", subcatchIndex);
        return;
    }
    
    StubSubcatch* subcatch = &g_stubSubcatchments[subcatchIndex];
    
    // Validate LID index (Requirement 2.2)
    if (lidIndex < 0 || lidIndex >= subcatch->lidCount) {
        snprintf(g_stubErrorMsg, sizeof(g_stubErrorMsg), 
                 "LID API Error: Invalid LID unit index %d", lidIndex);
        return;
    }
    
    // Copy name to buffer with size limit (Requirement 2.3)
    strncpy_s(name, size, subcatch->lidUnits[lidIndex].controlName, _TRUNCATE);
}

//-----------------------------------------------------------------------------
// LID API Implementation (Requirement 3: LID Storage Volume Access)
//-----------------------------------------------------------------------------

/**
 * @brief Get the current storage volume in an LID unit
 * @param subcatchIndex Zero-based subcatchment index
 * @param lidIndex Zero-based LID unit index
 * @return Current storage volume in cubic feet (or cubic meters)
 * 
 * Validates: Requirements 3.1, 3.2, 3.3, 3.4, 3.5, 3.6
 */
extern "C" double DLLEXPORT swmm_getLidUStorageVolume(int subcatchIndex, int lidIndex)
{
    // Validate initialization
    if (!g_stubInitialized) {
        snprintf(g_stubErrorMsg, sizeof(g_stubErrorMsg), 
                 "LID API Error: Function called before swmm_start()");
        return 0.0;
    }
    
    // Validate subcatchment index (Requirement 3.3)
    if (subcatchIndex < 0 || subcatchIndex >= g_stubSubcatchCount) {
        snprintf(g_stubErrorMsg, sizeof(g_stubErrorMsg), 
                 "LID API Error: Invalid subcatchment index %d", subcatchIndex);
        return 0.0;
    }
    
    StubSubcatch* subcatch = &g_stubSubcatchments[subcatchIndex];
    
    // Validate LID index (Requirement 3.3)
    if (lidIndex < 0 || lidIndex >= subcatch->lidCount) {
        snprintf(g_stubErrorMsg, sizeof(g_stubErrorMsg), 
                 "LID API Error: Invalid LID unit index %d", lidIndex);
        return 0.0;
    }
    
    // Return storage volume (Requirements 3.1, 3.2, 3.4, 3.5, 3.6)
    // Volume is always non-negative (Requirement 3.6)
    double volume = subcatch->lidUnits[lidIndex].storageVolume;
    return (volume >= 0.0) ? volume : 0.0;
}

//-----------------------------------------------------------------------------
// LID API Implementation (Requirement 4: LID Surface Overflow Access)
//-----------------------------------------------------------------------------

/**
 * @brief Get the current surface overflow rate from an LID unit
 * @param subcatchIndex Zero-based subcatchment index
 * @param lidIndex Zero-based LID unit index
 * @return Current surface overflow rate in flow units (CFS or CMS)
 * 
 * Validates: Surface overflow retrieval for LID units
 */
extern "C" double DLLEXPORT swmm_getLidUSurfaceOutflow(int subcatchIndex, int lidIndex)
{
    // Validate initialization
    if (!g_stubInitialized) {
        snprintf(g_stubErrorMsg, sizeof(g_stubErrorMsg), 
                 "LID API Error: Function called before swmm_start()");
        return 0.0;
    }
    
    // Validate subcatchment index
    if (subcatchIndex < 0 || subcatchIndex >= g_stubSubcatchCount) {
        snprintf(g_stubErrorMsg, sizeof(g_stubErrorMsg), 
                 "LID API Error: Invalid subcatchment index %d", subcatchIndex);
        return 0.0;
    }
    
    StubSubcatch* subcatch = &g_stubSubcatchments[subcatchIndex];
    
    // Validate LID index
    if (lidIndex < 0 || lidIndex >= subcatch->lidCount) {
        snprintf(g_stubErrorMsg, sizeof(g_stubErrorMsg), 
                 "LID API Error: Invalid LID unit index %d", lidIndex);
        return 0.0;
    }
    
    // Return surface overflow rate
    // In real SWMM, this would return lidUnit->surfaceOutflow
    return subcatch->lidUnits[lidIndex].surfaceOutflow;
}

//-----------------------------------------------------------------------------
// Error message retrieval (integrates with existing swmm_getError)
//-----------------------------------------------------------------------------

extern "C" const char* SwmmLidStub_GetLastError() {
    return g_stubErrorMsg;
}
