#include <windows.h>
#include <iostream>

typedef void (__stdcall *StubInitFunc)(int);
typedef void (__stdcall *StubAddLidFunc)(int, const char*, double);
typedef int (__stdcall *GetCountFunc)(int);
typedef void (__stdcall *GetNameFunc)(int, int, char*, int);

int main() {
    std::cout << "Loading DLL..." << std::endl;
    HMODULE hDll = LoadLibraryA("GSswmm.dll");
    if (!hDll) {
        std::cout << "Failed to load DLL" << std::endl;
        return 1;
    }
    
    StubInitFunc stubInit = (StubInitFunc)GetProcAddress(hDll, "SwmmLidStub_Initialize");
    StubAddLidFunc stubAddLid = (StubAddLidFunc)GetProcAddress(hDll, "SwmmLidStub_AddLidUnit");
    GetCountFunc getCount = (GetCountFunc)GetProcAddress(hDll, "swmm_getLidUCount");
    GetNameFunc getName = (GetNameFunc)GetProcAddress(hDll, "swmm_getLidUName");
    
    if (!stubInit || !stubAddLid || !getCount || !getName) {
        std::cout << "Failed to get function pointers" << std::endl;
        std::cout << "  stubInit: " << (stubInit ? "OK" : "FAIL") << std::endl;
        std::cout << "  stubAddLid: " << (stubAddLid ? "OK" : "FAIL") << std::endl;
        std::cout << "  getCount: " << (getCount ? "OK" : "FAIL") << std::endl;
        std::cout << "  getName: " << (getName ? "OK" : "FAIL") << std::endl;
        FreeLibrary(hDll);
        return 1;
    }
    
    std::cout << "Initializing stub..." << std::endl;
    stubInit(10);
    
    std::cout << "Adding LID unit..." << std::endl;
    stubAddLid(0, "TestLID", 100.0);
    
    std::cout << "Getting count..." << std::endl;
    int count = getCount(0);
    std::cout << "Count: " << count << std::endl;
    
    if (count > 0) {
        char name[64];
        getName(0, 0, name, sizeof(name));
        std::cout << "Name: " << name << std::endl;
    }
    
    FreeLibrary(hDll);
    std::cout << "Test complete!" << std::endl;
    return 0;
}
