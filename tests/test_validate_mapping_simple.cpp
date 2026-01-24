//-----------------------------------------------------------------------------
//   test_validate_mapping_simple.cpp
//
//   Simplified test for ValidateMapping with new input types
//   Requirements: 6.1, 6.2, 6.3, 6.4, 6.5, 6.6
//-----------------------------------------------------------------------------

#include <iostream>
#include <windows.h>
#include <fstream>
#include <string>

typedef void (*BridgeFunctionType)(int, int*, double*, double*);

#define XF_INITIALIZE       0
#define XF_CLEANUP          99
#define XF_SUCCESS              0
#define XF_FAILURE_WITH_MSG    -1

void CreateFile(const std::string& filename, const std::string& content) {
    std::ofstream file(filename);
    if (file.is_open()) {
        file << content;
        file.close();
    }
}

std::string GetErrorMessage(double* outargs) {
    ULONG_PTR* pAddr = (ULONG_PTR*)outargs;
    char* errorMsg = (char*)(*pAddr);
    return (errorMsg != nullptr) ? std::string(errorMsg) : "";
}

int main()
{
    std::cout << "=== ValidateMapping Simple Test ===" << std::endl << std::endl;

    HMODULE hDll = LoadLibraryA("GSswmm.dll");
    if (!hDll) {
        std::cerr << "ERROR: Failed to load GSswmm.dll" << std::endl;
        return 1;
    }

    BridgeFunctionType SwmmGoldSimBridge = 
        (BridgeFunctionType)GetProcAddress(hDll, "SwmmGoldSimBridge");
    
    if (!SwmmGoldSimBridge) {
        std::cerr << "ERROR: Failed to get function" << std::endl;
        FreeLibrary(hDll);
        return 1;
    }

    int status;
    double inargs[10] = {0};
    double outargs[10] = {0};
    int pass_count = 0;
    int test_count = 0;

    // Create a simple valid SWMM model without DUMMY references
    std::string model = R"([TITLE]
Simple Validation Test Model

[OPTIONS]
FLOW_UNITS CFS
INFILTRATION HORTON
FLOW_ROUTING KINWAVE
START_DATE 01/01/2020
START_TIME 00:00:00
END_DATE 01/01/2020
END_TIME 02:00:00
REPORT_STEP 00:15:00
WET_STEP 00:05:00
DRY_STEP 01:00:00
ROUTING_STEP 60

[RAINGAGES]
;;Name Format Interval SCF Source
RG1 INTENSITY 0:01 1.0 TIMESERIES TS1

[SUBCATCHMENTS]
;;Name RainGage Outlet Area PercImperv Width Slope CurbLength
S1 RG1 J1 10 50 500 0.5 0

[SUBAREAS]
;;Subcatchment N-Imperv N-Perv S-Imperv S-Perv PctZero RouteTo
S1 0.01 0.1 0.05 0.05 25 OUTLET

[INFILTRATION]
;;Subcatchment MaxRate MinRate Decay DryTime MaxInfil
S1 3.0 0.5 4 7 0

[JUNCTIONS]
;;Name Elevation MaxDepth InitDepth SurDepth Aponded
J1 10 10 0 0 0
J2 5 10 0 0 0

[STORAGE]
;;Name Elevation MaxDepth InitDepth Shape Curve_Name/Params Ponded Evap
WET_WELL 0 15 5 FUNCTIONAL 1000 0 0 0 0
POND1 10 15 5 FUNCTIONAL 2000 0 0 0 0

[OUTFALLS]
;;Name Elevation Type Stage Data Gated Route To
OUT1 0 FREE NO

[CONDUITS]
;;Name From Node To Node Length Roughness InOffset OutOffset InitFlow MaxFlow
C1 J1 WET_WELL 400 0.01 0 0 0 0
C2 J2 OUT1 400 0.01 0 0 0 0

[PUMPS]
;;Name From Node To Node Pcurve Status Startup Shutoff
P1 WET_WELL J2 * ON 0 0

[ORIFICES]
;;Name From Node To Node Type Offset Qcoeff Gated CloseTime
OR1 POND1 J1 SIDE 0 0.65 NO 0

[WEIRS]
;;Name From Node To Node Type CrestHt Qcoeff Gated EndCon EndCoeff Surcharge RoadWidth RoadSurf
W1 POND1 J2 TRANSVERSE 0 3.33 NO 0 0 YES

[TIMESERIES]
;;Name Date Time Value
TS1 0:00 0.0
TS1 1:00 0.5

[XSECTIONS]
;;Link Shape Geom1 Geom2 Geom3 Geom4 Barrels
C1 CIRCULAR 1 0 0 0 1
C2 CIRCULAR 1 0 0 0 1
OR1 CIRCULAR 0.5 0 0 0
W1 RECT_OPEN 2 1 0 0

[REPORT]
INPUT NO
CONTROLS NO
NODES ALL
LINKS ALL
)";
    CreateFile("model.inp", model);

    // Test 1: PUMP object type
    std::cout << "Test 1: PUMP object type" << std::endl;
    test_count++;
    std::string mapping1 = R"({"version":"1.0","inp_file_hash":"test","input_count":2,"output_count":1,
"inputs":[{"index":0,"name":"ElapsedTime","object_type":"SYSTEM","property":"ELAPSEDTIME"},
{"index":1,"name":"P1","object_type":"PUMP","property":"SETTING"}],
"outputs":[{"index":0,"name":"OUT1","object_type":"OUTFALL","property":"FLOW","swmm_index":0}]})";
    CreateFile("SwmmGoldSimBridge.json", mapping1);
    SwmmGoldSimBridge(XF_INITIALIZE, &status, inargs, outargs);
    if (status == XF_SUCCESS) {
        std::cout << "  [PASS]" << std::endl;
        SwmmGoldSimBridge(XF_CLEANUP, &status, inargs, outargs);
        pass_count++;
    } else {
        std::cout << "  [FAIL] " << GetErrorMessage(outargs) << std::endl;
    }

    // Test 2: ORIFICE type
    std::cout << "Test 2: ORIFICE object type" << std::endl;
    test_count++;
    std::string mapping2 = R"({"version":"1.0","inp_file_hash":"test","input_count":2,"output_count":1,
"inputs":[{"index":0,"name":"ElapsedTime","object_type":"SYSTEM","property":"ELAPSEDTIME"},
{"index":1,"name":"OR1","object_type":"ORIFICE","property":"SETTING"}],
"outputs":[{"index":0,"name":"OUT1","object_type":"OUTFALL","property":"FLOW","swmm_index":0}]})";
    CreateFile("SwmmGoldSimBridge.json", mapping2);
    SwmmGoldSimBridge(XF_INITIALIZE, &status, inargs, outargs);
    if (status == XF_SUCCESS) {
        std::cout << "  [PASS]" << std::endl;
        SwmmGoldSimBridge(XF_CLEANUP, &status, inargs, outargs);
        pass_count++;
    } else {
        std::cout << "  [FAIL] " << GetErrorMessage(outargs) << std::endl;
    }

    // Test 3: WEIR type
    std::cout << "Test 3: WEIR object type" << std::endl;
    test_count++;
    std::string mapping3 = R"({"version":"1.0","inp_file_hash":"test","input_count":2,"output_count":1,
"inputs":[{"index":0,"name":"ElapsedTime","object_type":"SYSTEM","property":"ELAPSEDTIME"},
{"index":1,"name":"W1","object_type":"WEIR","property":"SETTING"}],
"outputs":[{"index":0,"name":"OUT1","object_type":"OUTFALL","property":"FLOW","swmm_index":0}]})";
    CreateFile("SwmmGoldSimBridge.json", mapping3);
    SwmmGoldSimBridge(XF_INITIALIZE, &status, inargs, outargs);
    if (status == XF_SUCCESS) {
        std::cout << "  [PASS]" << std::endl;
        SwmmGoldSimBridge(XF_CLEANUP, &status, inargs, outargs);
        pass_count++;
    } else {
        std::cout << "  [FAIL] " << GetErrorMessage(outargs) << std::endl;
    }

    // Test 4: NODE type
    std::cout << "Test 4: NODE object type" << std::endl;
    test_count++;
    std::string mapping4 = R"({"version":"1.0","inp_file_hash":"test","input_count":2,"output_count":1,
"inputs":[{"index":0,"name":"ElapsedTime","object_type":"SYSTEM","property":"ELAPSEDTIME"},
{"index":1,"name":"J1","object_type":"NODE","property":"LATFLOW"}],
"outputs":[{"index":0,"name":"OUT1","object_type":"OUTFALL","property":"FLOW","swmm_index":0}]})";
    CreateFile("SwmmGoldSimBridge.json", mapping4);
    SwmmGoldSimBridge(XF_INITIALIZE, &status, inargs, outargs);
    if (status == XF_SUCCESS) {
        std::cout << "  [PASS]" << std::endl;
        SwmmGoldSimBridge(XF_CLEANUP, &status, inargs, outargs);
        pass_count++;
    } else {
        std::cout << "  [FAIL] " << GetErrorMessage(outargs) << std::endl;
    }

    // Test 5: Missing element error
    std::cout << "Test 5: Missing element error" << std::endl;
    test_count++;
    std::string mapping5 = R"({"version":"1.0","inp_file_hash":"test","input_count":2,"output_count":1,
"inputs":[{"index":0,"name":"ElapsedTime","object_type":"SYSTEM","property":"ELAPSEDTIME"},
{"index":1,"name":"NONEXISTENT","object_type":"PUMP","property":"SETTING"}],
"outputs":[{"index":0,"name":"OUT1","object_type":"OUTFALL","property":"FLOW","swmm_index":0}]})";
    CreateFile("SwmmGoldSimBridge.json", mapping5);
    SwmmGoldSimBridge(XF_INITIALIZE, &status, inargs, outargs);
    if (status == XF_FAILURE_WITH_MSG) {
        std::string error = GetErrorMessage(outargs);
        if (error.find("not found") != std::string::npos && error.find("NONEXISTENT") != std::string::npos) {
            std::cout << "  [PASS] Error: " << error << std::endl;
            pass_count++;
        } else {
            std::cout << "  [FAIL] Wrong error: " << error << std::endl;
        }
    } else {
        std::cout << "  [FAIL] Expected error, got status " << status << std::endl;
    }

    // Test 6: Unknown object type error
    std::cout << "Test 6: Unknown object type error" << std::endl;
    test_count++;
    std::string mapping6 = R"({"version":"1.0","inp_file_hash":"test","input_count":2,"output_count":1,
"inputs":[{"index":0,"name":"ElapsedTime","object_type":"SYSTEM","property":"ELAPSEDTIME"},
{"index":1,"name":"P1","object_type":"UNKNOWN","property":"SETTING"}],
"outputs":[{"index":0,"name":"OUT1","object_type":"OUTFALL","property":"FLOW","swmm_index":0}]})";
    CreateFile("SwmmGoldSimBridge.json", mapping6);
    SwmmGoldSimBridge(XF_INITIALIZE, &status, inargs, outargs);
    if (status == XF_FAILURE_WITH_MSG) {
        std::string error = GetErrorMessage(outargs);
        if (error.find("Unknown") != std::string::npos || error.find("object type") != std::string::npos) {
            std::cout << "  [PASS] Error: " << error << std::endl;
            pass_count++;
        } else {
            std::cout << "  [FAIL] Wrong error: " << error << std::endl;
        }
    } else {
        std::cout << "  [FAIL] Expected error, got status " << status << std::endl;
    }

    // Test 7: All types together
    std::cout << "Test 7: All types together" << std::endl;
    test_count++;
    std::string mapping7 = R"({"version":"1.0","inp_file_hash":"test","input_count":5,"output_count":1,
"inputs":[{"index":0,"name":"ElapsedTime","object_type":"SYSTEM","property":"ELAPSEDTIME"},
{"index":1,"name":"P1","object_type":"PUMP","property":"SETTING"},
{"index":2,"name":"OR1","object_type":"ORIFICE","property":"SETTING"},
{"index":3,"name":"W1","object_type":"WEIR","property":"SETTING"},
{"index":4,"name":"J1","object_type":"NODE","property":"LATFLOW"}],
"outputs":[{"index":0,"name":"OUT1","object_type":"OUTFALL","property":"FLOW","swmm_index":0}]})";
    CreateFile("SwmmGoldSimBridge.json", mapping7);
    SwmmGoldSimBridge(XF_INITIALIZE, &status, inargs, outargs);
    if (status == XF_SUCCESS) {
        std::cout << "  [PASS]" << std::endl;
        SwmmGoldSimBridge(XF_CLEANUP, &status, inargs, outargs);
        pass_count++;
    } else {
        std::cout << "  [FAIL] " << GetErrorMessage(outargs) << std::endl;
    }

    // Clean up
    FreeLibrary(hDll);

    std::cout << std::endl << "=== Summary ===" << std::endl;
    std::cout << "Passed: " << pass_count << "/" << test_count << std::endl;

    return (pass_count == test_count) ? 0 : 1;
}
