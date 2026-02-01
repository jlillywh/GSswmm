/**
 * Direct test of bridge with LID API
 * Tests that the bridge can initialize and resolve LID outputs
 */
#include <windows.h>
#include <stdio.h>
#include <string.h>

typedef void (*BridgeFunc)(int, int*, double*, double*);

int main() {
    printf("========================================\n");
    printf("Bridge LID API Direct Test\n");
    printf("========================================\n\n");
    
    // Load bridge DLL
    HMODULE bridge = LoadLibraryA("GSswmm.dll");
    if (!bridge) {
        printf("ERROR: Failed to load GSswmm.dll\n");
        return 1;
    }
    printf("[PASS] Bridge DLL loaded\n");
    
    // Get bridge function
    BridgeFunc func = (BridgeFunc)GetProcAddress(bridge, "SwmmGoldSimBridge");
    if (!func) {
        printf("ERROR: Failed to get SwmmGoldSimBridge function\n");
        FreeLibrary(bridge);
        return 1;
    }
    printf("[PASS] Bridge function found\n\n");
    
    int status;
    double inargs[10] = {0};
    double outargs[20] = {0};
    
    // Test 1: Get version
    printf("[Test 1] Get version...\n");
    func(2, &status, inargs, outargs);
    if (status == 0) {
        printf("[PASS] Version: %.1f\n\n", outargs[0]);
    } else {
        printf("[FAIL] Version check failed\n\n");
    }
    
    // Test 2: Get arguments
    printf("[Test 2] Get arguments...\n");
    func(3, &status, inargs, outargs);
    if (status == 0) {
        printf("[PASS] Inputs: %.0f, Outputs: %.0f\n\n", outargs[0], outargs[1]);
    } else {
        printf("[FAIL] Get arguments failed\n\n");
    }
    
    // Test 3: Initialize
    printf("[Test 3] Initialize bridge...\n");
    func(0, &status, inargs, outargs);
    if (status == 0) {
        printf("[PASS] Bridge initialized successfully\n");
        printf("[INFO] Check bridge_debug.log for LID resolution details\n\n");
        
        // Test 4: Calculate (get initial outputs)
        printf("[Test 4] Get initial outputs...\n");
        inargs[0] = 0.0;  // ElapsedTime = 0
        func(1, &status, inargs, outargs);
        if (status == 0) {
            printf("[PASS] Calculate succeeded\n");
            printf("[INFO] Output values:\n");
            for (int i = 0; i < 18; i++) {
                printf("  Output[%d]: %.6f\n", i, outargs[i]);
            }
        } else {
            printf("[FAIL] Calculate failed, status=%d\n", status);
            if (status == -1) {
                char* err_msg = (char*)(ULONG_PTR)outargs[0];
                printf("[ERROR] %s\n", err_msg);
            }
        }
        
        // Cleanup
        printf("\n[Test 5] Cleanup...\n");
        func(99, &status, inargs, outargs);
        printf("[PASS] Cleanup complete\n");
    } else {
        printf("[FAIL] Bridge initialization failed, status=%d\n", status);
        if (status == -1) {
            char* err_msg = (char*)(ULONG_PTR)outargs[0];
            printf("[ERROR] %s\n", err_msg);
        }
    }
    
    FreeLibrary(bridge);
    
    printf("\n========================================\n");
    printf("Test Complete\n");
    printf("========================================\n");
    printf("Check bridge_debug.log for detailed LID resolution info\n");
    
    return 0;
}
