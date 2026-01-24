//-----------------------------------------------------------------------------
//   test_error_messages.cpp
//
//   Test program to verify error message formatting
//   Tests: Requirements 8.1, 8.2, 8.3
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
    std::cout << "=== Error Message Format Test ===" << std::endl;
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

    // Test 1: Missing element error format (Requirement 8.1)
    std::cout << "Test 1: Missing element error format" << std::endl;
    test_count++;
    
    // Create a valid SWMM model
    CopyFileA("test_model_pumps.inp", "model.inp", FALSE);
    
    // Create mapping with non-existent element
    std::string missing_mapping = R"({
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
      "name": "NONEXISTENT",
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
    
    CreateMappingFile("SwmmGoldSimBridge.json", missing_mapping);
    
    SwmmGoldSimBridge(XF_INITIALIZE, &status, inargs, outargs);
    if (status == XF_FAILURE_WITH_MSG) {
        std::string error = GetErrorMessage(outargs);
        bool has_error = error.find("Error:") != std::string::npos;
        bool has_context = error.find("Context:") != std::string::npos;
        bool has_suggestion = error.find("Suggestion:") != std::string::npos;
        
        if (has_error && has_context && has_suggestion) {
            std::cout << "  [PASS] Error message has correct format" << std::endl;
            std::cout << "  [INFO] Message: " << error << std::endl;
            pass_count++;
        } else {
            std::cout << "  [FAIL] Error message missing required sections" << std::endl;
            std::cout << "  [INFO] Has 'Error:': " << has_error << std::endl;
            std::cout << "  [INFO] Has 'Context:': " << has_context << std::endl;
            std::cout << "  [INFO] Has 'Suggestion:': " << has_suggestion << std::endl;
            std::cout << "  [INFO] Message: " << error << std::endl;
        }
    } else {
        std::cout << "  [FAIL] Expected XF_FAILURE_WITH_MSG, got status " << status << std::endl;
    }
    std::cout << std::endl;

    // Test 2: Unknown object type error format (Requirement 8.2)
    std::cout << "Test 2: Unknown object type error format" << std::endl;
    test_count++;
    
    CopyFileA("test_model_pumps.inp", "model.inp", FALSE);
    
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
      "object_type": "INVALID_TYPE",
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
        bool has_error = error.find("Error:") != std::string::npos;
        bool has_context = error.find("Context:") != std::string::npos;
        bool has_suggestion = error.find("Suggestion:") != std::string::npos;
        bool mentions_supported = error.find("Supported types") != std::string::npos;
        
        if (has_error && has_context && has_suggestion && mentions_supported) {
            std::cout << "  [PASS] Error message has correct format with supported types" << std::endl;
            std::cout << "  [INFO] Message: " << error << std::endl;
            pass_count++;
        } else {
            std::cout << "  [FAIL] Error message missing required sections" << std::endl;
            std::cout << "  [INFO] Message: " << error << std::endl;
        }
    } else {
        std::cout << "  [FAIL] Expected XF_FAILURE_WITH_MSG, got status " << status << std::endl;
    }
    std::cout << std::endl;

    // Test 3: Invalid property error format (Requirement 8.3)
    std::cout << "Test 3: Invalid property error format" << std::endl;
    test_count++;
    
    CopyFileA("test_model_pumps.inp", "model.inp", FALSE);
    
    std::string invalid_property_mapping = R"({
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
      "object_type": "PUMP",
      "property": "INVALID_PROPERTY"
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
    
    CreateMappingFile("SwmmGoldSimBridge.json", invalid_property_mapping);
    
    // Initialize first
    SwmmGoldSimBridge(XF_INITIALIZE, &status, inargs, outargs);
    if (status == XF_SUCCESS) {
        // Now try Calculate which will check properties
        inargs[0] = 0.0; // elapsed time
        inargs[1] = 1.0; // pump setting
        
        SwmmGoldSimBridge(1, &status, inargs, outargs); // Method 1 = Calculate
        
        if (status == XF_FAILURE_WITH_MSG) {
            std::string error = GetErrorMessage(outargs);
            bool has_error = error.find("Error:") != std::string::npos;
            bool has_context = error.find("Context:") != std::string::npos;
            bool has_suggestion = error.find("Suggestion:") != std::string::npos;
            
            if (has_error && has_context && has_suggestion) {
                std::cout << "  [PASS] Error message has correct format" << std::endl;
                std::cout << "  [INFO] Message: " << error << std::endl;
                pass_count++;
            } else {
                std::cout << "  [FAIL] Error message missing required sections" << std::endl;
                std::cout << "  [INFO] Message: " << error << std::endl;
            }
        } else {
            std::cout << "  [FAIL] Expected XF_FAILURE_WITH_MSG, got status " << status << std::endl;
        }
        
        SwmmGoldSimBridge(XF_CLEANUP, &status, inargs, outargs);
    } else {
        std::cout << "  [SKIP] Could not initialize for property test" << std::endl;
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
