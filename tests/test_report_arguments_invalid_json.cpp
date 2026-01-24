//-----------------------------------------------------------------------------
//   test_report_arguments_invalid_json.cpp
//
//   Test XF_REP_ARGUMENTS with invalid JSON
//   This must be a separate executable because mapping is cached
//-----------------------------------------------------------------------------

#include <windows.h>
#include <iostream>
#include <fstream>

// GoldSim method IDs
#define XF_REP_ARGUMENTS    3

// GoldSim status codes
#define XF_SUCCESS              0
#define XF_FAILURE_WITH_MSG    -1

// External DLL function
extern "C" __declspec(dllimport) void SwmmGoldSimBridge(
    int methodID,
    int* status,
    double* inargs,
    double* outargs
);

int main() {
    std::cout << "Test: XF_REP_ARGUMENTS with Invalid JSON" << std::endl;

    // Create invalid JSON file
    std::ofstream file("SwmmGoldSimBridge.json");
    file << "{ invalid json content\n";
    file.close();

    int status = 0;
    double inargs[10] = {0};
    double outargs[10] = {0};

    SwmmGoldSimBridge(XF_REP_ARGUMENTS, &status, inargs, outargs);
    
    if (status == XF_FAILURE_WITH_MSG) {
        std::cout << "  [PASS] Status = " << status << " (error with message)" << std::endl;
        ULONG_PTR* pAddr = reinterpret_cast<ULONG_PTR*>(outargs);
        const char* error_msg = reinterpret_cast<const char*>(*pAddr);
        if (error_msg) {
            std::cout << "  Error message: " << error_msg << std::endl;
        }
        DeleteFileA("SwmmGoldSimBridge.json");
        return 0;
    } else {
        std::cout << "  [FAIL] Expected status -1, got status " << status << std::endl;
        DeleteFileA("SwmmGoldSimBridge.json");
        return 1;
    }
}
