//-----------------------------------------------------------------------------
//   MappingLoader.h
//   Loads interface mapping from JSON configuration
//-----------------------------------------------------------------------------

#ifndef MAPPING_LOADER_H
#define MAPPING_LOADER_H

#include <string>
#include <vector>

class MappingLoader {
public:
    struct InputMapping {
        int interface_index;
        std::string name;
        std::string object_type;
        std::string property;
        int swmm_index;
        InputMapping() : interface_index(0), swmm_index(-1) {}
    };
    
    struct OutputMapping {
        int interface_index;
        std::string name;
        std::string object_type;
        std::string property;
        int swmm_index;
        OutputMapping() : interface_index(0), swmm_index(-1) {}
    };

    MappingLoader();
    ~MappingLoader();
    MappingLoader(const MappingLoader&) = delete;
    MappingLoader& operator=(const MappingLoader&) = delete;

    bool LoadFromFile(const std::string& path, std::string& error);
    
    int GetInputCount() const;
    int GetOutputCount() const;
    const std::vector<InputMapping>& GetInputs() const;
    const std::vector<OutputMapping>& GetOutputs() const;
    const std::string& GetLoggingLevel() const;

private:
    std::vector<InputMapping> inputs_;
    std::vector<OutputMapping> outputs_;
    std::string logging_level_;
};

#endif
