#include <windows.h>
#include <iostream>

typedef void (__stdcall *BridgeFunctionType)(int, int*, double*, double*);
typedef void (__stdcall *StubInitFunc)(int);
typedef void (__stdcall *StubAddLidFunc)(int, const char*, double);
typedef int (__stdcall *GetCountFunc)(int);

#define XF_INITIALIZE 0
#define XF_CLEANUP 99

int main() {
    std::cout << "Loading DLL..." << std::endl;
    HMODULE hDll = LoadLibraryA("GSswmm.dll");
    if (!hDll) {
        std::cout << "Failed to load DLL" << std::endl;
        return 1;
    }
    
    BridgeFunctionType bridge = (BridgeFunctionType)GetProcAddress(hDll, "SwmmGoldSimBridge");
    StubInitFunc stubInit = (StubInitFunc)GetProcAddress(hDll, "SwmmLidStub_Initialize");
    StubAddLidFunc stubAddLid = (StubAddLidFunc)GetProcAddress(hDll, "SwmmLidStub_AddLidUnit");
    GetCountFunc getCount = (GetCountFunc)GetProcAddress(hDll, "swmm_getLidUCount");
    
    std::cout << "Initializing stub..." << std::endl;
    stubInit(9);
    stubAddLid(0, "InfilTrench", 100.0);
    stubAddLid(0, "RainBarrels", 50.0);
    
    std::cout << "Checking count before bridge call..." << std::endl;
    int count_before = getCount(0);
    std::cout << "Count before: " << count_before << std::endl;
    
    std::cout << "Calling bridge XF_INITIALIZE..." << std::endl;
    int status = 0;
    double inargs[10] = {0};
    double outargs[10] = {0};
    
    CopyFileA("lid_test_model.inp", "model.inp", FALSE);
    bridge(XF_INITIALIZE, &status, inargs, outargs);
    
    std::cout << "Status: " << status << std::endl;
    
    std::cout << "Checking count after bridge call..." << std::endl;
    int count_after = getCount(0);
    std::cout << "Count after: " << count_after << std::endl;
    
    bridge(XF_CLEANUP, &status, inargs, outargs);
    FreeLibrary(hDll);
    return 0;
}
