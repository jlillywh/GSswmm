/**
 * Simple program to check if LID API functions are available
 */
#include <stdio.h>
#include "../include/swmm5.h"

int main() {
    printf("Checking LID API function availability...\n\n");
    
    // Try to call the functions (they will fail without initialization, but that's OK)
    // We just want to see if they link
    
    printf("Testing swmm_getLidUCount...\n");
    int count = swmm_getLidUCount(0);
    printf("  Result: %d (expected -1 for uninitialized)\n\n", count);
    
    printf("Testing swmm_getLidUName...\n");
    char name[64];
    swmm_getLidUName(0, 0, name, sizeof(name));
    printf("  Result: '%s' (expected empty for uninitialized)\n\n", name);
    
    printf("Testing swmm_getLidUStorageVolume...\n");
    double volume = swmm_getLidUStorageVolume(0, 0);
    printf("  Result: %.2f (expected 0.0 for uninitialized)\n\n", volume);
    
    printf("SUCCESS: All LID API functions are available!\n");
    
    return 0;
}
