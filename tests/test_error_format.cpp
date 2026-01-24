//-----------------------------------------------------------------------------
//   test_error_format.cpp
//
//   Simple test to verify error message formatting
//   Uses test_model.inp which is known to be valid
//-----------------------------------------------------------------------------

#include <iostream>
#include <windows.h>
#include <fstream>
#include <string>

typedef void (*BridgeFunctionType)(int, int*, double*, double*);

#define XF_INITIALIZE       0
#define XF_CLEANUP          99
#define XF_SUCCESS              0
#define XF_FAILURE              1
#define XF_FAILURE_WITH_MSG    -1

void CreateMappingFile(const std::string& filename, const std::string& content) {
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
    std::cout << "=== Error Message Format Test ===" << std::endl << std::endl;

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

    // Test 1: Unknown object type error
    std::cout << "Test 1: Unknown object type error format" << std::endl;
    test_count++;
    
    // Use minimal valid model
    CopyFileA("minimal_valid.inp", "model.inp", FALSE);
    
    std::string unknown_type_mapping = R"({
  "version": "1.0",
  "inp_file_hash": "test",
  "input_count": 2,
  "output_count": 0,
  "inputs": [
    {
      "index": 0,
      "name": "ElapsedTime",
      "object_type": "SYSTEM",
      "property": "ELAPSEDTIME"
    },
    {
      "index": 1,
      "name": "TestElement",
      "object_type": "INVALID_TYPE",
      "property": "SETTING"
    }
  ],
  "outputs": []
})";
    
    CreateMappingFile("SwmmGoldSimBridge.json", unknown_type_mapping);
    
    SwmmGoldSimBridge(XF_INITIALIZE, &status, inargs, outargs);
    if (status == XF_FAILURE_WITH_MSG) {
        std::string error = GetErrorMessage(outargs);
        std::cout << "  Error message:" << std::endl;
        std::cout << "  " << error << std::endl << std::endl;
        
        if (error.find("Error:") != std::string::npos &&
            error.find("Context:") != std::string::npos &&
            error.find("Suggestion:") != std::string::npos) {
            std::cout << "  [PASS] Has required format sections" << std::endl;
            pass_count++;
        } else {
            std::cout << "  [FAIL] Missing required format sections" << std::endl;
        }
    } else {
        std::cout << "  [FAIL] Wrong status: " << status << std::endl;
    }
    std::cout << std::endl;

    // Test 2: Missing file error
    std::cout << "Test 2: Missing file error format" << std::endl;
    test_count++;
    
    DeleteFileA("model.inp");
    
    std::string simple_mapping = R"({
  "version": "1.0",
  "inp_file_hash": "test",
  "input_count": 1,
  "output_count": 0,
  "inputs": [
    {
      "index": 0,
      "name": "ElapsedTime",
      "object_type": "SYSTEM",
      "property": "ELAPSEDTIME"
    }
  ],
  "outputs": []
})";
    
    CreateMappingFile("SwmmGoldSimBridge.json", simple_mapping);
    
    SwmmGoldSimBridge(XF_INITIALIZE, &status, inargs, outargs);
    if (status == XF_FAILURE_WITH_MSG) {
        std::string error = GetErrorMessage(outargs);
        std::cout << "  Error message:" << std::endl;
        std::cout << "  " << error << std::endl << std::endl;
        
        if (error.find("Error:") != std::string::npos &&
            error.find("Context:") != std::string::npos &&
            error.find("Suggestion:") != std::string::npos) {
            std::cout << "  [PASS] Has required format sections" << std::endl;
            pass_count++;
        } else {
            std::cout << "  [FAIL] Missing required format sections" << std::endl;
        }
    } else {
        std::cout << "  [FAIL] Wrong status: " << status << std::endl;
    }
    std::cout << std::endl;

    FreeLibrary(hDll);

    std::cout << "=== Summary ===" << std::endl;
    std::cout << "Passed: " << pass_count << "/" << test_count << std::endl;

    return (pass_count == test_count) ? 0 : 1;
}
