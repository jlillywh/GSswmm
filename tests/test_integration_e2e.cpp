//-----------------------------------------------------------------------------
//   test_integration_e2e.cpp
//
//   End-to-end integration test for the scripted interface mapping feature
//
//   Tests the complete workflow:
//   1. Generate mapping from test_model.inp using Python script
//   2. Load DLL and call XF_REP_ARGUMENTS
//   3. Verify counts match parser output
//   4. Call XF_INITIALIZE and verify success
//   5. Call XF_CALCULATE and verify outputs
//-----------------------------------------------------------------------------

#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>

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

//-----------------------------------------------------------------------------
// Helper: Run Python script to generate mapping
//-----------------------------------------------------------------------------
bool GenerateMapping(const std::string& inp_file) {
    std::string command = "python ..\\generate_mapping.py " + inp_file;
    std::cout << "[INFO] Running: " << command << std::endl;
    
    int result = system(command.c_str());
    
    if (result == 0) {
        std::cout << "[PASS] Mapping generation succeeded" << std::endl;
        return true;
    } else {
        std::cout << "[FAIL] Mapping generation failed with exit code " << result << std::endl;
        return false;
    }
}

//-----------------------------------------------------------------------------
// Helper: Read expected counts from generated JSON
//-----------------------------------------------------------------------------
bool ReadExpectedCounts(int& input_count, int& output_count) {
    std::ifstream file("SwmmGoldSimBridge.json");
    if (!file.is_open()) {
        std::cout << "[FAIL] Could not open SwmmGoldSimBridge.json" << std::endl;
        return false;
    }
    
    std::string line;
    bool found_input = false;
    bool found_output = false;
    
    while (std::getline(file, line)) {
        // Simple parsing - look for "input_count": N
        size_t pos = line.find("\"input_count\"");
        if (pos != std::string::npos) {
            size_t colon = line.find(":", pos);
            if (colon != std::string::npos) {
                size_t comma = line.find(",", colon);
                std::string value_str = line.substr(colon + 1, comma - colon - 1);
                input_count = std::stoi(value_str);
                found_input = true;
            }
        }
        
        // Look for "output_count": N
        pos = line.find("\"output_count\"");
        if (pos != std::string::npos) {
            size_t colon = line.find(":", pos);
            if (colon != std::string::npos) {
                size_t comma = line.find(",", colon);
                std::string value_str = line.substr(colon + 1, comma - colon - 1);
                output_count = std::stoi(value_str);
                found_output = true;
            }
        }
    }
    
    file.close();
    
    if (found_input && found_output) {
        std::cout << "[INFO] Expected counts from JSON: inputs=" << input_count 
                  << ", outputs=" << output_count << std::endl;
        return true;
    }
    
    std::cout << "[FAIL] Could not parse counts from JSON" << std::endl;
    return false;
}

//-----------------------------------------------------------------------------
// Main test function
//-----------------------------------------------------------------------------
int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "End-to-End Integration Test" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    int test_count = 0;
    int pass_count = 0;

    // Step 1: Generate mapping from test_model.inp
    std::cout << "Step 1: Generate mapping from test_model.inp" << std::endl;
    test_count++;
    
    if (!GenerateMapping("test_model.inp")) {
        std::cout << "[FAIL] Cannot proceed without mapping file" << std::endl;
        return 1;
    }
    pass_count++;
    std::cout << std::endl;

    // Step 2: Read expected counts from generated JSON
    std::cout << "Step 2: Read expected counts from JSON" << std::endl;
    test_count++;
    
    int expected_inputs = 0;
    int expected_outputs = 0;
    
    if (!ReadExpectedCounts(expected_inputs, expected_outputs)) {
        std::cout << "[FAIL] Cannot proceed without expected counts" << std::endl;
        return 1;
    }
    pass_count++;
    std::cout << std::endl;

    // Step 3: Load DLL
    std::cout << "Step 3: Load DLL" << std::endl;
    test_count++;
    
    HMODULE hDll = LoadLibraryA("GSswmm.dll");
    if (!hDll) {
        std::cout << "[FAIL] Failed to load GSswmm.dll" << std::endl;
        return 1;
    }
    
    BridgeFunctionType SwmmGoldSimBridge = 
        (BridgeFunctionType)GetProcAddress(hDll, "SwmmGoldSimBridge");
    
    if (!SwmmGoldSimBridge) {
        std::cout << "[FAIL] Failed to get SwmmGoldSimBridge function" << std::endl;
        FreeLibrary(hDll);
        return 1;
    }
    
    std::cout << "[PASS] DLL loaded successfully" << std::endl;
    pass_count++;
    std::cout << std::endl;

    // Test variables
    int status = 0;
    double inargs[10] = {0};
    double outargs[10] = {0};

    // Step 4: Call XF_REP_ARGUMENTS and verify counts
    std::cout << "Step 4: Call XF_REP_ARGUMENTS" << std::endl;
    test_count++;
    
    SwmmGoldSimBridge(XF_REP_ARGUMENTS, &status, inargs, outargs);
    
    if (status == XF_SUCCESS) {
        int dll_inputs = static_cast<int>(outargs[0]);
        int dll_outputs = static_cast<int>(outargs[1]);
        
        std::cout << "[INFO] DLL reports: inputs=" << dll_inputs 
                  << ", outputs=" << dll_outputs << std::endl;
        
        if (dll_inputs == expected_inputs && dll_outputs == expected_outputs) {
            std::cout << "[PASS] Counts match parser output" << std::endl;
            pass_count++;
        } else {
            std::cout << "[FAIL] Count mismatch! Expected inputs=" << expected_inputs 
                      << ", outputs=" << expected_outputs << std::endl;
        }
    } else {
        std::cout << "[FAIL] XF_REP_ARGUMENTS failed with status " << status << std::endl;
        
        if (status == XF_FAILURE_WITH_MSG) {
            ULONG_PTR* pAddr = reinterpret_cast<ULONG_PTR*>(outargs);
            const char* error_msg = reinterpret_cast<const char*>(*pAddr);
            if (error_msg) {
                std::cout << "  Error: " << error_msg << std::endl;
            }
        }
    }
    std::cout << std::endl;

    // Step 5: Copy test model to expected location
    std::cout << "Step 5: Prepare SWMM model file" << std::endl;
    test_count++;
    
    if (CopyFileA("test_model.inp", "model.inp", FALSE)) {
        std::cout << "[PASS] Copied test_model.inp to model.inp" << std::endl;
        pass_count++;
    } else {
        std::cout << "[FAIL] Failed to copy model file" << std::endl;
    }
    std::cout << std::endl;

    // Step 6: Call XF_INITIALIZE
    std::cout << "Step 6: Call XF_INITIALIZE" << std::endl;
    test_count++;
    
    SwmmGoldSimBridge(XF_INITIALIZE, &status, inargs, outargs);
    
    if (status == XF_SUCCESS) {
        std::cout << "[PASS] XF_INITIALIZE succeeded" << std::endl;
        pass_count++;
    } else {
        std::cout << "[FAIL] XF_INITIALIZE failed with status " << status << std::endl;
        
        if (status == XF_FAILURE_WITH_MSG) {
            ULONG_PTR* pAddr = reinterpret_cast<ULONG_PTR*>(outargs);
            const char* error_msg = reinterpret_cast<const char*>(*pAddr);
            if (error_msg) {
                std::cout << "  Error: " << error_msg << std::endl;
            }
        }
    }
    std::cout << std::endl;

    // Step 7: Call XF_CALCULATE and verify outputs
    std::cout << "Step 7: Call XF_CALCULATE" << std::endl;
    test_count++;
    
    // Set elapsed time input
    inargs[0] = 0.0;
    
    SwmmGoldSimBridge(XF_CALCULATE, &status, inargs, outargs);
    
    if (status == XF_SUCCESS) {
        std::cout << "[PASS] XF_CALCULATE succeeded" << std::endl;
        std::cout << "[INFO] Output values:" << std::endl;
        
        for (int i = 0; i < expected_outputs; i++) {
            std::cout << "  outargs[" << i << "] = " << outargs[i] << std::endl;
        }
        
        // Verify outputs are reasonable (non-NaN, finite)
        bool outputs_valid = true;
        for (int i = 0; i < expected_outputs; i++) {
            if (std::isnan(outargs[i]) || std::isinf(outargs[i])) {
                std::cout << "[WARN] Output " << i << " is NaN or infinite" << std::endl;
                outputs_valid = false;
            }
        }
        
        if (outputs_valid) {
            std::cout << "[PASS] All outputs are valid numbers" << std::endl;
            pass_count++;
        } else {
            std::cout << "[FAIL] Some outputs are invalid" << std::endl;
        }
    } else {
        std::cout << "[FAIL] XF_CALCULATE failed with status " << status << std::endl;
        
        if (status == XF_FAILURE_WITH_MSG) {
            ULONG_PTR* pAddr = reinterpret_cast<ULONG_PTR*>(outargs);
            const char* error_msg = reinterpret_cast<const char*>(*pAddr);
            if (error_msg) {
                std::cout << "  Error: " << error_msg << std::endl;
            }
        }
    }
    std::cout << std::endl;

    // Step 8: Cleanup
    std::cout << "Step 8: Call XF_CLEANUP" << std::endl;
    test_count++;
    
    SwmmGoldSimBridge(XF_CLEANUP, &status, inargs, outargs);
    
    if (status == XF_SUCCESS) {
        std::cout << "[PASS] XF_CLEANUP succeeded" << std::endl;
        pass_count++;
    } else {
        std::cout << "[FAIL] XF_CLEANUP failed with status " << status << std::endl;
    }
    std::cout << std::endl;

    // Cleanup
    FreeLibrary(hDll);

    // Summary
    std::cout << "========================================" << std::endl;
    std::cout << "Test Summary: " << pass_count << "/" << test_count << " passed" << std::endl;
    std::cout << "========================================" << std::endl;

    return (pass_count == test_count) ? 0 : 1;
}
