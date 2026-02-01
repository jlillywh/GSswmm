//-----------------------------------------------------------------------------
//   test_lid_overflow.cpp
//
//   Test for swmm_getLidUSurfaceOutflow API function
//   
//   Tests the new overflow retrieval functionality for LID units
//-----------------------------------------------------------------------------

#include "../include/swmm5.h"
#include <stdio.h>
#include <string.h>

// Stub helper functions
extern "C" void SwmmLidStub_Initialize(int subcatchCount);
extern "C" void SwmmLidStub_AddLidUnit(int subcatchIndex, const char* controlName, double initialVolume);
extern "C" void SwmmLidStub_SetSurfaceOutflow(int subcatchIndex, int lidIndex, double outflow);
extern "C" void SwmmLidStub_Cleanup();

int main() {
    printf("Testing swmm_getLidUSurfaceOutflow API function\n");
    printf("================================================\n\n");
    
    // Initialize stub with 1 subcatchment
    SwmmLidStub_Initialize(1);
    
    // Add LID unit: InfilTrench with initial volume
    SwmmLidStub_AddLidUnit(0, "InfilTrench", 500.0);
    
    // Test 1: Get overflow when none exists
    printf("Test 1: Get overflow with no overflow condition\n");
    double overflow = swmm_getLidUSurfaceOutflow(0, 0);
    printf("  Overflow rate: %.2f CFS\n", overflow);
    printf("  Expected: 0.00 CFS\n");
    printf("  Result: %s\n\n", (overflow == 0.0) ? "PASS" : "FAIL");
    
    // Test 2: Set overflow and retrieve it
    printf("Test 2: Get overflow with active overflow\n");
    SwmmLidStub_SetSurfaceOutflow(0, 0, 2.5);
    overflow = swmm_getLidUSurfaceOutflow(0, 0);
    printf("  Overflow rate: %.2f CFS\n", overflow);
    printf("  Expected: 2.50 CFS\n");
    printf("  Result: %s\n\n", (overflow == 2.5) ? "PASS" : "FAIL");
    
    // Test 3: Invalid subcatchment index
    printf("Test 3: Invalid subcatchment index\n");
    overflow = swmm_getLidUSurfaceOutflow(99, 0);
    printf("  Overflow rate: %.2f CFS\n", overflow);
    printf("  Expected: 0.00 CFS (error)\n");
    printf("  Result: %s\n\n", (overflow == 0.0) ? "PASS" : "FAIL");
    
    // Test 4: Invalid LID index
    printf("Test 4: Invalid LID index\n");
    overflow = swmm_getLidUSurfaceOutflow(0, 99);
    printf("  Overflow rate: %.2f CFS\n", overflow);
    printf("  Expected: 0.00 CFS (error)\n");
    printf("  Result: %s\n\n", (overflow == 0.0) ? "PASS" : "FAIL");
    
    // Test 5: Multiple LID units
    printf("Test 5: Multiple LID units with different overflow rates\n");
    SwmmLidStub_AddLidUnit(0, "RainBarrel", 100.0);
    SwmmLidStub_SetSurfaceOutflow(0, 0, 1.5);  // InfilTrench
    SwmmLidStub_SetSurfaceOutflow(0, 1, 0.3);  // RainBarrel
    
    double overflow1 = swmm_getLidUSurfaceOutflow(0, 0);
    double overflow2 = swmm_getLidUSurfaceOutflow(0, 1);
    
    printf("  InfilTrench overflow: %.2f CFS\n", overflow1);
    printf("  RainBarrel overflow: %.2f CFS\n", overflow2);
    printf("  Result: %s\n\n", 
           (overflow1 == 1.5 && overflow2 == 0.3) ? "PASS" : "FAIL");
    
    // Cleanup
    SwmmLidStub_Cleanup();
    
    printf("\nAll tests completed!\n");
    return 0;
}
