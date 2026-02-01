// =============================================================================
// ADD THESE LINES TO: SWMM5-source/src/swmm5.h
// Location: After the existing API function declarations (around line 150)
// Look for other "DLLEXPORT" functions and add these after them
// =============================================================================

// LID API Extensions - Get LID unit information
int    DLLEXPORT swmm_getLidUCount(int subcatchIndex);
void   DLLEXPORT swmm_getLidUName(int subcatchIndex, int lidIndex, char* name, int size);
double DLLEXPORT swmm_getLidUStorageVolume(int subcatchIndex, int lidIndex);
double DLLEXPORT swmm_getLidUSurfaceOutflow(int subcatchIndex, int lidIndex);
double DLLEXPORT swmm_getLidUSurfaceInflow(int subcatchIndex, int lidIndex);
