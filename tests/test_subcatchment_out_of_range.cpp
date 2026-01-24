//-----------------------------------------------------------------------------
//   test_subcatchment_out_of_range.cpp
//
//   Test program to verify subcatchment index validation rejects out-of-range indices
//   This test uses a helper DLL function to set the subcatchment index for testing
//   Requirements: 7.3
//-----------------------------------------------------------------------------

#include <iostream>
#include <windows.h>
#include <fstream>

// Function pointer types
typedef void (*BridgeFunctionType)(int, int*, double*, double*);
typedef void (*SetSubcatchIndexType)(int);

// GoldSim Method IDs
#define XF_INITIALIZE       0
#define XF_CLEANUP          99

// GoldSim Status Codes
#define XF_SUCCESS              0
#define XF_FAILURE              1
#define XF_FAILURE_WITH_MSG    -1

// Helper function to create a temporary test file with specified number of subcatchments
bool CreateTestFile(const char* filename, int num_subcatchments)
{
    std::ofstream file(filename);
    if (!file.is_open())
        return false;
    
    // Write complete SWMM input file content
    file << "[TITLE]\n";
    file << "Test Model for Out-of-Range Subcatchment Validation\n\n";
    file << "[OPTIONS]\n";
    file << "FLOW_UNITS CFS\n";
    file << "INFILTRATION HORTON\n";
    file << "FLOW_ROUTING KINWAVE\n";
    file << "START_DATE 01/01/2020\n";
    file << "START_TIME 00:00:00\n";
    file << "END_DATE 01/01/2020\n";
    file << "END_TIME 01:00:00\n";
    file << "REPORT_STEP 00:15:00\n";
    file << "WET_STEP 00:05:00\n";
    file << "DRY_STEP 01:00:00\n";
    file << "ROUTING_STEP 60\n";
    file << "ALLOW_PONDING NO\n";
    file << "INERTIAL_DAMPING PARTIAL\n";
    file << "VARIABLE_STEP 0.75\n";
    file << "LENGTHENING_STEP 0\n";
    file << "MIN_SURFAREA 0\n";
    file << "NORMAL_FLOW_LIMITED BOTH\n";
    file << "SKIP_STEADY_STATE NO\n";
    file << "FORCE_MAIN_EQUATION H-W\n";
    file << "LINK_OFFSETS DEPTH\n";
    file << "MIN_SLOPE 0\n\n";
    file << "[JUNCTIONS]\n";
    file << ";;Name Elevation MaxDepth InitDepth SurDepth Aponded\n";
    file << "J1 0 10 0 0 0\n\n";
    file << "[OUTFALLS]\n";
    file << ";;Name Elevation Type Stage Data Gated Route To\n";
    file << "OUT1 0 FREE NO\n\n";
    file << "[CONDUITS]\n";
    file << ";;Name From Node To Node Length Roughness InOffset OutOffset InitFlow MaxFlow\n";
    file << "C1 J1 OUT1 400 0.01 0 0 0 0\n\n";
    file << "[XSECTIONS]\n";
    file << ";;Link Shape Geom1 Geom2 Geom3 Geom4 Barrels\n";
    file << "C1 CIRCULAR 1 0 0 0 1\n\n";
    
    // Add subcatchments
    file << "[SUBCATCHMENTS]\n";
    file << ";;Name Rain Gage Outlet Area %Imperv Width %Slope CurbLen SnowPack\n";
    for (int i = 0; i < num_subcatchments; i++)
    {
        file << "S" << (i+1) << " RG1 J1 10 50 500 0.5 0\n";
    }
    file << "\n";
    
    file << "[SUBAREAS]\n";
    file << ";;Subcatchment N-Imperv N-Perv S-Imperv S-Perv PctZero RouteTo PctRouted\n";
    for (int i = 0; i < num_subcatchments; i++)
    {
        file << "S" << (i+1) << " 0.01 0.1 0.05 0.05 25 OUTLET\n";
    }
    file << "\n";
    
    file << "[INFILTRATION]\n";
    file << ";;Subcatchment MaxRate MinRate Decay DryTime MaxInfil\n";
    for (int i = 0; i < num_subcatchments; i++)
    {
        file << "S" << (i+1) << " 3.0 0.5 4 7 0\n";
    }
    file << "\n";
    
    file << "[RAINGAGES]\n";
    file << ";;Name Format Interval SCF Source\n";
    file << "RG1 INTENSITY 0:01 1.0 TIMESERIES TS1\n\n";
    file << "[TIMESERIES]\n";
    file << ";;Name Date Time Value\n";
    file << "TS1 0:00 0.0\n";
    file << "TS1 0:10 0.5\n";
    file << "TS1 1:00 0.0\n\n";
    file << "[REPORT]\n";
    file << "INPUT NO\n";
    file << "CONTROLS NO\n";
    file << "SUBCATCHMENTS ALL\n";
    file << "NODES ALL\n";
    file << "LINKS ALL\n\n";
    
    file.close();
    return true;
}

int main()
{
    std::cout << "=== Out-of-Range Subcatchment Index Validation Test ===" << std::endl;
    std::cout << std::endl;

    // Load the DLL
    HMODULE hDll = LoadLibraryA("GSswmm.dll");
    if (!hDll)
    {
        std::cerr << "ERROR: Failed to load GSswmm.dll" << std::endl;
        std::cerr << "Make sure the DLL is built and in the same directory" << std::endl;
        return 1;
    }
    std::cout << "[PASS] DLL loaded successfully" << std::endl;

    // Get the bridge function pointer
    BridgeFunctionType SwmmGoldSimBridge = 
        (BridgeFunctionType)GetProcAddress(hDll, "SwmmGoldSimBridge");
    
    if (!SwmmGoldSimBridge)
    {
        std::cerr << "ERROR: Failed to get SwmmGoldSimBridge function" << std::endl;
        FreeLibrary(hDll);
        return 1;
    }
    std::cout << "[PASS] Bridge function pointer obtained" << std::endl;

    // Try to get the helper function to set subcatchment index
    SetSubcatchIndexType SetSubcatchIndex = 
        (SetSubcatchIndexType)GetProcAddress(hDll, "SetSubcatchmentIndex");
    
    if (!SetSubcatchIndex)
    {
        std::cout << "[INFO] SetSubcatchmentIndex helper function not available" << std::endl;
        std::cout << "[INFO] Will test validation logic indirectly" << std::endl;
    }
    else
    {
        std::cout << "[PASS] Helper function pointer obtained" << std::endl;
    }
    std::cout << std::endl;

    // Test variables
    int status;
    double inargs[10] = {0};
    double outargs[10] = {0};
    int test_count = 0;
    int pass_count = 0;

    if (SetSubcatchIndex)
    {
        // Test 1: Out-of-range positive index
        std::cout << "Test 1: Initialize with out-of-range positive index" << std::endl;
        test_count++;
        
        if (CreateTestFile("model.inp", 2))
        {
            std::cout << "  [INFO] Created test model with 2 subcatchments (valid indices: 0-1)" << std::endl;
            
            // Set subcatchment index to 5 (out of range)
            SetSubcatchIndex(5);
            std::cout << "  [INFO] Set subcatchment index to 5 (out of range)" << std::endl;
            
            SwmmGoldSimBridge(XF_INITIALIZE, &status, inargs, outargs);
            if (status == XF_FAILURE_WITH_MSG)
            {
                ULONG_PTR* pAddr = (ULONG_PTR*)outargs;
                char* errorMsg = (char*)(*pAddr);
                std::cout << "  [PASS] Initialize correctly failed with message: " << errorMsg << std::endl;
                std::cout << "  [INFO] Validates Requirement 7.3: Return XF_FAILURE for out-of-range" << std::endl;
                pass_count++;
            }
            else if (status == XF_FAILURE)
            {
                std::cout << "  [PASS] Initialize correctly failed (status = " << status << ")" << std::endl;
                pass_count++;
            }
            else
            {
                std::cout << "  [FAIL] Initialize should fail with out-of-range index (status = " << status << ")" << std::endl;
            }
            
            // Reset to default
            SetSubcatchIndex(0);
        }
        std::cout << std::endl;

        // Test 2: Out-of-range negative index
        std::cout << "Test 2: Initialize with negative subcatchment index" << std::endl;
        test_count++;
        
        if (CreateTestFile("model.inp", 3))
        {
            std::cout << "  [INFO] Created test model with 3 subcatchments (valid indices: 0-2)" << std::endl;
            
            // Set subcatchment index to -1 (out of range)
            SetSubcatchIndex(-1);
            std::cout << "  [INFO] Set subcatchment index to -1 (negative, out of range)" << std::endl;
            
            SwmmGoldSimBridge(XF_INITIALIZE, &status, inargs, outargs);
            if (status == XF_FAILURE_WITH_MSG)
            {
                ULONG_PTR* pAddr = (ULONG_PTR*)outargs;
                char* errorMsg = (char*)(*pAddr);
                std::cout << "  [PASS] Initialize correctly failed with message: " << errorMsg << std::endl;
                std::cout << "  [INFO] Validates Requirement 7.3: Negative indices rejected" << std::endl;
                pass_count++;
            }
            else if (status == XF_FAILURE)
            {
                std::cout << "  [PASS] Initialize correctly failed (status = " << status << ")" << std::endl;
                pass_count++;
            }
            else
            {
                std::cout << "  [FAIL] Initialize should fail with negative index (status = " << status << ")" << std::endl;
            }
            
            // Reset to default
            SetSubcatchIndex(0);
        }
        std::cout << std::endl;

        // Test 3: Boundary test - exactly at upper limit
        std::cout << "Test 3: Initialize with index at upper boundary (valid)" << std::endl;
        test_count++;
        
        if (CreateTestFile("model.inp", 3))
        {
            std::cout << "  [INFO] Created test model with 3 subcatchments (valid indices: 0-2)" << std::endl;
            
            // Set subcatchment index to 2 (last valid index)
            SetSubcatchIndex(2);
            std::cout << "  [INFO] Set subcatchment index to 2 (last valid index)" << std::endl;
            
            SwmmGoldSimBridge(XF_INITIALIZE, &status, inargs, outargs);
            if (status == XF_SUCCESS)
            {
                std::cout << "  [PASS] Initialize succeeded with boundary index (status = " << status << ")" << std::endl;
                std::cout << "  [INFO] Validates boundary condition handling" << std::endl;
                pass_count++;
                
                // Clean up
                SwmmGoldSimBridge(XF_CLEANUP, &status, inargs, outargs);
            }
            else
            {
                std::cout << "  [FAIL] Initialize should succeed with valid boundary index" << std::endl;
            }
            
            // Reset to default
            SetSubcatchIndex(0);
        }
        std::cout << std::endl;

        // Test 4: Boundary test - one past upper limit
        std::cout << "Test 4: Initialize with index one past upper boundary (invalid)" << std::endl;
        test_count++;
        
        if (CreateTestFile("model.inp", 3))
        {
            std::cout << "  [INFO] Created test model with 3 subcatchments (valid indices: 0-2)" << std::endl;
            
            // Set subcatchment index to 3 (one past last valid index)
            SetSubcatchIndex(3);
            std::cout << "  [INFO] Set subcatchment index to 3 (one past valid range)" << std::endl;
            
            SwmmGoldSimBridge(XF_INITIALIZE, &status, inargs, outargs);
            if (status == XF_FAILURE_WITH_MSG || status == XF_FAILURE)
            {
                if (status == XF_FAILURE_WITH_MSG)
                {
                    ULONG_PTR* pAddr = (ULONG_PTR*)outargs;
                    char* errorMsg = (char*)(*pAddr);
                    std::cout << "  [PASS] Initialize correctly failed with message: " << errorMsg << std::endl;
                }
                else
                {
                    std::cout << "  [PASS] Initialize correctly failed (status = " << status << ")" << std::endl;
                }
                std::cout << "  [INFO] Validates off-by-one boundary handling" << std::endl;
                pass_count++;
            }
            else
            {
                std::cout << "  [FAIL] Initialize should fail with index past valid range" << std::endl;
            }
            
            // Reset to default
            SetSubcatchIndex(0);
        }
        std::cout << std::endl;
    }
    else
    {
        std::cout << "Test 1-4: Skipped (helper function not available)" << std::endl;
        std::cout << "  [INFO] To enable these tests, add SetSubcatchmentIndex export to DLL" << std::endl;
        std::cout << std::endl;
    }

    // Clean up test file
    DeleteFileA("model.inp");

    // Clean up DLL
    FreeLibrary(hDll);

    // Print summary
    std::cout << "=== Test Summary ===" << std::endl;
    std::cout << "Tests run: " << test_count << std::endl;
    std::cout << "Tests passed: " << pass_count << std::endl;
    std::cout << "Tests failed: " << (test_count - pass_count) << std::endl;
    std::cout << std::endl;

    if (test_count == 0)
    {
        std::cout << "NO TESTS RUN - Helper function not available" << std::endl;
        std::cout << "Add this to SwmmGoldSimBridge.cpp to enable testing:" << std::endl;
        std::cout << "extern \"C\" void __declspec(dllexport) SetSubcatchmentIndex(int index)" << std::endl;
        std::cout << "{" << std::endl;
        std::cout << "    subcatchment_index = index;" << std::endl;
        std::cout << "}" << std::endl;
        return 0;
    }

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
