//-----------------------------------------------------------------------------
//   test_validate_mapping.cpp
//
//   Test program to verify ValidateMapping handles new input types
//   Tests: PUMP, ORIFICE, WEIR, NODE object types
//   Requirements: 6.1, 6.2, 6.3, 6.4, 6.5, 6.6
//-----------------------------------------------------------------------------

#include <iostream>
#include <windows.h>
#include <fstream>
#include <string>

// Function pointer type for the bridge function
typedef void (*BridgeFunctionType)(int, int*, double*, double*);

// GoldSim Method IDs
#define XF_INITIALIZE       0
#define XF_CLEANUP          99

// GoldSim Status Codes
#define XF_SUCCESS              0
#define XF_FAILURE              1
#define XF_FAILURE_WITH_MSG    -1

// Helper function to create a mapping file
void CreateMappingFile(const std::string& filename, const std::string& content) {
    std::ofstream file(filename);
    if (file.is_open()) {
        file << content;
        file.close();
    }
}

// Helper function to get error message from outargs
std::string GetErrorMessage(double* outargs) {
    ULONG_PTR* pAddr = (ULONG_PTR*)outargs;
    char* errorMsg = (char*)(*pAddr);
    return (errorMsg != nullptr) ? std::string(errorMsg) : "";
}

int main()
{
    std::cout << "=== ValidateMapping Test - New Input Types ===" << std::endl;
    std::cout << std::endl;

    // Load the DLL
    HMODULE hDll = LoadLibraryA("GSswmm.dll");
    if (!hDll) {
        std::cerr << "ERROR: Failed to load GSswmm.dll" << std::endl;
        return 1;
    }
    std::cout << "[PASS] DLL loaded successfully" << std::endl;

    // Get the function pointer
    BridgeFunctionType SwmmGoldSimBridge = 
        (BridgeFunctionType)GetProcAddress(hDll, "SwmmGoldSimBridge");
    
    if (!SwmmGoldSimBridge) {
        std::cerr << "ERROR: Failed to get SwmmGoldSimBridge function" << std::endl;
        FreeLibrary(hDll);
        return 1;
    }
    std::cout << "[PASS] Function pointer obtained" << std::endl;
    std::cout << std::endl;

    int status;
    double inargs[10] = {0};
    double outargs[10] = {0};
    int test_count = 0;
    int pass_count = 0;

    // Test 1: Validate PUMP object type (Requirement 6.1)
    std::cout << "Test 1: Validate PUMP object type resolution" << std::endl;
    test_count++;
    
    // Create a minimal valid SWMM model with pumps
    std::string pump_model = R"([TITLE]
Test model with pumps

[OPTIONS]
FLOW_UNITS CFS
INFILTRATION HORTON
FLOW_ROUTING KINWAVE
START_DATE 01/01/2024
START_TIME 00:00:00
END_DATE 01/01/2024
END_TIME 01:00:00
REPORT_STEP 00:01:00
WET_STEP 00:01:00
DRY_STEP 01:00:00
ROUTING_STEP 60

[RAINGAGES]
RG1 INTENSITY 0:01 1.0 TIMESERIES TS1

[SUBCATCHMENTS]
S1 RG1 J1 10 50 500 0.5 0

[SUBAREAS]
S1 0.01 0.1 0.05 0.05 25 OUTLET

[INFILTRATION]
S1 3.0 0.5 4 7 0

[JUNCTIONS]
J1 0 10 0 0 0
J2 5 10 0 0 0

[OUTFALLS]
OUTLET 0 FREE NO

[PUMPS]
P1 J1 J2 * ON 0 0
P2 J2 OUTLET * ON 0 0

[TIMESERIES]
TS1 0:00 0.0
TS1 1:00 0.5

[REPORT]
INPUT NO
CONTROLS NO
)";
    
    CreateMappingFile("model.inp", pump_model);
    
    // Create mapping with PUMP inputs
    std::string pump_mapping = R"({
  "version": "1.0",
  "inp_file_hash": "test",
  "input_count": 3,
  "output_count": 1,
  "inputs": [
    {
      "index": 0,
      "name": "ElapsedTime",
      "object_type": "SYSTEM",
      "property": "ELAPSEDTIME"
    },
    {
      "index": 1,
      "name": "P1",
      "object_type": "PUMP",
      "property": "SETTING"
    },
    {
      "index": 2,
      "name": "P2",
      "object_type": "PUMP",
      "property": "SETTING"
    }
  ],
  "outputs": [
    {
      "index": 0,
      "name": "OUTLET",
      "object_type": "OUTFALL",
      "property": "FLOW",
      "swmm_index": 0
    }
  ]
})";
    
    CreateMappingFile("SwmmGoldSimBridge.json", pump_mapping);
    
    SwmmGoldSimBridge(XF_INITIALIZE, &status, inargs, outargs);
    if (status == XF_SUCCESS) {
        std::cout << "  [PASS] PUMP object type validated successfully" << std::endl;
        SwmmGoldSimBridge(XF_CLEANUP, &status, inargs, outargs);
        pass_count++;
    } else {
        std::string error = GetErrorMessage(outargs);
        std::cout << "  [FAIL] PUMP validation failed: " << error << std::endl;
    }
    std::cout << std::endl;

    // Test 2: Validate ORIFICE object type (Requirement 6.2)
    std::cout << "Test 2: Validate ORIFICE object type resolution" << std::endl;
    test_count++;
    
    // Create model with orifices
    std::string orifice_model = R"([TITLE]
Test model with orifices

[OPTIONS]
FLOW_UNITS CFS
INFILTRATION HORTON
FLOW_ROUTING KINWAVE
START_DATE 01/01/2024
START_TIME 00:00:00
END_DATE 01/01/2024
END_TIME 01:00:00
REPORT_STEP 00:01:00
WET_STEP 00:01:00
DRY_STEP 01:00:00
ROUTING_STEP 60

[RAINGAGES]
RG1 INTENSITY 0:01 1.0 TIMESERIES TS1

[SUBCATCHMENTS]
S1 RG1 J1 10 50 500 0.5 0

[SUBAREAS]
S1 0.01 0.1 0.05 0.05 25 OUTLET

[INFILTRATION]
S1 3.0 0.5 4 7 0

[JUNCTIONS]
J1 10 10 0 0 0

[STORAGE]
POND1 10 15 5 FUNCTIONAL 2000 0 0 0 0
POND2 10 15 5 FUNCTIONAL 2000 0 0 0 0

[OUTFALLS]
OUT1 0 FREE NO

[CONDUITS]
C1 J1 OUT1 100 0.01 0 0 0 0

[ORIFICES]
OR1 POND1 J1 SIDE 0 0.65 NO 0
OR2 POND2 J1 SIDE 0 0.65 NO 0

[TIMESERIES]
TS1 0:00 0.0
TS1 1:00 0.5

[XSECTIONS]
C1 CIRCULAR 1 0 0 0 1
OR1 CIRCULAR 0.5 0 0 0
OR2 CIRCULAR 0.5 0 0 0

[REPORT]
INPUT NO
CONTROLS NO
)";
    
    CreateMappingFile("model.inp", orifice_model);
    
    std::string orifice_mapping = R"({
  "version": "1.0",
  "inp_file_hash": "test",
  "input_count": 3,
  "output_count": 1,
  "inputs": [
    {
      "index": 0,
      "name": "ElapsedTime",
      "object_type": "SYSTEM",
      "property": "ELAPSEDTIME"
    },
    {
      "index": 1,
      "name": "OR1",
      "object_type": "ORIFICE",
      "property": "SETTING"
    },
    {
      "index": 2,
      "name": "OR2",
      "object_type": "ORIFICE",
      "property": "SETTING"
    }
  ],
  "outputs": [
    {
      "index": 0,
      "name": "OUT1",
      "object_type": "OUTFALL",
      "property": "FLOW",
      "swmm_index": 0
    }
  ]
})";
    
    CreateMappingFile("SwmmGoldSimBridge.json", orifice_mapping);
    
    SwmmGoldSimBridge(XF_INITIALIZE, &status, inargs, outargs);
    if (status == XF_SUCCESS) {
        std::cout << "  [PASS] ORIFICE object type validated successfully" << std::endl;
        SwmmGoldSimBridge(XF_CLEANUP, &status, inargs, outargs);
        pass_count++;
    } else {
        std::string error = GetErrorMessage(outargs);
        std::cout << "  [FAIL] ORIFICE validation failed: " << error << std::endl;
    }
    std::cout << std::endl;

    // Test 3: Validate WEIR object type (Requirement 6.3)
    std::cout << "Test 3: Validate WEIR object type resolution" << std::endl;
    test_count++;
    
    // Create model with weirs
    std::string weir_model = R"([TITLE]
Test model with weirs

[OPTIONS]
FLOW_UNITS CFS
INFILTRATION HORTON
FLOW_ROUTING KINWAVE
START_DATE 01/01/2024
START_TIME 00:00:00
END_DATE 01/01/2024
END_TIME 01:00:00
REPORT_STEP 00:01:00
WET_STEP 00:01:00
DRY_STEP 01:00:00
ROUTING_STEP 60

[RAINGAGES]
RG1 INTENSITY 0:01 1.0 TIMESERIES TS1

[SUBCATCHMENTS]
S1 RG1 J1 10 50 500 0.5 0

[SUBAREAS]
S1 0.01 0.1 0.05 0.05 25 OUTLET

[INFILTRATION]
S1 3.0 0.5 4 7 0

[JUNCTIONS]
J1 5 10 0 0 0

[STORAGE]
POND1 10 15 5 FUNCTIONAL 2000 0 0 0 0
POND2 10 15 5 FUNCTIONAL 2000 0 0 0 0

[OUTFALLS]
OUT1 0 FREE NO

[CONDUITS]
C1 J1 OUT1 100 0.01 0 0 0 0

[WEIRS]
W1 POND1 J1 TRANSVERSE 0 3.33 NO 0 0 YES
W2 POND2 J1 TRANSVERSE 0 3.33 NO 0 0 YES

[TIMESERIES]
TS1 0:00 0.0
TS1 1:00 0.5

[XSECTIONS]
C1 CIRCULAR 1 0 0 0 1
W1 RECT_OPEN 2 1 0 0
W2 RECT_OPEN 2 1 0 0

[REPORT]
INPUT NO
CONTROLS NO
)";
    
    CreateMappingFile("model.inp", weir_model);
    
    std::string weir_mapping = R"({
  "version": "1.0",
  "inp_file_hash": "test",
  "input_count": 3,
  "output_count": 1,
  "inputs": [
    {
      "index": 0,
      "name": "ElapsedTime",
      "object_type": "SYSTEM",
      "property": "ELAPSEDTIME"
    },
    {
      "index": 1,
      "name": "W1",
      "object_type": "WEIR",
      "property": "SETTING"
    },
    {
      "index": 2,
      "name": "W2",
      "object_type": "WEIR",
      "property": "SETTING"
    }
  ],
  "outputs": [
    {
      "index": 0,
      "name": "OUT1",
      "object_type": "OUTFALL",
      "property": "FLOW",
      "swmm_index": 0
    }
  ]
})";
    
    CreateMappingFile("SwmmGoldSimBridge.json", weir_mapping);
    
    SwmmGoldSimBridge(XF_INITIALIZE, &status, inargs, outargs);
    if (status == XF_SUCCESS) {
        std::cout << "  [PASS] WEIR object type validated successfully" << std::endl;
        SwmmGoldSimBridge(XF_CLEANUP, &status, inargs, outargs);
        pass_count++;
    } else {
        std::string error = GetErrorMessage(outargs);
        std::cout << "  [FAIL] WEIR validation failed: " << error << std::endl;
    }
    std::cout << std::endl;

    // Test 4: Validate NODE object type (Requirement 6.4)
    std::cout << "Test 4: Validate NODE object type resolution" << std::endl;
    test_count++;
    
    // Create model with nodes
    std::string node_model = R"([TITLE]
Test model with nodes

[OPTIONS]
FLOW_UNITS CFS
INFILTRATION HORTON
FLOW_ROUTING KINWAVE
START_DATE 01/01/2024
START_TIME 00:00:00
END_DATE 01/01/2024
END_TIME 01:00:00
REPORT_STEP 00:01:00
WET_STEP 00:01:00
DRY_STEP 01:00:00
ROUTING_STEP 60

[RAINGAGES]
RG1 INTENSITY 0:01 1.0 TIMESERIES TS1

[SUBCATCHMENTS]
S1 RG1 J1 10 50 500 0.5 0

[SUBAREAS]
S1 0.01 0.1 0.05 0.05 25 OUTLET

[INFILTRATION]
S1 3.0 0.5 4 7 0

[JUNCTIONS]
J1 10 10 0 0 0
J3 5 10 0 0 0

[OUTFALLS]
OUT1 0 FREE NO

[CONDUITS]
C1 J1 OUT1 100 0.01 0 0 0 0
C3 J3 OUT1 100 0.01 0 0 0 0

[TIMESERIES]
TS1 0:00 0.0
TS1 1:00 0.5

[XSECTIONS]
C1 CIRCULAR 1 0 0 0 1
C3 CIRCULAR 1 0 0 0 1

[REPORT]
INPUT NO
CONTROLS NO
)";
    
    CreateMappingFile("model.inp", node_model);
    
    std::string node_mapping = R"({
  "version": "1.0",
  "inp_file_hash": "test",
  "input_count": 3,
  "output_count": 1,
  "inputs": [
    {
      "index": 0,
      "name": "ElapsedTime",
      "object_type": "SYSTEM",
      "property": "ELAPSEDTIME"
    },
    {
      "index": 1,
      "name": "J1",
      "object_type": "NODE",
      "property": "LATFLOW"
    },
    {
      "index": 2,
      "name": "J3",
      "object_type": "NODE",
      "property": "LATFLOW"
    }
  ],
  "outputs": [
    {
      "index": 0,
      "name": "OUT1",
      "object_type": "OUTFALL",
      "property": "FLOW",
      "swmm_index": 0
    }
  ]
})";
    
    CreateMappingFile("SwmmGoldSimBridge.json", node_mapping);
    
    SwmmGoldSimBridge(XF_INITIALIZE, &status, inargs, outargs);
    if (status == XF_SUCCESS) {
        std::cout << "  [PASS] NODE object type validated successfully" << std::endl;
        SwmmGoldSimBridge(XF_CLEANUP, &status, inargs, outargs);
        pass_count++;
    } else {
        std::string error = GetErrorMessage(outargs);
        std::cout << "  [FAIL] NODE validation failed: " << error << std::endl;
    }
    std::cout << std::endl;

    // Test 5: Error handling for missing element (Requirement 6.5)
    std::cout << "Test 5: Error handling for missing element" << std::endl;
    test_count++;
    
    // Reuse pump model from Test 1
    CreateMappingFile("model.inp", pump_model);
    
    std::string missing_element_mapping = R"({
  "version": "1.0",
  "inp_file_hash": "test",
  "input_count": 2,
  "output_count": 1,
  "inputs": [
    {
      "index": 0,
      "name": "ElapsedTime",
      "object_type": "SYSTEM",
      "property": "ELAPSEDTIME"
    },
    {
      "index": 1,
      "name": "NONEXISTENT_PUMP",
      "object_type": "PUMP",
      "property": "SETTING"
    }
  ],
  "outputs": [
    {
      "index": 0,
      "name": "OUTLET",
      "object_type": "OUTFALL",
      "property": "FLOW",
      "swmm_index": 0
    }
  ]
})";
    
    CreateMappingFile("SwmmGoldSimBridge.json", missing_element_mapping);
    
    SwmmGoldSimBridge(XF_INITIALIZE, &status, inargs, outargs);
    if (status == XF_FAILURE_WITH_MSG) {
        std::string error = GetErrorMessage(outargs);
        if (error.find("SWMM element not found") != std::string::npos &&
            error.find("NONEXISTENT_PUMP") != std::string::npos) {
            std::cout << "  [PASS] Missing element error handled correctly" << std::endl;
            std::cout << "  [INFO] Error message: " << error << std::endl;
            pass_count++;
        } else {
            std::cout << "  [FAIL] Error message format incorrect: " << error << std::endl;
        }
    } else {
        std::cout << "  [FAIL] Expected XF_FAILURE_WITH_MSG, got status " << status << std::endl;
    }
    std::cout << std::endl;

    // Test 6: Error handling for unknown object type (Requirement 6.6)
    std::cout << "Test 6: Error handling for unknown object type" << std::endl;
    test_count++;
    
    // Reuse pump model from Test 1
    CreateMappingFile("model.inp", pump_model);
    
    std::string unknown_type_mapping = R"({
  "version": "1.0",
  "inp_file_hash": "test",
  "input_count": 2,
  "output_count": 1,
  "inputs": [
    {
      "index": 0,
      "name": "ElapsedTime",
      "object_type": "SYSTEM",
      "property": "ELAPSEDTIME"
    },
    {
      "index": 1,
      "name": "P1",
      "object_type": "UNKNOWN_TYPE",
      "property": "SETTING"
    }
  ],
  "outputs": [
    {
      "index": 0,
      "name": "OUTLET",
      "object_type": "OUTFALL",
      "property": "FLOW",
      "swmm_index": 0
    }
  ]
})";
    
    CreateMappingFile("SwmmGoldSimBridge.json", unknown_type_mapping);
    
    SwmmGoldSimBridge(XF_INITIALIZE, &status, inargs, outargs);
    if (status == XF_FAILURE_WITH_MSG) {
        std::string error = GetErrorMessage(outargs);
        if (error.find("Unknown input object type") != std::string::npos &&
            error.find("UNKNOWN_TYPE") != std::string::npos &&
            error.find("Supported types") != std::string::npos) {
            std::cout << "  [PASS] Unknown object type error handled correctly" << std::endl;
            std::cout << "  [INFO] Error message: " << error << std::endl;
            pass_count++;
        } else {
            std::cout << "  [FAIL] Error message format incorrect: " << error << std::endl;
        }
    } else {
        std::cout << "  [FAIL] Expected XF_FAILURE_WITH_MSG, got status " << status << std::endl;
    }
    std::cout << std::endl;

    // Test 7: Mixed object types (all types together)
    std::cout << "Test 7: Mixed object types validation" << std::endl;
    test_count++;
    
    // Create comprehensive model with all types - avoid cyclic loops
    std::string mixed_model = R"([TITLE]
Test model with all types

[OPTIONS]
FLOW_UNITS CFS
INFILTRATION HORTON
FLOW_ROUTING KINWAVE
START_DATE 01/01/2024
START_TIME 00:00:00
END_DATE 01/01/2024
END_TIME 01:00:00
REPORT_STEP 00:01:00
WET_STEP 00:01:00
DRY_STEP 01:00:00
ROUTING_STEP 60

[RAINGAGES]
RG1 INTENSITY 0:01 1.0 TIMESERIES TS1

[SUBCATCHMENTS]
S1 RG1 J1 10 50 500 0.5 0

[SUBAREAS]
S1 0.01 0.1 0.05 0.05 25 OUTLET

[INFILTRATION]
S1 3.0 0.5 4 7 0

[JUNCTIONS]
J1 10 10 0 0 0
J2 5 10 0 0 0

[STORAGE]
WET_WELL 0 15 5 FUNCTIONAL 1000 0 0 0 0
POND1 10 15 5 FUNCTIONAL 2000 0 0 0 0

[OUTFALLS]
OUT1 0 FREE NO

[CONDUITS]
C1 J1 WET_WELL 100 0.01 0 0 0 0
C2 J2 OUT1 100 0.01 0 0 0 0

[PUMPS]
P1 WET_WELL J2 * ON 0 0

[ORIFICES]
OR1 POND1 J1 SIDE 0 0.65 NO 0

[WEIRS]
W1 POND1 J2 TRANSVERSE 0 3.33 NO 0 0 YES

[TIMESERIES]
TS1 0:00 0.0
TS1 1:00 0.5

[XSECTIONS]
C1 CIRCULAR 1 0 0 0 1
C2 CIRCULAR 1 0 0 0 1
OR1 CIRCULAR 0.5 0 0 0
W1 RECT_OPEN 2 1 0 0

[REPORT]
INPUT NO
CONTROLS NO
)";
    
    CreateMappingFile("model.inp", mixed_model);
    
    std::string mixed_mapping = R"({
  "version": "1.0",
  "inp_file_hash": "test",
  "input_count": 6,
  "output_count": 1,
  "inputs": [
    {
      "index": 0,
      "name": "ElapsedTime",
      "object_type": "SYSTEM",
      "property": "ELAPSEDTIME"
    },
    {
      "index": 1,
      "name": "RG1",
      "object_type": "GAGE",
      "property": "RAINFALL"
    },
    {
      "index": 2,
      "name": "P1",
      "object_type": "PUMP",
      "property": "SETTING"
    },
    {
      "index": 3,
      "name": "OR1",
      "object_type": "ORIFICE",
      "property": "SETTING"
    },
    {
      "index": 4,
      "name": "W1",
      "object_type": "WEIR",
      "property": "SETTING"
    },
    {
      "index": 5,
      "name": "J1",
      "object_type": "NODE",
      "property": "LATFLOW"
    }
  ],
  "outputs": [
    {
      "index": 0,
      "name": "OUT1",
      "object_type": "OUTFALL",
      "property": "FLOW",
      "swmm_index": 0
    }
  ]
})";
    
    CreateMappingFile("SwmmGoldSimBridge.json", mixed_mapping);
    
    SwmmGoldSimBridge(XF_INITIALIZE, &status, inargs, outargs);
    if (status == XF_SUCCESS) {
        std::cout << "  [PASS] Mixed object types validated successfully" << std::endl;
        SwmmGoldSimBridge(XF_CLEANUP, &status, inargs, outargs);
        pass_count++;
    } else {
        std::string error = GetErrorMessage(outargs);
        std::cout << "  [FAIL] Mixed types validation failed: " << error << std::endl;
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

    if (pass_count == test_count) {
        std::cout << "ALL TESTS PASSED!" << std::endl;
        return 0;
    } else {
        std::cout << "SOME TESTS FAILED" << std::endl;
        return 1;
    }
}
