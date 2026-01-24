//-----------------------------------------------------------------------------
//   MappingLoader.h
//
//   Project: GoldSim-SWMM Bridge DLL
//   Description: Loads and manages interface mapping from JSON configuration
//
//   This class reads the SwmmGoldSimBridge.json file that defines the
//   dynamic interface between GoldSim and SWMM, eliminating hardcoded
//   interface definitions.
//-----------------------------------------------------------------------------

#ifndef MAPPING_LOADER_H
#define MAPPING_LOADER_H

#include <string>
#include <vector>

//=============================================================================
// MappingLoader - Loads interface mapping from JSON file
//=============================================================================

class MappingLoader {
public:
    //-------------------------------------------------------------------------
    // InputMapping - Defines a single input element
    //-------------------------------------------------------------------------
    struct InputMapping {
        int interface_index;        // Position in GoldSim inargs array (0-based)
        std::string name;           // Element name from SWMM model
        std::string object_type;    // SWMM object type (e.g., "GAGE", "SYSTEM")
        std::string property;       // Property name (e.g., "RAINFALL", "ELAPSEDTIME")
        int swmm_index;            // SWMM API index for the element
        
        InputMapping()
            : interface_index(0)
            , swmm_index(-1)
        {}
    };
    
    //-------------------------------------------------------------------------
    // OutputMapping - Defines a single output element
    //-------------------------------------------------------------------------
    struct OutputMapping {
        int interface_index;        // Position in GoldSim outargs array (0-based)
        std::string name;           // Element name from SWMM model
        std::string object_type;    // SWMM object type (e.g., "STORAGE", "OUTFALL")
        std::string property;       // Property name (e.g., "VOLUME", "FLOW")
        int swmm_index;            // SWMM API index for the element
        
        OutputMapping()
            : interface_index(0)
            , swmm_index(-1)
        {}
    };

    //-------------------------------------------------------------------------
    // Constructor / Destructor
    //-------------------------------------------------------------------------
    MappingLoader();
    ~MappingLoader();

    // Prevent copying
    MappingLoader(const MappingLoader&) = delete;
    MappingLoader& operator=(const MappingLoader&) = delete;

    //-------------------------------------------------------------------------
    // LoadFromFile - Load mapping from JSON file
    //
    // Parameters:
    //   path  - Path to JSON mapping file
    //   error - Output parameter for error messages
    //
    // Returns:
    //   true if successful, false on error (error string populated)
    //
    // Requirements: 6.1, 6.2, 6.3
    //-------------------------------------------------------------------------
    bool LoadFromFile(const std::string& path, std::string& error);

    //-------------------------------------------------------------------------
    // Getter Methods
    //-------------------------------------------------------------------------
    
    // Get total number of inputs
    // Requirements: 6.4
    int GetInputCount() const;
    
    // Get total number of outputs
    // Requirements: 6.5
    int GetOutputCount() const;
    
    // Get reference to inputs vector
    const std::vector<InputMapping>& GetInputs() const;
    
    // Get reference to outputs vector
    const std::vector<OutputMapping>& GetOutputs() const;
    
    // Get INP file hash for validation
    // Requirements: 5.2, 7.5
    const std::string& GetHash() const;

private:
    //-------------------------------------------------------------------------
    // Member Variables
    //-------------------------------------------------------------------------
    std::vector<InputMapping> inputs_;      // Input element mappings
    std::vector<OutputMapping> outputs_;    // Output element mappings
    std::string inp_hash_;                  // Hash of INP file for validation
};

#endif // MAPPING_LOADER_H
