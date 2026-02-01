//-----------------------------------------------------------------------------
//   test_lid_bridge_integration.cpp
//
//   Integration test for LID API bridge functionality
//
//   Tests:
//   1. Bridge initialization with LID composite IDs
//   2. Composite ID parsing and resolution
//   3. LID storage volume retrieval through bridge
//   4. Backward compatibility with non-LID outputs
//   5. Error handling for invalid composite IDs
//-----------------------------------------------------------------------------

#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <cmath>

// GoldSim method IDs
#define XF_INITIALIZE       0
#define XF_CALCULATE        1
#define XF_REP_VERSION      2
#define XF_REP_ARGUMENTS    3
#define XF_CLEANUP          99

// GoldSim status codes
#define XF_SUCCESS              0
#define XF_FAILURE              1
#define XF_FAILURE_WITH_MSG    -1

// Function pointer type
typedef void (*BridgeFunctionType)(int, int*, double*, double*);

// Function pointer types for stub functions (must match __stdcall convention)
typedef void (__stdcall *StubInitFunc)(int);
typedef void (__stdcall *StubAddLidFunc)(int, const char*, double);
typedef void (__stdcall *StubCleanupFunc)();

//-----------------------------------------------------------------------------
// Test helper functions
//-----------------------------------------------------------------------------

bool LoadDLL(HMODULE& hDll, BridgeFunctionType& bridgeFunc,
             StubInitFunc& stubInit, StubAddLidFunc& stubAddLid, StubCleanupFunc& stubCleanup) {
    hDll = LoadLibraryA("GSswmm.dll");
    if (!hDll) {
        std::cout << "[FAIL] Failed to load GSswmm.dll" << std::endl;
        return false;
    }
    
    bridgeFunc = (BridgeFunctionType)GetProcAddress(hDll, "SwmmGoldSimBridge");
    if (!bridgeFunc) {
        std::cout << "[FAIL] Failed to get SwmmGoldSimBridge function" << std::endl;
        FreeLibrary(hDll);
        return false;
    }
    
    stubInit = (StubInitFunc)GetProcAddress(hDll, "SwmmLidStub_Initialize");
    if (!stubInit) {
        std::cout << "[FAIL] Failed to get SwmmLidStub_Initialize function" << std::endl;
        FreeLibrary(hDll);
        return false;
    }
    
    stubAddLid = (StubAddLidFunc)GetProcAddress(hDll, "SwmmLidStub_AddLidUnit");
    if (!stubAddLid) {
        std::cout << "[FAIL] Failed to get SwmmLidStub_AddLidUnit function" << std::endl;
        FreeLibrary(hDll);
        return false;
    }
    
    stubCleanup = (StubCleanupFunc)GetProcAddress(hDll, "SwmmLidStub_Cleanup");
    if (!stubCleanup) {
        std::cout << "[FAIL] Failed to get SwmmLidStub_Cleanup function" << std::endl;
        FreeLibrary(hDll);
        return false;
    }
    
    return true;
}

void PrintError(double* outargs, int status) {
    if (status == XF_FAILURE_WITH_MSG) {
        ULONG_PTR* pAddr = reinterpret_cast<ULONG_PTR*>(outargs);
        const char* error_msg = reinterpret_cast<const char*>(*pAddr);
        if (error_msg) {
            std::cout << "  Error: " << error_msg << std::endl;
        }
    }
}

//-----------------------------------------------------------------------------
// Test 1: Bridge initialization with LID composite IDs
//-----------------------------------------------------------------------------

bool Test_BridgeInitialization() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "Test 1: Bridge Initialization with LID" << std::endl;
    std::cout << "========================================" << std::endl;
    
    HMODULE hDll;
    BridgeFunctionType SwmmGoldSimBridge;
    StubInitFunc stubInit;
    StubAddLidFunc stubAddLid;
    StubCleanupFunc stubCleanup;
    
    if (!LoadDLL(hDll, SwmmGoldSimBridge, stubInit, stubAddLid, stubCleanup)) {
        return false;
    }
    
    // Setup stub with LID units matching the mapping file
    // S1 has InfilTrench and RainBarrels
    stubInit(9);  // 9 subcatchments in LID_Model.inp
    stubAddLid(0, "InfilTrench", 100.0);  // S1 is index 0
    stubAddLid(0, "RainBarrels", 50.0);
    
    int status = 0;
    double inargs[10] = {0};
    double outargs[10] = {0};
    
    // Copy model file
    if (!CopyFileA("lid_test_model.inp", "model.inp", FALSE)) {
        std::cout << "[FAIL] Failed to copy model file" << std::endl;
        FreeLibrary(hDll);
        SwmmLidStub_Cleanup();
        return false;
    }
    
    // Call XF_REP_ARGUMENTS
    std::cout << "\n[Step 1] Calling XF_REP_ARGUMENTS..." << std::endl;
    SwmmGoldSimBridge(XF_REP_ARGUMENTS, &status, inargs, outargs);
    
    if (status != XF_SUCCESS) {
        std::cout << "[FAIL] XF_REP_ARGUMENTS failed" << std::endl;
        PrintError(outargs, status);
        stubCleanup();
        FreeLibrary(hDll);
        return false;
    }
    
    int input_count = static_cast<int>(outargs[0]);
    int output_count = static_cast<int>(outargs[1]);
    
    std::cout << "[PASS] XF_REP_ARGUMENTS: " << input_count << " inputs, " 
              << output_count << " outputs" << std::endl;
    
    if (input_count != 1 || output_count != 3) {
        std::cout << "[FAIL] Expected 1 input and 3 outputs" << std::endl;
        stubCleanup();
        FreeLibrary(hDll);
        return false;
    }
    
    // Call XF_INITIALIZE
    std::cout << "\n[Step 2] Calling XF_INITIALIZE..." << std::endl;
    SwmmGoldSimBridge(XF_INITIALIZE, &status, inargs, outargs);
    
    if (status != XF_SUCCESS) {
        std::cout << "[FAIL] XF_INITIALIZE failed" << std::endl;
        PrintError(outargs, status);
        stubCleanup();
        FreeLibrary(hDll);
        return false;
    }
    
    std::cout << "[PASS] XF_INITIALIZE succeeded" << std::endl;
    std::cout << "[INFO] Composite IDs resolved successfully:" << std::endl;
    std::cout << "  - S1/InfilTrench" << std::endl;
    std::cout << "  - S1/RainBarrels" << std::endl;
    
    // Cleanup
    SwmmGoldSimBridge(XF_CLEANUP, &status, inargs, outargs);
    stubCleanup();
    FreeLibrary(hDll);
    
    return true;
}

//-----------------------------------------------------------------------------
// Test 2: LID storage volume retrieval
//-----------------------------------------------------------------------------

bool Test_LidStorageRetrieval() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "Test 2: LID Storage Volume Retrieval" << std::endl;
    std::cout << "========================================" << std::endl;
    
    HMODULE hDll;
    BridgeFunctionType SwmmGoldSimBridge;
    StubInitFunc stubInit;
    StubAddLidFunc stubAddLid;
    StubCleanupFunc stubCleanup;
    
    if (!LoadDLL(hDll, SwmmGoldSimBridge, stubInit, stubAddLid, stubCleanup)) {
        return false;
    }
    
    // Setup stub with known storage volumes
    stubInit(9);
    stubAddLid(0, "InfilTrench", 123.45);
    stubAddLid(0, "RainBarrels", 67.89);
    
    int status = 0;
    double inargs[10] = {0};
    double outargs[10] = {0};
    
    CopyFileA("lid_test_model.inp", "model.inp", FALSE);
    
    // Initialize
    SwmmGoldSimBridge(XF_INITIALIZE, &status, inargs, outargs);
    if (status != XF_SUCCESS) {
        std::cout << "[FAIL] XF_INITIALIZE failed" << std::endl;
        PrintError(outargs, status);
        stubCleanup();
        FreeLibrary(hDll);
        return false;
    }
    
    // Call XF_CALCULATE to get outputs
    std::cout << "\n[Step 1] Calling XF_CALCULATE..." << std::endl;
    inargs[0] = 0.0;  // Elapsed time
    SwmmGoldSimBridge(XF_CALCULATE, &status, inargs, outargs);
    
    if (status != XF_SUCCESS) {
        std::cout << "[FAIL] XF_CALCULATE failed" << std::endl;
        PrintError(outargs, status);
        SwmmGoldSimBridge(XF_CLEANUP, &status, inargs, outargs);
        stubCleanup();
        FreeLibrary(hDll);
        return false;
    }
    
    std::cout << "[PASS] XF_CALCULATE succeeded" << std::endl;
    std::cout << "\n[Step 2] Verifying output values..." << std::endl;
    std::cout << "  Output[0] (S1/InfilTrench): " << outargs[0] << std::endl;
    std::cout << "  Output[1] (S1/RainBarrels): " << outargs[1] << std::endl;
    std::cout << "  Output[2] (O1 Flow): " << outargs[2] << std::endl;
    
    // Verify LID storage volumes match stub values
    bool pass = true;
    
    if (std::abs(outargs[0] - 123.45) > 0.01) {
        std::cout << "[FAIL] InfilTrench storage volume mismatch" << std::endl;
        std::cout << "  Expected: 123.45, Got: " << outargs[0] << std::endl;
        pass = false;
    } else {
        std::cout << "[PASS] InfilTrench storage volume correct" << std::endl;
    }
    
    if (std::abs(outargs[1] - 67.89) > 0.01) {
        std::cout << "[FAIL] RainBarrels storage volume mismatch" << std::endl;
        std::cout << "  Expected: 67.89, Got: " << outargs[1] << std::endl;
        pass = false;
    } else {
        std::cout << "[PASS] RainBarrels storage volume correct" << std::endl;
    }
    
    // Verify non-negative values
    if (outargs[0] < 0.0 || outargs[1] < 0.0) {
        std::cout << "[FAIL] Storage volumes must be non-negative" << std::endl;
        pass = false;
    } else {
        std::cout << "[PASS] All storage volumes are non-negative" << std::endl;
    }
    
    // Cleanup
    SwmmGoldSimBridge(XF_CLEANUP, &status, inargs, outargs);
    stubCleanup();
    FreeLibrary(hDll);
    
    return pass;
}

//-----------------------------------------------------------------------------
// Test 3: Error handling for invalid composite IDs
//-----------------------------------------------------------------------------

bool Test_InvalidCompositeID() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "Test 3: Invalid Composite ID Handling" << std::endl;
    std::cout << "========================================" << std::endl;
    
    HMODULE hDll;
    BridgeFunctionType SwmmGoldSimBridge;
    StubInitFunc stubInit;
    StubAddLidFunc stubAddLid;
    StubCleanupFunc stubCleanup;
    
    if (!LoadDLL(hDll, SwmmGoldSimBridge, stubInit, stubAddLid, stubCleanup)) {
        return false;
    }
    
    // Setup stub without the LID units referenced in mapping
    stubInit(9);
    // Intentionally NOT adding InfilTrench or RainBarrels
    
    int status = 0;
    double inargs[10] = {0};
    double outargs[10] = {0};
    
    CopyFileA("lid_test_model.inp", "model.inp", FALSE);
    
    // Call XF_INITIALIZE - should fail because LID units don't exist
    std::cout << "\n[Step 1] Calling XF_INITIALIZE with missing LID units..." << std::endl;
    SwmmGoldSimBridge(XF_INITIALIZE, &status, inargs, outargs);
    
    if (status == XF_SUCCESS) {
        std::cout << "[FAIL] XF_INITIALIZE should have failed with missing LID units" << std::endl;
        SwmmGoldSimBridge(XF_CLEANUP, &status, inargs, outargs);
        stubCleanup();
        FreeLibrary(hDll);
        return false;
    }
    
    std::cout << "[PASS] XF_INITIALIZE correctly failed" << std::endl;
    PrintError(outargs, status);
    
    // Verify error message mentions LID
    if (status == XF_FAILURE_WITH_MSG) {
        ULONG_PTR* pAddr = reinterpret_cast<ULONG_PTR*>(outargs);
        const char* error_msg = reinterpret_cast<const char*>(*pAddr);
        if (error_msg && strstr(error_msg, "LID")) {
            std::cout << "[PASS] Error message mentions LID" << std::endl;
        } else {
            std::cout << "[WARN] Error message doesn't mention LID" << std::endl;
        }
    }
    
    stubCleanup();
    FreeLibrary(hDll);
    
    return true;
}

//-----------------------------------------------------------------------------
// Test 4: Backward compatibility with non-LID outputs
//-----------------------------------------------------------------------------

bool Test_BackwardCompatibility() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "Test 4: Backward Compatibility" << std::endl;
    std::cout << "========================================" << std::endl;
    
    // Create a mapping file with only non-LID outputs
    std::ofstream mapping("SwmmGoldSimBridge.json");
    mapping << R"({
  "version": "1.0",
  "logging_level": "INFO",
  "input_count": 1,
  "output_count": 1,
  "inputs": [
    {
      "index": 0,
      "name": "ElapsedTime",
      "object_type": "SYSTEM",
      "property": "ELAPSEDTIME"
    }
  ],
  "outputs": [
    {
      "index": 0,
      "name": "O1",
      "object_type": "OUTFALL",
      "property": "FLOW",
      "swmm_index": 0
    }
  ]
})";
    mapping.close();
    
    HMODULE hDll;
    BridgeFunctionType SwmmGoldSimBridge;
    StubInitFunc stubInit;
    StubAddLidFunc stubAddLid;
    StubCleanupFunc stubCleanup;
    
    if (!LoadDLL(hDll, SwmmGoldSimBridge, stubInit, stubAddLid, stubCleanup)) {
        return false;
    }
    
    stubInit(9);
    
    int status = 0;
    double inargs[10] = {0};
    double outargs[10] = {0};
    
    CopyFileA("lid_test_model.inp", "model.inp", FALSE);
    
    // Initialize with non-LID mapping
    std::cout << "\n[Step 1] Initializing with non-LID mapping..." << std::endl;
    SwmmGoldSimBridge(XF_INITIALIZE, &status, inargs, outargs);
    
    if (status != XF_SUCCESS) {
        std::cout << "[FAIL] XF_INITIALIZE failed with non-LID mapping" << std::endl;
        PrintError(outargs, status);
        stubCleanup();
        FreeLibrary(hDll);
        return false;
    }
    
    std::cout << "[PASS] XF_INITIALIZE succeeded with non-LID mapping" << std::endl;
    
    // Call XF_CALCULATE
    std::cout << "\n[Step 2] Calling XF_CALCULATE..." << std::endl;
    SwmmGoldSimBridge(XF_CALCULATE, &status, inargs, outargs);
    
    if (status != XF_SUCCESS) {
        std::cout << "[FAIL] XF_CALCULATE failed" << std::endl;
        PrintError(outargs, status);
        SwmmGoldSimBridge(XF_CLEANUP, &status, inargs, outargs);
        stubCleanup();
        FreeLibrary(hDll);
        return false;
    }
    
    std::cout << "[PASS] XF_CALCULATE succeeded" << std::endl;
    std::cout << "  Output[0] (O1 Flow): " << outargs[0] << std::endl;
    std::cout << "[PASS] Backward compatibility maintained" << std::endl;
    
    // Cleanup
    SwmmGoldSimBridge(XF_CLEANUP, &status, inargs, outargs);
    stubCleanup();
    FreeLibrary(hDll);
    
    // Restore original mapping file
    std::ofstream restore("SwmmGoldSimBridge.json");
    restore << R"({
  "version": "1.0",
  "logging_level": "DEBUG",
  "input_count": 1,
  "output_count": 3,
  "inputs": [
    {
      "index": 0,
      "name": "ElapsedTime",
      "object_type": "SYSTEM",
      "property": "ELAPSEDTIME"
    }
  ],
  "outputs": [
    {
      "index": 0,
      "name": "S1/InfilTrench",
      "object_type": "LID",
      "property": "STORAGE_VOLUME",
      "swmm_index": 0
    },
    {
      "index": 1,
      "name": "S1/RainBarrels",
      "object_type": "LID",
      "property": "STORAGE_VOLUME",
      "swmm_index": 0
    },
    {
      "index": 2,
      "name": "O1",
      "object_type": "OUTFALL",
      "property": "FLOW",
      "swmm_index": 0
    }
  ]
})";
    restore.close();
    
    return true;
}

//-----------------------------------------------------------------------------
// Main test runner
//-----------------------------------------------------------------------------

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "LID Bridge Integration Test Suite" << std::endl;
    std::cout << "========================================" << std::endl;
    
    int total_tests = 0;
    int passed_tests = 0;
    
    // Test 1: Bridge initialization
    total_tests++;
    if (Test_BridgeInitialization()) {
        passed_tests++;
        std::cout << "\n[RESULT] Test 1: PASSED" << std::endl;
    } else {
        std::cout << "\n[RESULT] Test 1: FAILED" << std::endl;
    }
    
    // Test 2: Storage volume retrieval
    total_tests++;
    if (Test_LidStorageRetrieval()) {
        passed_tests++;
        std::cout << "\n[RESULT] Test 2: PASSED" << std::endl;
    } else {
        std::cout << "\n[RESULT] Test 2: FAILED" << std::endl;
    }
    
    // Test 3: Invalid composite ID
    total_tests++;
    if (Test_InvalidCompositeID()) {
        passed_tests++;
        std::cout << "\n[RESULT] Test 3: PASSED" << std::endl;
    } else {
        std::cout << "\n[RESULT] Test 3: FAILED" << std::endl;
    }
    
    // Test 4: Backward compatibility
    total_tests++;
    if (Test_BackwardCompatibility()) {
        passed_tests++;
        std::cout << "\n[RESULT] Test 4: PASSED" << std::endl;
    } else {
        std::cout << "\n[RESULT] Test 4: FAILED" << std::endl;
    }
    
    // Summary
    std::cout << "\n========================================" << std::endl;
    std::cout << "Test Summary: " << passed_tests << "/" << total_tests << " passed" << std::endl;
    std::cout << "========================================" << std::endl;
    
    return (passed_tests == total_tests) ? 0 : 1;
}
