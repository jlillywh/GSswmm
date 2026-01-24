//-----------------------------------------------------------------------------
//   SwmmGoldSimBridge.cpp
//
//   Project: GoldSim-SWMM Bridge DLL
//   Description: Bridge DLL enabling bidirectional communication between
//                GoldSim simulation software and EPA SWMM hydraulic engine
//
//   Refactored to use modern C++ practices with proper encapsulation,
//   RAII, and modular design.
//-----------------------------------------------------------------------------

#include <windows.h>
#include <string>
#include <memory>
#include <stdexcept>
#include <cstdarg>
#include <fstream>
#include "include/swmm5.h"
#include "include/MappingLoader.h"

//=============================================================================
// Constants and Enumerations
//=============================================================================

namespace GoldSim {
    enum class MethodID {
        Initialize = 0,
        Calculate = 1,
        ReportVersion = 2,
        ReportArguments = 3,
        Cleanup = 99
    };

    enum class Status {
        Success = 0,
        Failure = 1,
        FailureWithMessage = -1
    };

    constexpr double VERSION = 1.04;
    constexpr int INPUT_COUNT = 2;   // ETime, Rainfall
    constexpr int OUTPUT_COUNT = 1;  // Runoff
    constexpr size_t ERROR_BUFFER_SIZE = 200;
    constexpr size_t PATH_BUFFER_SIZE = 260;
}

//=============================================================================
// PathHelper - Manages file paths relative to DLL location
//=============================================================================

class PathHelper {
public:
    // Get the directory where this DLL is located
    static std::string GetDllDirectory() {
        char dll_path[GoldSim::PATH_BUFFER_SIZE];
        HMODULE hModule = NULL;
        
        // Get handle to this DLL
        if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | 
                               GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                               (LPCSTR)&GetDllDirectory, &hModule)) {
            // Get full path to DLL
            if (GetModuleFileNameA(hModule, dll_path, sizeof(dll_path)) > 0) {
                // Extract directory by removing filename
                std::string path(dll_path);
                size_t last_slash = path.find_last_of("\\/");
                if (last_slash != std::string::npos) {
                    return path.substr(0, last_slash);
                }
            }
        }
        
        // Fallback to current directory
        return ".";
    }
    
    // Build a path relative to the DLL directory
    static std::string BuildDllRelativePath(const std::string& filename) {
        std::string dll_dir = GetDllDirectory();
        return dll_dir + "\\" + filename;
    }
};

//=============================================================================
// Logger Class - Handles debug logging with RAII
//=============================================================================

class Logger {
private:
    static bool first_write_;
    
    // Logging control: Set to false to disable all logging for production use
    // This significantly improves performance by eliminating file I/O overhead
    static constexpr bool ENABLE_LOGGING = false;
    
public:
    static void Debug(const char* format, ...) {
        // Skip logging if disabled
        if (!ENABLE_LOGGING) {
            return;
        }
        
        // Get DLL directory for log file
        std::string log_path = PathHelper::BuildDllRelativePath("bridge_debug.log");
        
        // Overwrite on first write, append thereafter
        const char* mode = first_write_ ? "w" : "a";
        first_write_ = false;
        
        FILE* log_file = nullptr;
        if (fopen_s(&log_file, log_path.c_str(), mode) == 0 && log_file) {
            SYSTEMTIME st;
            GetLocalTime(&st);
            fprintf(log_file, "[%02d:%02d:%02d.%03d] ", 
                    st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
            
            va_list args;
            va_start(args, format);
            vfprintf(log_file, format, args);
            va_end(args);
            
            fprintf(log_file, "\n");
            fclose(log_file);
        }
    }
};

// Initialize static member
bool Logger::first_write_ = true;

//=============================================================================
// FileValidator - Validates file paths before SWMM operations
//=============================================================================

class FileValidator {
public:
    static bool Exists(const std::string& path, std::string& error) {
        if (path.empty()) {
            error = "Error: Input file path is not provided\n"
                    "Context: File path is empty\n"
                    "Suggestion: Ensure the input file path is specified in the model configuration";
            return false;
        }

        DWORD fileAttrib = GetFileAttributesA(path.c_str());
        if (fileAttrib == INVALID_FILE_ATTRIBUTES) {
            error = "Error: Input file does not exist\n"
                    "Context: File path '" + path + "'\n" +
                    "Suggestion: Verify the file path is correct and the file exists";
            return false;
        }

        if (fileAttrib & FILE_ATTRIBUTE_DIRECTORY) {
            error = "Error: Input file path is a directory\n"
                    "Context: Path '" + path + "'\n" +
                    "Suggestion: Provide a file path, not a directory path";
            return false;
        }

        return true;
    }
};

//TODO: see if we can return the error 200 message when the inp file is ill formed

//=============================================================================
// SwmmSimulation - Encapsulates SWMM simulation state and operations
//=============================================================================

class SwmmSimulation {
private:
    bool is_running_;
    int subcatchment_index_;
    double last_swmm_time_;
    double accumulated_rainfall_;
    
    std::string input_file_;
    std::string report_file_;
    std::string output_file_;
    
    char error_buffer_[GoldSim::ERROR_BUFFER_SIZE];
    
    // Resolved SWMM indices from mapping
    std::vector<int> input_indices_;
    std::vector<int> output_indices_;

public:
    SwmmSimulation()
        : is_running_(false)
        , subcatchment_index_(0)
        , last_swmm_time_(-1.0)
        , accumulated_rainfall_(0.0)
        , input_file_("model.inp")  // Input file is in working directory (where GoldSim runs)
        , report_file_(PathHelper::BuildDllRelativePath("model.rpt"))  // Output files go with DLL
        , output_file_(PathHelper::BuildDllRelativePath("model.out"))
    {
        error_buffer_[0] = '\0';
    }

    ~SwmmSimulation() {
        if (is_running_) {
            Cleanup();
        }
    }

    // Prevent copying
    SwmmSimulation(const SwmmSimulation&) = delete;
    SwmmSimulation& operator=(const SwmmSimulation&) = delete;

    bool IsRunning() const { return is_running_; }
    int GetSubcatchmentIndex() const { return subcatchment_index_; }
    void SetSubcatchmentIndex(int index) { subcatchment_index_ = index; }
    const char* GetErrorMessage() const { return error_buffer_; }

    //-------------------------------------------------------------------------
    // Initialize - Start SWMM simulation
    //
    // Requirements: 7.1, 7.2, 7.3, 7.4, 7.5, 7.6
    //-------------------------------------------------------------------------
    bool Initialize(const MappingLoader& mapping, std::string& error) {
        Logger::Debug("=== Initialize ===");

        // Cleanup if already running
        if (is_running_) {
            Logger::Debug("SWMM already running, cleaning up first");
            if (!Cleanup()) {
                error = "Failed to cleanup existing simulation";
                return false;
            }
        }

        // Validate input file
        if (!FileValidator::Exists(input_file_, error)) {
            Logger::Debug("File validation failed: %s", error.c_str());
            // FileValidator already provides formatted error message
            return false;
        }

        // Open SWMM model
        Logger::Debug("Opening SWMM model: %s", input_file_.c_str());
        Logger::Debug("Report file: %s", report_file_.c_str());
        Logger::Debug("Output file: %s", output_file_.c_str());
        int error_code = swmm_open(input_file_.c_str(), report_file_.c_str(), output_file_.c_str());
        if (error_code != 0) {
            GetSwmmError(error);
            Logger::Debug("swmm_open failed: %s", error.c_str());
            
            // Try to read report file for detailed error information
            Logger::Debug("Attempting to read report file for detailed errors...");
            
            // Give SWMM a moment to write the file
            Sleep(100);
            
            std::ifstream report(report_file_);
            if (report.is_open()) {
                std::string line;
                int line_count = 0;
                Logger::Debug("--- Report File Contents (first 50 lines) ---");
                while (std::getline(report, line) && line_count < 50) {
                    Logger::Debug("  %s", line.c_str());
                    line_count++;
                }
                Logger::Debug("--- End Report File Contents ---");
                report.close();
            } else {
                Logger::Debug("Could not open report file: %s", report_file_.c_str());
                
                // Check if file exists
                DWORD fileAttrib = GetFileAttributesA(report_file_.c_str());
                if (fileAttrib == INVALID_FILE_ATTRIBUTES) {
                    Logger::Debug("Report file does not exist yet");
                } else {
                    Logger::Debug("Report file exists but cannot be opened");
                }
            }
            
            return false;
        }

        // Validate and resolve mapping elements
        Logger::Debug("Validating mapping elements");
        if (!ValidateMapping(mapping, error)) {
            swmm_close();
            Logger::Debug("ValidateMapping failed: %s", error.c_str());
            return false;
        }

        // Validate subcatchment index (legacy - may be removed in future)
        int subcatch_count = swmm_getCount(swmm_SUBCATCH);
        Logger::Debug("Subcatchment count: %d, using index: %d", subcatch_count, subcatchment_index_);
        
        if (subcatchment_index_ < 0 || subcatchment_index_ >= subcatch_count) {
            swmm_close();
            char buffer[GoldSim::ERROR_BUFFER_SIZE];
            sprintf_s(buffer, sizeof(buffer),
                     "Error: Subcatchment index out of range\n"
                     "Context: Index %d (valid range: 0-%d)\n"
                     "Suggestion: Verify the subcatchment index is within the valid range",
                     subcatchment_index_, subcatch_count - 1);
            error = buffer;
            Logger::Debug("%s", error.c_str());
            return false;
        }

        // Start simulation
        Logger::Debug("Starting SWMM simulation");
        error_code = swmm_start(1);
        if (error_code != 0) {
            swmm_close();
            GetSwmmError(error);
            Logger::Debug("swmm_start failed: %s", error.c_str());
            return false;
        }

        // Reset state
        is_running_ = true;
        last_swmm_time_ = -1.0;
        accumulated_rainfall_ = 0.0;

        Logger::Debug("Initialize successful");
        return true;
    }

    //-------------------------------------------------------------------------
    // Calculate - Advance simulation and exchange data
    //
    // Requirements: 8.1, 8.2, 8.3, 8.4, 8.5, 8.6
    //-------------------------------------------------------------------------
    bool Calculate(const double* inargs, double* outargs, 
                   const MappingLoader& mapping, std::string& error) {
        Logger::Debug("=== Calculate ===");

        if (!is_running_) {
            error = "SWMM not running";
            Logger::Debug("ERROR: %s", error.c_str());
            return false;
        }

        // Get elapsed time from first input (always index 0)
        double elapsed_seconds = inargs[0];
        double elapsed_days = elapsed_seconds / 86400.0;
        
        Logger::Debug("Elapsed time: %.2fs (%.6f days)", elapsed_seconds, elapsed_days);

        // Iterate through input mappings and set values (Requirements 8.1, 8.2)
        const std::vector<MappingLoader::InputMapping>& inputs = mapping.GetInputs();
        for (size_t i = 0; i < inputs.size(); i++) {
            const MappingLoader::InputMapping& input = inputs[i];
            
            // Skip elapsed time - it's not set on SWMM, just used for timing
            if (input.object_type == "SYSTEM") {
                Logger::Debug("Input %d: %s (SYSTEM property, skipping)", 
                             static_cast<int>(i), input.name.c_str());
                continue;
            }
            
            // Get value from inargs array
            double value = inargs[input.interface_index];
            
            // Determine SWMM property constant (Requirements 5.1, 5.2, 5.3, 5.4, 5.5)
            int swmm_property = -1;
            if (input.object_type == "GAGE" && input.property == "RAINFALL") {
                // Requirement 5.5: Rain gage rainfall
                swmm_property = swmm_GAGE_RAINFALL;
            } 
            else if ((input.object_type == "PUMP" || 
                      input.object_type == "ORIFICE" || 
                      input.object_type == "WEIR") && 
                     input.property == "SETTING") {
                // Requirements 5.1, 5.2, 5.3: Pump/Orifice/Weir setting
                swmm_property = swmm_LINK_SETTING;
            }
            else if (input.object_type == "NODE" && input.property == "LATFLOW") {
                // Requirement 5.4: Node lateral inflow
                swmm_property = swmm_NODE_LATFLOW;
            }
            else {
                // Requirement 5.6: Descriptive error for unknown property combinations
                error = "Error: Unknown input property combination\n"
                        "Context: " + input.object_type + "." + input.property + 
                        " for element '" + input.name + "'\n" +
                        "Suggestion: Valid combinations are:\n" +
                        "  - GAGE.RAINFALL\n" +
                        "  - PUMP.SETTING\n" +
                        "  - ORIFICE.SETTING\n" +
                        "  - WEIR.SETTING\n" +
                        "  - NODE.LATFLOW";
                Logger::Debug("ERROR: %s", error.c_str());
                return false;
            }
            
            // Validate handle (Requirement 8.6, 8.3)
            if (input_indices_[i] < 0 && input.object_type != "SYSTEM") {
                error = "Error: Invalid element handle\n"
                        "Context: Input element '" + input.name + "' (type: " + input.object_type + ")\n" +
                        "Suggestion: Element may not have been properly resolved during initialization";
                Logger::Debug("ERROR: %s", error.c_str());
                return false;
            }
            
            // Set value on SWMM element
            int swmm_index = input_indices_[i];
            swmm_setValue(swmm_property, swmm_index, value);
            
            Logger::Debug("Input %d: Set %s[%d].%s = %.6f", 
                         static_cast<int>(i), input.object_type.c_str(), 
                         swmm_index, input.property.c_str(), value);
            
            // Track accumulated rainfall for legacy logging
            if (input.object_type == "GAGE") {
                accumulated_rainfall_ += value;
            }
        }

        // Determine if we should step SWMM
        double routing_step_seconds = swmm_getValue(swmm_ROUTESTEP, 0);
        double routing_step_days = routing_step_seconds / 86400.0;
        
        bool should_step = ShouldStepSimulation(elapsed_days, routing_step_days);

        if (should_step) {
            double swmm_elapsed_time = 0.0;
            int error_code = swmm_step(&swmm_elapsed_time);
            
            Logger::Debug("swmm_step returned %d, elapsed=%.6f days", error_code, swmm_elapsed_time);

            if (error_code < 0) {
                GetSwmmError(error);
                Logger::Debug("ERROR: %s", error.c_str());
                return false;
            }
            else if (error_code > 0) {
                Logger::Debug("Simulation ended normally");
                Cleanup();
                
                // Zero out all outputs when simulation ends
                const std::vector<MappingLoader::OutputMapping>& outputs = mapping.GetOutputs();
                for (size_t i = 0; i < outputs.size(); i++) {
                    outargs[outputs[i].interface_index] = 0.0;
                }
                
                return true;
            }

            last_swmm_time_ = elapsed_days;
            accumulated_rainfall_ = 0.0;
        }

        // Iterate through output mappings and retrieve values (Requirements 8.3, 8.4, 8.5)
        const std::vector<MappingLoader::OutputMapping>& outputs = mapping.GetOutputs();
        for (size_t i = 0; i < outputs.size(); i++) {
            const MappingLoader::OutputMapping& output = outputs[i];
            
            // Validate handle (Requirement 8.6)
            if (output_indices_[i] < 0) {
                error = "Error: Invalid element handle\n"
                        "Context: Output element '" + output.name + "' (type: " + output.object_type + ")\n" +
                        "Suggestion: Element may not have been properly resolved during initialization";
                Logger::Debug("ERROR: %s", error.c_str());
                return false;
            }
            
            // Determine SWMM property constant
            int swmm_property = -1;
            if (output.object_type == "STORAGE" && output.property == "VOLUME") {
                swmm_property = swmm_NODE_VOLUME;
            } else if (output.object_type == "OUTFALL" && output.property == "FLOW") {
                swmm_property = swmm_NODE_INFLOW;
            } else if (output.object_type == "ORIFICE" && output.property == "FLOW") {
                swmm_property = swmm_LINK_FLOW;
            } else if (output.object_type == "WEIR" && output.property == "FLOW") {
                swmm_property = swmm_LINK_FLOW;
            } else if (output.object_type == "SUBCATCH" && output.property == "RUNOFF") {
                swmm_property = swmm_SUBCATCH_RUNOFF;
            } else {
                error = "Error: Unknown output property\n"
                        "Context: " + output.object_type + "." + output.property + " for element '" + output.name + "'\n" +
                        "Suggestion: Valid combinations are:\n" +
                        "  - STORAGE.VOLUME\n" +
                        "  - OUTFALL.FLOW\n" +
                        "  - ORIFICE.FLOW\n" +
                        "  - WEIR.FLOW\n" +
                        "  - SUBCATCH.RUNOFF";
                Logger::Debug("ERROR: %s", error.c_str());
                return false;
            }
            
            // Get value from SWMM element
            int swmm_index = output_indices_[i];
            double value = swmm_getValue(swmm_property, swmm_index);
            
            // Populate outargs in mapping order (Requirement 8.5)
            outargs[output.interface_index] = value;
            
            Logger::Debug("Output %d: Get %s[%d].%s = %.6f", 
                         static_cast<int>(i), output.object_type.c_str(), 
                         swmm_index, output.property.c_str(), value);
        }

        return true;
    }

    //-------------------------------------------------------------------------
    // Cleanup - Terminate SWMM simulation
    //-------------------------------------------------------------------------
    bool Cleanup() {
        Logger::Debug("=== Cleanup ===");

        if (!is_running_) {
            Logger::Debug("SWMM not running, nothing to cleanup");
            return true;
        }

        int end_error = swmm_end();
        int close_error = swmm_close();
        is_running_ = false;

        if (end_error != 0) {
            Logger::Debug("swmm_end failed with code %d", end_error);
            return false;
        }

        if (close_error != 0) {
            Logger::Debug("swmm_close failed with code %d", close_error);
            return false;
        }

        Logger::Debug("Cleanup successful");
        return true;
    }

private:
    //-------------------------------------------------------------------------
    // ValidateMapping - Validate and resolve element names from mapping
    //
    // Requirements: 6.1, 6.2, 6.3, 6.4, 6.5, 6.6
    //-------------------------------------------------------------------------
    bool ValidateMapping(const MappingLoader& mapping, std::string& error) {
        Logger::Debug("=== ValidateMapping ===");
        
        // Clear any existing indices
        input_indices_.clear();
        output_indices_.clear();
        
        // Resolve input element indices
        const std::vector<MappingLoader::InputMapping>& inputs = mapping.GetInputs();
        Logger::Debug("Validating %d input elements", static_cast<int>(inputs.size()));
        
        for (size_t i = 0; i < inputs.size(); i++) {
            const MappingLoader::InputMapping& input = inputs[i];
            
            // Skip elapsed time - it's a system property, not an element
            if (input.object_type == "SYSTEM") {
                Logger::Debug("Input %d: %s (SYSTEM property, no index needed)", 
                             static_cast<int>(i), input.name.c_str());
                input_indices_.push_back(-1); // Use -1 to indicate system property
                continue;
            }
            
            // Determine SWMM object type for resolution (Requirements 6.1, 6.2, 6.3, 6.4)
            int swmm_obj_type = -1;
            if (input.object_type == "GAGE") {
                swmm_obj_type = swmm_GAGE;
            } 
            else if (input.object_type == "PUMP" || 
                     input.object_type == "ORIFICE" || 
                     input.object_type == "WEIR") {
                // Pumps, orifices, and weirs are all link types (Requirements 6.1, 6.2, 6.3)
                swmm_obj_type = swmm_LINK;
            }
            else if (input.object_type == "NODE") {
                // Nodes use swmm_NODE (Requirement 6.4)
                swmm_obj_type = swmm_NODE;
            }
            else {
                // Return descriptive error for unknown object types (Requirement 6.6, 8.2)
                error = "Error: Unknown input object type\n"
                        "Context: Type '" + input.object_type + "' for element '" + input.name + "'\n" +
                        "Suggestion: Supported types are GAGE, PUMP, ORIFICE, WEIR, NODE";
                Logger::Debug("ERROR: %s", error.c_str());
                return false;
            }
            
            // Resolve element name to SWMM index (Requirement 6.5, 8.1)
            int swmm_index = swmm_getIndex(swmm_obj_type, input.name.c_str());
            if (swmm_index < 0) {
                error = "Error: SWMM element not found\n" 
                        "Context: " + input.object_type + " '" + input.name + "'\n" +
                        "Suggestion: Verify that " + input.object_type + " '" + input.name + 
                        "' exists in the SWMM .inp file";
                Logger::Debug("ERROR: %s", error.c_str());
                return false;
            }
            
            Logger::Debug("Input %d: %s resolved to SWMM index %d", 
                         static_cast<int>(i), input.name.c_str(), swmm_index);
            input_indices_.push_back(swmm_index);
        }
        
        // Resolve output element indices
        const std::vector<MappingLoader::OutputMapping>& outputs = mapping.GetOutputs();
        Logger::Debug("Validating %d output elements", static_cast<int>(outputs.size()));
        
        for (size_t i = 0; i < outputs.size(); i++) {
            const MappingLoader::OutputMapping& output = outputs[i];
            
            // Determine SWMM object type
            int swmm_obj_type = -1;
            if (output.object_type == "STORAGE" || output.object_type == "OUTFALL") {
                swmm_obj_type = swmm_NODE;
            } else if (output.object_type == "ORIFICE" || output.object_type == "WEIR") {
                swmm_obj_type = swmm_LINK;
            } else if (output.object_type == "SUBCATCH") {
                swmm_obj_type = swmm_SUBCATCH;
            } else {
                error = "Error: Unknown output object type\n"
                        "Context: Type '" + output.object_type + "' for element '" + output.name + "'\n" +
                        "Suggestion: Supported types are STORAGE, OUTFALL, ORIFICE, WEIR, SUBCATCH";
                Logger::Debug("ERROR: %s", error.c_str());
                return false;
            }
            
            // Resolve element name to SWMM index
            int swmm_index = swmm_getIndex(swmm_obj_type, output.name.c_str());
            if (swmm_index < 0) {
                error = "Error: SWMM element not found\n"
                        "Context: " + output.object_type + " '" + output.name + "'\n" +
                        "Suggestion: Verify that " + output.object_type + " '" + output.name + 
                        "' exists in the SWMM .inp file";
                Logger::Debug("ERROR: %s", error.c_str());
                return false;
            }
            
            Logger::Debug("Output %d: %s resolved to SWMM index %d", 
                         static_cast<int>(i), output.name.c_str(), swmm_index);
            output_indices_.push_back(swmm_index);
        }
        
        Logger::Debug("ValidateMapping successful");
        return true;
    }

    //-------------------------------------------------------------------------
    // ShouldStepSimulation - Determine if SWMM should advance
    //-------------------------------------------------------------------------
    bool ShouldStepSimulation(double elapsed_days, double routing_step_days) {
        if (last_swmm_time_ < 0.0) {
            Logger::Debug("First call - will step SWMM");
            return true;
        }

        double time_since_last_step = elapsed_days - last_swmm_time_;
        Logger::Debug("Time since last step: %.6f days (routing step: %.6f days)",
                     time_since_last_step, routing_step_days);

        // Use small tolerance for floating point comparison
        if (time_since_last_step >= routing_step_days * 0.99) {
            Logger::Debug("Will step SWMM");
            return true;
        }

        Logger::Debug("Skipping step - not enough time elapsed");
        return false;
    }

    //-------------------------------------------------------------------------
    // GetSwmmError - Retrieve error message from SWMM
    //-------------------------------------------------------------------------
    void GetSwmmError(std::string& error) {
        swmm_getError(error_buffer_, sizeof(error_buffer_));
        error_buffer_[sizeof(error_buffer_) - 1] = '\0';
        error = error_buffer_;
    }
};

//=============================================================================
// BridgeHandler - Handles GoldSim method calls
//=============================================================================

class BridgeHandler {
private:
    SwmmSimulation simulation_;
    MappingLoader mapping_;
    bool mapping_loaded_;
    int calculate_call_count_;
    double last_logged_time_;

public:
    BridgeHandler()
        : mapping_loaded_(false)
        , calculate_call_count_(0)
        , last_logged_time_(-1.0)
    {}

    //-------------------------------------------------------------------------
    // HandleMethod - Main dispatcher for GoldSim method calls
    //-------------------------------------------------------------------------
    void HandleMethod(int method_id, int* status, double* inargs, double* outargs) {
        Logger::Debug("=== Method Called: %d ===", method_id);
        
        *status = static_cast<int>(GoldSim::Status::Success);
        std::string error;

        switch (static_cast<GoldSim::MethodID>(method_id)) {
            case GoldSim::MethodID::Initialize:
                HandleInitialize(status, outargs, error);
                break;

            case GoldSim::MethodID::Calculate:
                HandleCalculate(status, inargs, outargs, error);
                break;

            case GoldSim::MethodID::ReportVersion:
                HandleReportVersion(outargs);
                break;

            case GoldSim::MethodID::ReportArguments:
                HandleReportArguments(status, outargs, error);
                break;

            case GoldSim::MethodID::Cleanup:
                HandleCleanup(status, outargs, error);
                break;

            default:
                Logger::Debug("ERROR: Unknown method ID %d", method_id);
                *status = static_cast<int>(GoldSim::Status::Failure);
                break;
        }

        Logger::Debug("=== Method %d Complete, Status = %d ===\n", method_id, *status);
    }

    //-------------------------------------------------------------------------
    // SetSubcatchmentIndex - For testing purposes
    //-------------------------------------------------------------------------
    void SetSubcatchmentIndex(int index) {
        simulation_.SetSubcatchmentIndex(index);
    }

private:
    void HandleInitialize(int* status, double* outargs, std::string& error) {
        // Log DLL version and initialization start
        Logger::Debug("========================================");
        Logger::Debug("GSswmm Bridge DLL Version %.2f", GoldSim::VERSION);
        Logger::Debug("Initialization started");
        Logger::Debug("========================================");
        
        // Always reload mapping file on initialize to support dynamic testing
        const std::string mapping_file = "SwmmGoldSimBridge.json";
        Logger::Debug("Loading mapping file: %s", mapping_file.c_str());
        
        if (!mapping_.LoadFromFile(mapping_file, error)) {
            Logger::Debug("Failed to load mapping file: %s", error.c_str());
            SetErrorMessage(outargs, error);
            *status = static_cast<int>(GoldSim::Status::FailureWithMessage);
            return;
        }
        
        mapping_loaded_ = true;
        Logger::Debug("Mapping file loaded successfully");
        Logger::Debug("  Input count: %d", mapping_.GetInputs().size());
        Logger::Debug("  Output count: %d", mapping_.GetOutputs().size());
        
        if (simulation_.Initialize(mapping_, error)) {
            Logger::Debug("========================================");
            Logger::Debug("Initialization SUCCESSFUL");
            Logger::Debug("========================================");
            *status = static_cast<int>(GoldSim::Status::Success);
        } else {
            Logger::Debug("========================================");
            Logger::Debug("Initialization FAILED: %s", error.c_str());
            Logger::Debug("========================================");
            SetErrorMessage(outargs, error);
            *status = static_cast<int>(GoldSim::Status::FailureWithMessage);
        }
    }

    void HandleCalculate(int* status, double* inargs, double* outargs, std::string& error) {
        calculate_call_count_++;
        bool is_first_call = (calculate_call_count_ == 1);
        
        // Load mapping file if not already loaded
        if (!mapping_loaded_) {
            const std::string mapping_file = "SwmmGoldSimBridge.json";
            Logger::Debug("Loading mapping file: %s", mapping_file.c_str());
            
            if (!mapping_.LoadFromFile(mapping_file, error)) {
                Logger::Debug("Failed to load mapping file: %s", error.c_str());
                SetErrorMessage(outargs, error);
                *status = static_cast<int>(GoldSim::Status::FailureWithMessage);
                return;
            }
            
            mapping_loaded_ = true;
            Logger::Debug("Mapping file loaded successfully");
        }

        // Log detailed info for first call
        if (is_first_call) {
            Logger::Debug("========================================");
            Logger::Debug("FIRST TIMESTEP - Calculate call #%d", calculate_call_count_);
            Logger::Debug("========================================");
            
            // Log all inputs
            const std::vector<MappingLoader::InputMapping>& inputs = mapping_.GetInputs();
            Logger::Debug("Input values from GoldSim:");
            for (size_t i = 0; i < inputs.size(); i++) {
                Logger::Debug("  [%d] %s = %.6f", 
                             static_cast<int>(i), inputs[i].name.c_str(), inargs[i]);
            }
        }

        if (simulation_.Calculate(inargs, outargs, mapping_, error)) {
            // Log detailed info for first call
            if (is_first_call) {
                const std::vector<MappingLoader::OutputMapping>& outputs = mapping_.GetOutputs();
                Logger::Debug("Output values to GoldSim:");
                for (size_t i = 0; i < outputs.size(); i++) {
                    Logger::Debug("  [%d] %s = %.6f", 
                                 static_cast<int>(i), outputs[i].name.c_str(), outargs[i]);
                }
                Logger::Debug("========================================");
            }
            
            last_logged_time_ = inargs[0];
            *status = static_cast<int>(GoldSim::Status::Success);
        } else {
            // If SWMM is not running, it means simulation ended - log final state
            if (!simulation_.IsRunning() && error == "SWMM not running") {
                Logger::Debug("========================================");
                Logger::Debug("SIMULATION ENDED - Calculate call #%d", calculate_call_count_);
                Logger::Debug("Last elapsed time: %.2f seconds", last_logged_time_);
                Logger::Debug("========================================");
                *status = static_cast<int>(GoldSim::Status::Failure);
            } else {
                Logger::Debug("Calculate FAILED: %s", error.c_str());
                SetErrorMessage(outargs, error);
                *status = static_cast<int>(GoldSim::Status::FailureWithMessage);
            }
        }
    }

    void HandleReportVersion(double* outargs) {
        Logger::Debug("Returning version %.2f", GoldSim::VERSION);
        outargs[0] = GoldSim::VERSION;
    }

    void HandleReportArguments(int* status, double* outargs, std::string& error) {
        // Load mapping file if not already loaded
        if (!mapping_loaded_) {
            const std::string mapping_file = "SwmmGoldSimBridge.json";
            Logger::Debug("Loading mapping file: %s", mapping_file.c_str());
            
            if (!mapping_.LoadFromFile(mapping_file, error)) {
                Logger::Debug("Failed to load mapping file: %s", error.c_str());
                SetErrorMessage(outargs, error);
                *status = static_cast<int>(GoldSim::Status::FailureWithMessage);
                return;
            }
            
            mapping_loaded_ = true;
            Logger::Debug("Mapping file loaded successfully");
        }
        
        int input_count = mapping_.GetInputCount();
        int output_count = mapping_.GetOutputCount();
        
        Logger::Debug("Returning %d inputs, %d outputs", input_count, output_count);
        outargs[0] = static_cast<double>(input_count);
        outargs[1] = static_cast<double>(output_count);
        *status = static_cast<int>(GoldSim::Status::Success);
    }

    void HandleCleanup(int* status, double* outargs, std::string& error) {
        Logger::Debug("========================================");
        Logger::Debug("CLEANUP - Closing SWMM simulation");
        Logger::Debug("Total Calculate calls: %d", calculate_call_count_);
        Logger::Debug("========================================");
        
        if (simulation_.Cleanup()) {
            Logger::Debug("Cleanup SUCCESSFUL");
            *status = static_cast<int>(GoldSim::Status::Success);
        } else {
            Logger::Debug("Cleanup FAILED");
            SetErrorMessage(outargs, simulation_.GetErrorMessage());
            *status = static_cast<int>(GoldSim::Status::FailureWithMessage);
        }
        
        // Reset counters for next run
        calculate_call_count_ = 0;
        last_logged_time_ = -1.0;
    }

    void SetErrorMessage(double* outargs, const std::string& error) {
        static char error_buffer[GoldSim::ERROR_BUFFER_SIZE];
        strncpy_s(error_buffer, error.c_str(), sizeof(error_buffer) - 1);
        error_buffer[sizeof(error_buffer) - 1] = '\0';
        
        ULONG_PTR* pAddr = reinterpret_cast<ULONG_PTR*>(outargs);
        *pAddr = reinterpret_cast<ULONG_PTR>(error_buffer);
    }
};

//=============================================================================
// Global Bridge Instance
//=============================================================================

static BridgeHandler g_bridge;

//=============================================================================
// Exported Functions - C Interface for GoldSim
//=============================================================================

extern "C" {

__declspec(dllexport) void SwmmGoldSimBridge(
    int methodID,
    int* status,
    double* inargs,
    double* outargs
) {
    g_bridge.HandleMethod(methodID, status, inargs, outargs);
}

__declspec(dllexport) void SetSubcatchmentIndex(int index) {
    g_bridge.SetSubcatchmentIndex(index);
}

} // extern "C"
