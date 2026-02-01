//-----------------------------------------------------------------------------
//   test_stub_verification.cpp
//
//   Verify that the LID API stub functions work correctly
//-----------------------------------------------------------------------------

#include <iostream>
#include <cstring>

// Stub functions
extern "C" {
    void SwmmLidStub_Initialize(int subcatchCount);
    void SwmmLidStub_AddLidUnit(int subcatchIndex, const char* controlName, double initialVolume);
    void SwmmLidStub_Cleanup();
    
    int swmm_getLidUCount(int subcatchIndex);
    void swmm_getLidUName(int subcatchIndex, int lidIndex, char* name, int size);
    double swmm_getLidUStorageVolume(int subcatchIndex, int lidIndex);
}

int main() {
    std::cout << "Testing LID API Stub Functions" << std::endl;
    std::cout << "===============================" << std::endl;
    
    // Initialize stub
    std::cout << "\n[1] Initializing stub with 9 subcatchments..." << std::endl;
    SwmmLidStub_Initialize(9);
    
    // Add LID units to subcatchment 0 (S1)
    std::cout << "[2] Adding LID units to subcatchment 0..." << std::endl;
    SwmmLidStub_AddLidUnit(0, "InfilTrench", 100.0);
    SwmmLidStub_AddLidUnit(0, "RainBarrels", 50.0);
    
    // Test swmm_getLidUCount
    std::cout << "\n[3] Testing swmm_getLidUCount(0)..." << std::endl;
    int count = swmm_getLidUCount(0);
    std::cout << "    Result: " << count << std::endl;
    if (count == 2) {
        std::cout << "    [PASS]" << std::endl;
    } else {
        std::cout << "    [FAIL] Expected 2, got " << count << std::endl;
        return 1;
    }
    
    // Test swmm_getLidUName for first LID
    std::cout << "\n[4] Testing swmm_getLidUName(0, 0, ...)..." << std::endl;
    char name1[64];
    swmm_getLidUName(0, 0, name1, sizeof(name1));
    std::cout << "    Result: '" << name1 << "'" << std::endl;
    if (strcmp(name1, "InfilTrench") == 0) {
        std::cout << "    [PASS]" << std::endl;
    } else {
        std::cout << "    [FAIL] Expected 'InfilTrench', got '" << name1 << "'" << std::endl;
        return 1;
    }
    
    // Test swmm_getLidUName for second LID
    std::cout << "\n[5] Testing swmm_getLidUName(0, 1, ...)..." << std::endl;
    char name2[64];
    swmm_getLidUName(0, 1, name2, sizeof(name2));
    std::cout << "    Result: '" << name2 << "'" << std::endl;
    if (strcmp(name2, "RainBarrels") == 0) {
        std::cout << "    [PASS]" << std::endl;
    } else {
        std::cout << "    [FAIL] Expected 'RainBarrels', got '" << name2 << "'" << std::endl;
        return 1;
    }
    
    // Test swmm_getLidUStorageVolume
    std::cout << "\n[6] Testing swmm_getLidUStorageVolume(0, 0)..." << std::endl;
    double vol1 = swmm_getLidUStorageVolume(0, 0);
    std::cout << "    Result: " << vol1 << std::endl;
    if (vol1 == 100.0) {
        std::cout << "    [PASS]" << std::endl;
    } else {
        std::cout << "    [FAIL] Expected 100.0, got " << vol1 << std::endl;
        return 1;
    }
    
    std::cout << "\n[7] Testing swmm_getLidUStorageVolume(0, 1)..." << std::endl;
    double vol2 = swmm_getLidUStorageVolume(0, 1);
    std::cout << "    Result: " << vol2 << std::endl;
    if (vol2 == 50.0) {
        std::cout << "    [PASS]" << std::endl;
    } else {
        std::cout << "    [FAIL] Expected 50.0, got " << vol2 << std::endl;
        return 1;
    }
    
    // Cleanup
    std::cout << "\n[8] Cleaning up..." << std::endl;
    SwmmLidStub_Cleanup();
    
    std::cout << "\n===============================" << std::endl;
    std::cout << "All stub tests PASSED!" << std::endl;
    std::cout << "===============================" << std::endl;
    
    return 0;
}
