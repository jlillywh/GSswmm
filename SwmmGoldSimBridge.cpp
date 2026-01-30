//-----------------------------------------------------------------------------
//   SwmmGoldSimBridge.cpp
//   GoldSim-SWMM Bridge DLL v5.0 (config-driven)
//-----------------------------------------------------------------------------

#include <windows.h>
#include <string>
#include <vector>
#include "include/swmm5.h"
#include "include/MappingLoader.h"

#define DLL_VERSION 5.202
#define CONFIG_FILE "SwmmGoldSimBridge.json"
#define PROPERTY_SKIP -1

// Logging: 0=OFF, 1=ERROR, 2=INFO, 3=DEBUG
static int s_log_level = 2;  // Default to INFO, can be overridden by JSON

static void Log(int level, const char* fmt, ...) {
    if (level > s_log_level) return;
    static bool first = true;
    FILE* f = NULL;
    if (fopen_s(&f, "bridge_debug.log", first ? "w" : "a") == 0 && f) {
        if (first) { fprintf(f, "GSswmm Bridge v5.202\n"); first = false; }
        SYSTEMTIME st; GetLocalTime(&st);
        const char* tag = (level == 1) ? "ERROR" : (level == 2) ? "INFO " : "DEBUG";
        fprintf(f, "[%02d:%02d:%02d] [%s] ", st.wHour, st.wMinute, st.wSecond, tag);
        va_list ap; va_start(ap, fmt); vfprintf(f, fmt, ap); va_end(ap);
        fprintf(f, "\n"); fclose(f);
    }
}

// GoldSim API
#define XF_INITIALIZE   0
#define XF_CALCULATE    1
#define XF_REP_VERSION  2
#define XF_REP_ARGUMENTS 3
#define XF_CLEANUP      99
#define XF_SUCCESS      0
#define XF_FAILURE      1
#define XF_FAILURE_WITH_MSG -1

struct Resolved { int iface_idx; int prop_enum; int swmm_idx; };

// State
static MappingLoader s_mapping;
static bool s_mapping_loaded = false;
static std::vector<Resolved> s_inputs, s_outputs;
static bool s_swmm_running = false;
static char s_error_buf[256];
static bool s_first_calculate = true;
static std::vector<double> s_pending_inputs;

static void SetError(double* outargs, int* status, const char* msg) {
    strncpy_s(s_error_buf, sizeof(s_error_buf), msg, _TRUNCATE);
    *(ULONG_PTR*)outargs = (ULONG_PTR)s_error_buf;
    *status = XF_FAILURE_WITH_MSG;
}

static void HandleSwmmError(double* outargs, int* status) {
    swmm_getError(s_error_buf, sizeof(s_error_buf));
    *(ULONG_PTR*)outargs = (ULONG_PTR)s_error_buf;
    *status = XF_FAILURE_WITH_MSG;
}

static int ObjTypeToSwmm(const std::string& ot) {
    if (ot == "SYSTEM") return swmm_SYSTEM;
    if (ot == "GAGE") return swmm_GAGE;
    if (ot == "SUBCATCH") return swmm_SUBCATCH;
    if (ot == "NODE" || ot == "STORAGE" || ot == "OUTFALL" || ot == "JUNCTION" || ot == "DIVIDER") return swmm_NODE;
    if (ot == "LINK" || ot == "PUMP" || ot == "ORIFICE" || ot == "WEIR" || ot == "CONDUIT" || ot == "OUTLET") return swmm_LINK;
    return -1;
}

static int InputPropToEnum(const std::string& ot, const std::string& prop) {
    if (ot == "SYSTEM" && prop == "ELAPSEDTIME") return PROPERTY_SKIP;
    if (ot == "GAGE" && prop == "RAINFALL") return swmm_GAGE_RAINFALL;
    if ((ot == "PUMP" || ot == "ORIFICE" || ot == "WEIR" || ot == "LINK") && prop == "SETTING") return swmm_LINK_SETTING;
    if (ot == "NODE" && prop == "LATFLOW") return swmm_NODE_LATFLOW;
    return -1;
}

static int OutputPropToEnum(const std::string& ot, const std::string& prop) {
    if (prop == "VOLUME" && (ot == "STORAGE" || ot == "NODE")) return swmm_NODE_VOLUME;
    if (prop == "DEPTH" && (ot == "STORAGE" || ot == "NODE" || ot == "JUNCTION" || ot == "OUTFALL")) return swmm_NODE_DEPTH;
    if (prop == "FLOW") {
        if (ot == "LINK" || ot == "PUMP" || ot == "ORIFICE" || ot == "WEIR" || ot == "CONDUIT" || ot == "OUTLET") return swmm_LINK_FLOW;
        if (ot == "OUTFALL" || ot == "NODE") return swmm_NODE_INFLOW;
    }
    if (prop == "INFLOW" && (ot == "NODE" || ot == "STORAGE" || ot == "JUNCTION" || ot == "OUTFALL")) return swmm_NODE_INFLOW;
    if (ot == "SUBCATCH" && prop == "RUNOFF") return swmm_SUBCATCH_RUNOFF;
    return -1;
}

static bool LoadMapping(double* outargs, int* status) {
    if (s_mapping_loaded) return true;
    std::string err;
    if (!s_mapping.LoadFromFile(CONFIG_FILE, err)) {
        Log(1, "Mapping load failed: %s", err.c_str());
        SetError(outargs, status, "Mapping file not found. Run: python generate_mapping.py model.inp");
        return false;
    }
    
    // Set log level from JSON
    std::string level = s_mapping.GetLoggingLevel();
    if (level == "DEBUG") s_log_level = 3;
    else if (level == "INFO") s_log_level = 2;
    else if (level == "ERROR") s_log_level = 1;
    else if (level == "OFF" || level == "NONE") s_log_level = 0;
    
    Log(2, "Log level set to: %s (%d)", level.c_str(), s_log_level);
    s_mapping_loaded = true;
    return true;
}

static void Cleanup(int* status, double* outargs) {
    if (!s_swmm_running) return;
    int e = swmm_end();
    int c = swmm_close();
    s_swmm_running = false;
    s_first_calculate = true;
    s_inputs.clear();
    s_outputs.clear();
    s_pending_inputs.clear();
    if (e != 0 && *status == XF_SUCCESS) HandleSwmmError(outargs, status);
    else if (c != 0 && *status == XF_SUCCESS) HandleSwmmError(outargs, status);
}

extern "C" void __declspec(dllexport) SwmmGoldSimBridge(int methodID, int* status, double* inargs, double* outargs) {
    *status = XF_SUCCESS;
    Log(2, "=== Method called: %d ===", methodID);

    switch (methodID) {
    case XF_REP_VERSION:
        Log(2, "XF_REP_VERSION called");
        outargs[0] = DLL_VERSION;
        Log(2, "XF_REP_VERSION returning: %.1f", DLL_VERSION);
        break;

    case XF_REP_ARGUMENTS:
        Log(2, "XF_REP_ARGUMENTS called");
        if (!LoadMapping(outargs, status)) {
            Log(1, "XF_REP_ARGUMENTS: LoadMapping failed");
            break;
        }
        outargs[0] = (double)s_mapping.GetInputCount();
        outargs[1] = (double)s_mapping.GetOutputCount();
        Log(2, "REP_ARGUMENTS: %d inputs, %d outputs", s_mapping.GetInputCount(), s_mapping.GetOutputCount());
        break;

    case XF_INITIALIZE:
        {
            Log(2, "XF_INITIALIZE called");
            if (s_swmm_running) { 
                Log(2, "SWMM already running, cleaning up first");
                Cleanup(status, outargs); 
                if (*status != XF_SUCCESS) {
                    Log(1, "Cleanup failed during re-initialization");
                    break;
                }
            }
            
            if (!LoadMapping(outargs, status)) {
                Log(1, "XF_INITIALIZE: LoadMapping failed");
                break;
            }
            Log(2, "Mapping loaded successfully");

            // Open SWMM
            Log(2, "Opening SWMM model: model.inp");
            int open_err = swmm_open("model.inp", "model.rpt", "model.out");
            if (open_err != 0) { 
                Log(1, "swmm_open failed with error: %d", open_err);
                HandleSwmmError(outargs, status); 
                break; 
            }
            Log(2, "swmm_open succeeded");
            
            Log(2, "Starting SWMM simulation");
            int start_err = swmm_start(1);
            if (start_err != 0) { 
                Log(1, "swmm_start failed with error: %d", start_err);
                swmm_close(); 
                HandleSwmmError(outargs, status); 
                break; 
            }
            Log(2, "swmm_start succeeded");

            // Resolve inputs
            Log(2, "Resolving %d inputs", s_mapping.GetInputCount());
            s_inputs.clear();
            for (const auto& inp : s_mapping.GetInputs()) {
                Log(2, "  Input[%d]: %s (%s/%s)", inp.interface_index, inp.name.c_str(), inp.object_type.c_str(), inp.property.c_str());
                int obj = ObjTypeToSwmm(inp.object_type);
                int prop = InputPropToEnum(inp.object_type, inp.property);
                
                // PROPERTY_SKIP is valid (for SYSTEM/ELAPSEDTIME)
                if (obj < 0 || (prop < 0 && prop != PROPERTY_SKIP)) {
                    sprintf_s(s_error_buf, "Unknown input: %s/%s", inp.object_type.c_str(), inp.property.c_str());
                    Log(1, "%s", s_error_buf);
                    Cleanup(status, outargs); SetError(outargs, status, s_error_buf); return;
                }
                
                int idx = (inp.object_type == "SYSTEM") ? 0 : swmm_getIndex((swmm_Object)obj, inp.name.c_str());
                if (inp.object_type != "SYSTEM" && idx < 0) {
                    sprintf_s(s_error_buf, "Element not found: %s", inp.name.c_str());
                    Log(1, "%s", s_error_buf);
                    Cleanup(status, outargs); SetError(outargs, status, s_error_buf); return;
                }
                Log(2, "    Resolved: obj=%d, prop=%d, idx=%d", obj, prop, idx);
                s_inputs.push_back({ inp.interface_index, prop, idx });
            }

            // Resolve outputs
            Log(2, "Resolving %d outputs", s_mapping.GetOutputCount());
            s_outputs.clear();
            for (const auto& out : s_mapping.GetOutputs()) {
                Log(2, "  Output[%d]: %s (%s/%s)", out.interface_index, out.name.c_str(), out.object_type.c_str(), out.property.c_str());
                int obj = ObjTypeToSwmm(out.object_type);
                int prop = OutputPropToEnum(out.object_type, out.property);
                if (obj < 0 || prop < 0) {
                    sprintf_s(s_error_buf, "Unknown output: %s/%s", out.object_type.c_str(), out.property.c_str());
                    Log(1, "%s", s_error_buf);
                    Cleanup(status, outargs); SetError(outargs, status, s_error_buf); return;
                }
                int idx = swmm_getIndex((swmm_Object)obj, out.name.c_str());
                if (idx < 0) {
                    sprintf_s(s_error_buf, "Element not found: %s", out.name.c_str());
                    Log(1, "%s", s_error_buf);
                    Cleanup(status, outargs); SetError(outargs, status, s_error_buf); return;
                }
                Log(2, "    Resolved: obj=%d, prop=%d, idx=%d", obj, prop, idx);
                s_outputs.push_back({ out.interface_index, prop, idx });
            }

            s_swmm_running = true;
            s_first_calculate = true;
            s_pending_inputs.clear();
            s_pending_inputs.resize(s_mapping.GetInputCount(), 0.0);
            Log(2, "INITIALIZE complete: %zu inputs, %zu outputs resolved", s_inputs.size(), s_outputs.size());
        }
        break;

    case XF_CALCULATE:
        {
            Log(2, "XF_CALCULATE called");
            if (!s_swmm_running) { 
                Log(1, "XF_CALCULATE called but SWMM not running!");
                *status = XF_FAILURE; 
                break; 
            }

            // On first call, we need to get initial outputs before any stepping
            if (s_first_calculate) {
                Log(2, "First calculate - getting initial outputs and storing inputs for next step");
                
                // Get initial outputs (before any stepping)
                Log(2, "Getting %zu initial outputs", s_outputs.size());
                for (const auto& r : s_outputs) {
                    double val = swmm_getValue(r.prop_enum, r.swmm_idx);
                    outargs[r.iface_idx] = val;
                    Log(2, "  Output[%d]: prop=%d, idx=%d, value=%.6f", r.iface_idx, r.prop_enum, r.swmm_idx, val);
                }
                
                // Store the inputs for the next timestep
                for (const auto& r : s_inputs) {
                    s_pending_inputs[r.iface_idx] = inargs[r.iface_idx];
                    Log(2, "  Stored input[%d] for next step: value=%.4f", r.iface_idx, inargs[r.iface_idx]);
                }
                
                s_first_calculate = false;
                Log(2, "XF_CALCULATE complete (first call)");
                break;
            }

            // For subsequent calls: apply the PREVIOUS inputs, step, then get outputs
            // This ensures outputs correspond to the same time period as the inputs
            
            // Apply the inputs that were provided in the PREVIOUS call
            Log(2, "Applying %zu inputs from previous timestep", s_inputs.size());
            for (const auto& r : s_inputs) {
                if (r.prop_enum != PROPERTY_SKIP) {
                    Log(2, "  Setting input[%d]: prop=%d, idx=%d, value=%.4f", r.iface_idx, r.prop_enum, r.swmm_idx, s_pending_inputs[r.iface_idx]);
                    swmm_setValue(r.prop_enum, r.swmm_idx, s_pending_inputs[r.iface_idx]);
                } else {
                    Log(2, "  Skipping input[%d] (PROPERTY_SKIP), value=%.4f", r.iface_idx, s_pending_inputs[r.iface_idx]);
                }
            }

            // Step SWMM forward - may need multiple internal steps
            Log(2, "Calling swmm_step");
            double elapsed;
            int ec = swmm_step(&elapsed);
            Log(2, "swmm_step returned: %d, elapsed=%.6f days (%.2f minutes)", ec, elapsed, elapsed * 1440.0);
            
            if (ec < 0) { 
                Log(1, "swmm_step failed with error: %d", ec);
                HandleSwmmError(outargs, status); 
                break; 
            }
            if (ec > 0) { 
                Log(2, "Simulation ended normally");
                Cleanup(status, outargs); 
                break; 
            }

            // Get outputs for the timestep we just completed
            Log(2, "Getting %zu outputs", s_outputs.size());
            for (const auto& r : s_outputs) {
                double val = swmm_getValue(r.prop_enum, r.swmm_idx);
                outargs[r.iface_idx] = val;
                Log(2, "  Output[%d]: prop=%d, idx=%d, value=%.6f", r.iface_idx, r.prop_enum, r.swmm_idx, val);
            }
            
            // Store the NEW inputs for the next timestep
            for (const auto& r : s_inputs) {
                s_pending_inputs[r.iface_idx] = inargs[r.iface_idx];
                Log(2, "  Stored input[%d] for next step: value=%.4f", r.iface_idx, inargs[r.iface_idx]);
            }
            
            Log(2, "XF_CALCULATE complete");
        }
        break;

    case XF_CLEANUP:
        Log(2, "XF_CLEANUP called");
        Cleanup(status, outargs);
        *status = XF_SUCCESS;
        Log(2, "XF_CLEANUP complete");
        break;

    default:
        Log(1, "Unknown method ID: %d", methodID);
        *status = XF_FAILURE;
        break;
    }
    Log(2, "=== Method %d complete, status=%d ===", methodID, *status);
}
