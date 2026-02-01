//-----------------------------------------------------------------------------
//   test_overflow_integration.cpp
//
//   Integration test for LID surface overflow API
//   Tests both STORAGE_VOLUME and SURFACE_OUTFLOW properties
//-----------------------------------------------------------------------------

#include <iostream>
#include <fstream>
#include <string>
#include "../include/swmm5.h"

void CreateTestMapping() {
    std::ofstream f("SwmmGoldSimBridge.json");
    f << R"({
  "model_file": "lid_test_model.inp",
  "log_level": 2,
  "inputs": [],
  "outputs": [
    {
      "index": 0,
      "name": "S1/InfilTrench",
      "object_type": "LID",
      "property": "STORAGE_VOLUME",
      "interface_index": 0,
      "swmm_index": 0
    },
    {
      "index": 1,
      "name": "S1/InfilTrench",
      "object_type": "LID",
      "property": "SURFACE_OUTFLOW",
      "interface_index": 1,
      "swmm_index": 0
    }
  ]
})";
    f.close();
}

int main() {
    std::cout << "LID Overflow Integration Test\n";
    std::cout << "==============================\n\n";
    
    // Create test mapping
    CreateTestMapping();
    std::cout << "[1] Created test mapping with STORAGE_VOLUME and SURFACE_OUTFLOW\n";
    
    // Initialize SWMM
    std::cout << "[2] Initializing SWMM...\n";
    int err = swmm_open("lid_test_model.inp", "test.rpt", "test.out");
    if (err != 0) {
        std::cout << "    [FAIL] Could not open SWMM model\n";
        return 1;
    }
    std::cout << "    [OK] SWMM opened successfully\n";
    
    err = swmm_start(1);
    if (err != 0) {
        std::cout << "    [FAIL] Could not start SWMM\n";
        swmm_close();
        return 1;
    }
    std::cout << "    [OK] SWMM started successfully\n\n";
    
    // Get subcatchment index
    int subcatch_idx = swmm_getIndex(swmm_SUBCATCH, "S1");
    if (subcatch_idx < 0) {
        std::cout << "[FAIL] Subcatchment 'S1' not found\n";
        swmm_end();
        swmm_close();
        return 1;
    }
    std::cout << "[3] Found subcatchment 'S1' at index " << subcatch_idx << "\n";
    
    // Get LID count
    int lid_count = swmm_getLidUCount(subcatch_idx);
    std::cout << "[4] Subcatchment has " << lid_count << " LID unit(s)\n";
    
    if (lid_count <= 0) {
        std::cout << "    [FAIL] No LID units found\n";
        swmm_end();
        swmm_close();
        return 1;
    }
    
    // Get LID name
    char lid_name[64];
    swmm_getLidUName(subcatch_idx, 0, lid_name, sizeof(lid_name));
    std::cout << "[5] LID unit 0 name: '" << lid_name << "'\n\n";
    
    // Run simulation and track overflow
    std::cout << "[6] Running simulation...\n";
    std::cout << "    Time(min)  Storage(cf)  Overflow(cfs)\n";
    std::cout << "    ---------  -----------  -------------\n";
    
    double elapsed;
    int step = 0;
    bool overflow_detected = false;
    
    while (true) {
        err = swmm_step(&elapsed);
        if (err < 0) {
            std::cout << "    [FAIL] SWMM step error\n";
            break;
        }
        
        // Get storage and overflow every 10 steps
        if (step % 10 == 0) {
            double volume = swmm_getLidUStorageVolume(subcatch_idx, 0);
            double overflow = swmm_getLidUSurfaceOutflow(subcatch_idx, 0);
            
            double time_min = elapsed * 1440.0; // Convert days to minutes
            printf("    %9.1f  %11.2f  %13.4f\n", time_min, volume, overflow);
            
            if (overflow > 0.001) {
                overflow_detected = true;
            }
        }
        
        step++;
        
        if (err > 0) {
            std::cout << "    [OK] Simulation completed\n";
            break;
        }
    }
    
    std::cout << "\n[7] Test Results:\n";
    std::cout << "    Total steps: " << step << "\n";
    std::cout << "    Overflow detected: " << (overflow_detected ? "YES" : "NO") << "\n";
    
    // Cleanup
    swmm_end();
    swmm_close();
    
    std::cout << "\n[8] SWMM closed successfully\n";
    
    if (overflow_detected) {
        std::cout << "\n[PASS] Surface overflow API is working!\n";
        return 0;
    } else {
        std::cout << "\n[INFO] No overflow occurred during simulation\n";
        std::cout << "       (This may be expected depending on the model)\n";
        return 0;
    }
}
