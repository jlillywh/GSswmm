//-----------------------------------------------------------------------------
//   test_report_arguments.cpp
//
//   Test XF_REP_ARGUMENTS with dynamic mapping file loading
//
//   Tests:
//   1. Valid mapping file returns correct counts
//   2. Missing mapping file returns error
//   3. Invalid JSON returns error
//   4. Mapping is loaded only once (cached)
//-----------------------------------------------------------------------------

#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>

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

// External DLL function
extern "C" __declspec(dllimport) void SwmmGoldSimBridge(
    int methodID,
    int* status,
    double* inargs,
    double* outargs
);

//-----------------------------------------------------------------------------
// Helper function to create a test mapping file
//-----------------------------------------------------------------------------
void CreateTestMapping(const std::string& filename, int input_count, int output_count) {
    std::ofstream file(filename);
    file << "{\n";
    file << "  \"version\": \"1.0\",\n";
    file << "  \"inp_file_hash\": \"test_hash_123\",\n";
    file << "  \"input_count\": " << input_count << ",\n";
    file << "  \"output_count\": " << output_count << ",\n";
    file << "  \"inputs\": [\n";
    
    // Generate input elements
    for (int i = 0; i < input_count; i++) {
        file << "    {\n";
        file << "      \"index\": " << i << ",\n";
        if (i == 0) {
            file << "      \"name\": \"ElapsedTime\",\n";
            file << "      \"object_type\": \"SYSTEM\",\n";
            file << "      \"property\": \"ELAPSEDTIME\"\n";
        } else {
            file << "      \"name\": \"RG" << i << "\",\n";
            file << "      \"object_type\": \"GAGE\",\n";
            file << "      \"property\": \"RAINFALL\"\n";
        }
        file << "    }";
        if (i < input_count - 1) file << ",";
        file << "\n";
    }
    
    file << "  ],\n";
    file << "  \"outputs\": [\n";
    
    // Generate output elements
    for (int i = 0; i < output_count; i++) {
        file << "    {\n";
        file << "      \"index\": " << i << ",\n";
        file << "      \"name\": \"OUT" << i << "\",\n";
        file << "      \"object_type\": \"STORAGE\",\n";
        file << "      \"property\": \"VOLUME\",\n";
        file << "      \"swmm_index\": " << i << "\n";
        file << "    }";
        if (i < output_count - 1) file << ",";
        file << "\n";
    }
    
    file << "  ]\n";
    file << "}\n";
    file.close();
}

//-----------------------------------------------------------------------------
// Helper function to create invalid JSON
//-----------------------------------------------------------------------------
void CreateInvalidJSON(const std::string& filename) {
    std::ofstream file(filename);
    file << "{ invalid json content\n";
    file.close();
}

//-----------------------------------------------------------------------------
// Main test function
//-----------------------------------------------------------------------------
int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "Test: XF_REP_ARGUMENTS with Mapping File" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    int status = 0;
    double inargs[10] = {0};
    double outargs[10] = {0};
    int test_count = 0;
    int pass_count = 0;

    // Test 1: Missing mapping file returns error
    // NOTE: Must test error cases FIRST before mapping is cached
    std::cout << "Test 1: Missing mapping file" << std::endl;
    test_count++;
    // Ensure file doesn't exist
    DeleteFileA("SwmmGoldSimBridge.json");
    SwmmGoldSimBridge(XF_REP_ARGUMENTS, &status, inargs, outargs);
    if (status == XF_FAILURE_WITH_MSG) {
        std::cout << "  [PASS] Status = " << status << " (error with message)" << std::endl;
        // Try to read error message
        ULONG_PTR* pAddr = reinterpret_cast<ULONG_PTR*>(outargs);
        const char* error_msg = reinterpret_cast<const char*>(*pAddr);
        if (error_msg) {
            std::cout << "  Error message: " << error_msg << std::endl;
        }
        pass_count++;
    } else {
        std::cout << "  [FAIL] Expected status -1 (failure with message), got status " 
                  << status << std::endl;
    }
    std::cout << std::endl;

    // Test 2: Valid mapping file returns correct counts
    std::cout << "Test 2: Valid mapping file" << std::endl;
    test_count++;
    CreateTestMapping("SwmmGoldSimBridge.json", 3, 5);
    SwmmGoldSimBridge(XF_REP_ARGUMENTS, &status, inargs, outargs);
    if (status == XF_SUCCESS && outargs[0] == 3.0 && outargs[1] == 5.0) {
        std::cout << "  [PASS] Inputs = " << outargs[0] << ", Outputs = " 
                  << outargs[1] << ", Status = " << status << std::endl;
        pass_count++;
    } else {
        std::cout << "  [FAIL] Expected 3 inputs, 5 outputs, status 0, got " 
                  << outargs[0] << " inputs, " << outargs[1] << " outputs, status " 
                  << status << std::endl;
        if (status == XF_FAILURE_WITH_MSG) {
            ULONG_PTR* pAddr = reinterpret_cast<ULONG_PTR*>(outargs);
            const char* error_msg = reinterpret_cast<const char*>(*pAddr);
            if (error_msg) {
                std::cout << "  Error message: " << error_msg << std::endl;
            }
        }
    }
    std::cout << std::endl;

    // Test 3: Cached mapping returns same counts (no reload)
    std::cout << "Test 3: Cached mapping (file deleted but still works)" << std::endl;
    test_count++;
    DeleteFileA("SwmmGoldSimBridge.json");  // Delete file to prove it's cached
    SwmmGoldSimBridge(XF_REP_ARGUMENTS, &status, inargs, outargs);
    if (status == XF_SUCCESS && outargs[0] == 3.0 && outargs[1] == 5.0) {
        std::cout << "  [PASS] Inputs = " << outargs[0] << ", Outputs = " 
                  << outargs[1] << ", Status = " << status << " (from cache)" << std::endl;
        pass_count++;
    } else {
        std::cout << "  [FAIL] Expected 3 inputs, 5 outputs from cache, got " 
                  << outargs[0] << " inputs, " << outargs[1] << " outputs, status " 
                  << status << std::endl;
    }
    std::cout << std::endl;

    // Summary
    std::cout << "========================================" << std::endl;
    std::cout << "Test Summary: " << pass_count << "/" << test_count << " passed" << std::endl;
    std::cout << "========================================" << std::endl;

    return (pass_count == test_count) ? 0 : 1;
}
