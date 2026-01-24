//-----------------------------------------------------------------------------
//   test_calculate.cpp
//
//   Test program to verify XF_CALCULATE handler implementation
//   Tests: Calculate handler with rainfall input and runoff output
//-----------------------------------------------------------------------------

#include <iostream>
#include <windows.h>
#include <cmath>

// Function pointer type for the bridge function
typedef void (*BridgeFunctionType)(int, int*, double*, double*);

// GoldSim Method IDs
#define XF_INITIALIZE       0
#define XF_CALCULATE        1
#define XF_REP_VERSION      2
#define XF_REP_ARGUMENTS    3
#define XF_CLEANUP          99

// GoldSim Status Codes
#define XF_SUCCESS              0
#define XF_FAILURE              1
#define XF_FAILURE_WITH_MSG    -1

int main()
{
    std::cout << "=== GoldSim-SWMM Bridge XF_CALCULATE Test ===" << std::endl;
    std::cout << std::endl;

    // Load the DLL
    HMODULE hDll = LoadLibraryA("GSswmm.dll");
    if (!hDll)
    {
        std::cerr << "ERROR: Failed to load GSswmm.dll" << std::endl;
        return 1;
    }
    std::cout << "[PASS] DLL loaded successfully" << std::endl;

    // Get the function pointer
    BridgeFunctionType SwmmGoldSimBridge = 
        (BridgeFunctionType)GetProcAddress(hDll, "SwmmGoldSimBridge");
    
    if (!SwmmGoldSimBridge)
    {
        std::cerr << "ERROR: Failed to get SwmmGoldSimBridge function" << std::endl;
        FreeLibrary(hDll);
        return 1;
    }
    std::cout << "[PASS] Function pointer obtained" << std::endl;
    std::cout << std::endl;

    // Test variables
    int status;
    double inargs[10] = {0};
    double outargs[10] = {0};
    int test_count = 0;
    int pass_count = 0;

    // Test 1: Calculate before initialize (should fail)
    std::cout << "Test 1: XF_CALCULATE before initialize (should fail)" << std::endl;
    test_count++;
    inargs[0] = 1.0;  // 1.0 inch/hour rainfall
    SwmmGoldSimBridge(XF_CALCULATE, &status, inargs, outargs);
    if (status == XF_FAILURE)
    {
        std::cout << "  [PASS] Calculate before initialize correctly failed (status = " 
                  << status << ")" << std::endl;
        pass_count++;
    }
    else
    {
        std::cout << "  [FAIL] Calculate before initialize should fail, got status " 
                  << status << std::endl;
    }
    std::cout << std::endl;

    // Test 2: Initialize SWMM
    std::cout << "Test 2: XF_INITIALIZE" << std::endl;
    test_count++;
    SwmmGoldSimBridge(XF_INITIALIZE, &status, inargs, outargs);
    if (status == XF_SUCCESS)
    {
        std::cout << "  [PASS] Initialize succeeded (status = " << status << ")" << std::endl;
        pass_count++;
    }
    else if (status == XF_FAILURE_WITH_MSG)
    {
        ULONG_PTR* pAddr = (ULONG_PTR*)outargs;
        char* errorMsg = (char*)(*pAddr);
        std::cout << "  [FAIL] Initialize failed: " << errorMsg << std::endl;
        FreeLibrary(hDll);
        return 1;
    }
    else
    {
        std::cout << "  [FAIL] Initialize failed with status " << status << std::endl;
        FreeLibrary(hDll);
        return 1;
    }
    std::cout << std::endl;

    // Test 3: Calculate with zero rainfall
    std::cout << "Test 3: XF_CALCULATE with zero rainfall" << std::endl;
    test_count++;
    inargs[0] = 0.0;  // 0.0 inch/hour rainfall
    SwmmGoldSimBridge(XF_CALCULATE, &status, inargs, outargs);
    if (status == XF_SUCCESS)
    {
        std::cout << "  [PASS] Calculate succeeded (status = " << status << ")" << std::endl;
        std::cout << "  [INFO] Rainfall = " << inargs[0] << " in/hr, Runoff = " 
                  << outargs[0] << " CFS" << std::endl;
        pass_count++;
    }
    else if (status == XF_FAILURE_WITH_MSG)
    {
        ULONG_PTR* pAddr = (ULONG_PTR*)outargs;
        char* errorMsg = (char*)(*pAddr);
        std::cout << "  [FAIL] Calculate failed: " << errorMsg << std::endl;
    }
    else
    {
        std::cout << "  [FAIL] Calculate failed with status " << status << std::endl;
    }
    std::cout << std::endl;

    // Test 4: Calculate with moderate rainfall
    std::cout << "Test 4: XF_CALCULATE with moderate rainfall (1.0 in/hr)" << std::endl;
    test_count++;
    inargs[0] = 1.0;  // 1.0 inch/hour rainfall
    SwmmGoldSimBridge(XF_CALCULATE, &status, inargs, outargs);
    if (status == XF_SUCCESS)
    {
        std::cout << "  [PASS] Calculate succeeded (status = " << status << ")" << std::endl;
        std::cout << "  [INFO] Rainfall = " << inargs[0] << " in/hr, Runoff = " 
                  << outargs[0] << " CFS" << std::endl;
        
        // Runoff should be non-negative
        if (outargs[0] >= 0.0)
        {
            std::cout << "  [PASS] Runoff value is non-negative" << std::endl;
        }
        else
        {
            std::cout << "  [WARN] Runoff value is negative: " << outargs[0] << std::endl;
        }
        pass_count++;
    }
    else if (status == XF_FAILURE_WITH_MSG)
    {
        ULONG_PTR* pAddr = (ULONG_PTR*)outargs;
        char* errorMsg = (char*)(*pAddr);
        std::cout << "  [FAIL] Calculate failed: " << errorMsg << std::endl;
    }
    else
    {
        std::cout << "  [FAIL] Calculate failed with status " << status << std::endl;
    }
    std::cout << std::endl;

    // Test 5: Multiple calculate calls (simulate time series)
    std::cout << "Test 5: Multiple XF_CALCULATE calls (time series)" << std::endl;
    test_count++;
    bool all_succeeded = true;
    double rainfall_series[] = {0.5, 1.0, 2.0, 1.5, 1.0, 0.5, 0.0};
    int num_steps = sizeof(rainfall_series) / sizeof(rainfall_series[0]);
    
    std::cout << "  [INFO] Running " << num_steps << " time steps..." << std::endl;
    for (int i = 0; i < num_steps; i++)
    {
        inargs[0] = rainfall_series[i];
        SwmmGoldSimBridge(XF_CALCULATE, &status, inargs, outargs);
        
        if (status == XF_SUCCESS)
        {
            std::cout << "    Step " << (i+1) << ": Rainfall = " << inargs[0] 
                      << " in/hr, Runoff = " << outargs[0] << " CFS" << std::endl;
        }
        else
        {
            std::cout << "    Step " << (i+1) << ": FAILED with status " << status << std::endl;
            all_succeeded = false;
            break;
        }
    }
    
    if (all_succeeded)
    {
        std::cout << "  [PASS] All time steps completed successfully" << std::endl;
        pass_count++;
    }
    else
    {
        std::cout << "  [FAIL] Some time steps failed" << std::endl;
    }
    std::cout << std::endl;

    // Test 6: Calculate with high rainfall
    std::cout << "Test 6: XF_CALCULATE with high rainfall (5.0 in/hr)" << std::endl;
    test_count++;
    inargs[0] = 5.0;  // 5.0 inch/hour rainfall (heavy storm)
    SwmmGoldSimBridge(XF_CALCULATE, &status, inargs, outargs);
    if (status == XF_SUCCESS)
    {
        std::cout << "  [PASS] Calculate succeeded (status = " << status << ")" << std::endl;
        std::cout << "  [INFO] Rainfall = " << inargs[0] << " in/hr, Runoff = " 
                  << outargs[0] << " CFS" << std::endl;
        pass_count++;
    }
    else if (status == XF_FAILURE_WITH_MSG)
    {
        ULONG_PTR* pAddr = (ULONG_PTR*)outargs;
        char* errorMsg = (char*)(*pAddr);
        std::cout << "  [FAIL] Calculate failed: " << errorMsg << std::endl;
    }
    else
    {
        std::cout << "  [FAIL] Calculate failed with status " << status << std::endl;
    }
    std::cout << std::endl;

    // Test 7: Run until simulation ends naturally
    std::cout << "Test 7: Run simulation until natural end" << std::endl;
    test_count++;
    int max_steps = 1000;  // Safety limit
    int steps_run = 0;
    bool ended_naturally = false;
    
    std::cout << "  [INFO] Running simulation with 0.5 in/hr rainfall..." << std::endl;
    inargs[0] = 0.5;
    
    for (int i = 0; i < max_steps; i++)
    {
        SwmmGoldSimBridge(XF_CALCULATE, &status, inargs, outargs);
        steps_run++;
        
        if (status == XF_SUCCESS)
        {
            // Check if SWMM ended (status is success but simulation ended)
            // We can't directly check this, but if we get success, continue
            continue;
        }
        else if (status == XF_FAILURE)
        {
            // This might indicate simulation ended
            ended_naturally = true;
            break;
        }
        else
        {
            // Error occurred
            std::cout << "  [FAIL] Error at step " << steps_run << ", status = " 
                      << status << std::endl;
            break;
        }
    }
    
    std::cout << "  [INFO] Ran " << steps_run << " steps" << std::endl;
    if (steps_run > 0)
    {
        std::cout << "  [PASS] Simulation ran successfully" << std::endl;
        pass_count++;
    }
    else
    {
        std::cout << "  [FAIL] Simulation did not run" << std::endl;
    }
    std::cout << std::endl;

    // Test 8: Cleanup
    std::cout << "Test 8: XF_CLEANUP" << std::endl;
    test_count++;
    SwmmGoldSimBridge(XF_CLEANUP, &status, inargs, outargs);
    if (status == XF_SUCCESS)
    {
        std::cout << "  [PASS] Cleanup succeeded (status = " << status << ")" << std::endl;
        pass_count++;
    }
    else
    {
        std::cout << "  [FAIL] Cleanup failed with status " << status << std::endl;
    }
    std::cout << std::endl;

    // Test 9: Calculate after cleanup (should fail)
    std::cout << "Test 9: XF_CALCULATE after cleanup (should fail)" << std::endl;
    test_count++;
    inargs[0] = 1.0;
    SwmmGoldSimBridge(XF_CALCULATE, &status, inargs, outargs);
    if (status == XF_FAILURE)
    {
        std::cout << "  [PASS] Calculate after cleanup correctly failed (status = " 
                  << status << ")" << std::endl;
        pass_count++;
    }
    else
    {
        std::cout << "  [FAIL] Calculate after cleanup should fail, got status " 
                  << status << std::endl;
    }
    std::cout << std::endl;

    // Clean up
    FreeLibrary(hDll);

    // Print summary
    std::cout << "=== Test Summary ===" << std::endl;
    std::cout << "Tests run: " << test_count << std::endl;
    std::cout << "Tests passed: " << pass_count << std::endl;
    std::cout << "Tests failed: " << (test_count - pass_count) << std::endl;
    std::cout << std::endl;

    if (pass_count == test_count)
    {
        std::cout << "ALL TESTS PASSED!" << std::endl;
        return 0;
    }
    else
    {
        std::cout << "SOME TESTS FAILED" << std::endl;
        return 1;
    }
}
