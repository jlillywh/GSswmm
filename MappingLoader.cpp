//-----------------------------------------------------------------------------
//   MappingLoader.cpp
//
//   Project: GoldSim-SWMM Bridge DLL
//   Description: Implementation of MappingLoader class
//
//   Loads interface mapping from JSON configuration file to enable
//   dynamic interface configuration between GoldSim and SWMM.
//-----------------------------------------------------------------------------

#include "include/MappingLoader.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

//=============================================================================
// Helper Functions for JSON Parsing
//=============================================================================

// Trim whitespace from both ends of a string
static std::string trim(const std::string& str) {
    size_t start = 0;
    size_t end = str.length();
    
    while (start < end && std::isspace(static_cast<unsigned char>(str[start]))) {
        start++;
    }
    
    while (end > start && std::isspace(static_cast<unsigned char>(str[end - 1]))) {
        end--;
    }
    
    return str.substr(start, end - start);
}

// Extract string value from JSON string (removes quotes)
static std::string extractString(const std::string& value) {
    std::string trimmed = trim(value);
    if (trimmed.length() >= 2 && trimmed[0] == '"' && trimmed[trimmed.length() - 1] == '"') {
        return trimmed.substr(1, trimmed.length() - 2);
    }
    return trimmed;
}

// Extract integer value from JSON string
static int extractInt(const std::string& value) {
    std::string trimmed = trim(value);
    return std::atoi(trimmed.c_str());
}

// Find the value for a given key in a JSON object string
static std::string findValue(const std::string& json, const std::string& key, std::string& error) {
    // Look for "key": value pattern
    std::string searchKey = "\"" + key + "\"";
    size_t keyPos = json.find(searchKey);
    
    if (keyPos == std::string::npos) {
        error = "Missing required field: " + key;
        return "";
    }
    
    // Find the colon after the key
    size_t colonPos = json.find(':', keyPos);
    if (colonPos == std::string::npos) {
        error = "Invalid JSON format for key: " + key;
        return "";
    }
    
    // Find the start of the value (skip whitespace)
    size_t valueStart = colonPos + 1;
    while (valueStart < json.length() && std::isspace(static_cast<unsigned char>(json[valueStart]))) {
        valueStart++;
    }
    
    if (valueStart >= json.length()) {
        error = "No value found for key: " + key;
        return "";
    }
    
    // Determine value end based on type
    size_t valueEnd = valueStart;
    
    if (json[valueStart] == '"') {
        // String value - find closing quote
        valueEnd = valueStart + 1;
        while (valueEnd < json.length() && json[valueEnd] != '"') {
            if (json[valueEnd] == '\\') valueEnd++; // Skip escaped characters
            valueEnd++;
        }
        if (valueEnd < json.length()) valueEnd++; // Include closing quote
    } else if (json[valueStart] == '[') {
        // Array value - find closing bracket
        int depth = 1;
        valueEnd = valueStart + 1;
        while (valueEnd < json.length() && depth > 0) {
            if (json[valueEnd] == '[') depth++;
            else if (json[valueEnd] == ']') depth--;
            valueEnd++;
        }
    } else if (json[valueStart] == '{') {
        // Object value - find closing brace
        int depth = 1;
        valueEnd = valueStart + 1;
        while (valueEnd < json.length() && depth > 0) {
            if (json[valueEnd] == '{') depth++;
            else if (json[valueEnd] == '}') depth--;
            valueEnd++;
        }
    } else {
        // Number or boolean - find comma or closing brace
        while (valueEnd < json.length() && 
               json[valueEnd] != ',' && 
               json[valueEnd] != '}' && 
               json[valueEnd] != ']') {
            valueEnd++;
        }
    }
    
    return json.substr(valueStart, valueEnd - valueStart);
}

// Parse an array of input objects from JSON
static bool parseInputArray(const std::string& arrayJson, 
                           std::vector<MappingLoader::InputMapping>& inputs,
                           std::string& error) {
    inputs.clear();
    
    // Find all objects in the array
    size_t pos = 0;
    while (pos < arrayJson.length()) {
        // Find next object start
        size_t objStart = arrayJson.find('{', pos);
        if (objStart == std::string::npos) break;
        
        // Find matching closing brace
        int depth = 1;
        size_t objEnd = objStart + 1;
        while (objEnd < arrayJson.length() && depth > 0) {
            if (arrayJson[objEnd] == '{') depth++;
            else if (arrayJson[objEnd] == '}') depth--;
            objEnd++;
        }
        
        if (depth != 0) {
            error = "Malformed JSON: unmatched braces in inputs array";
            return false;
        }
        
        // Parse this object
        std::string objJson = arrayJson.substr(objStart, objEnd - objStart);
        MappingLoader::InputMapping input;
        
        std::string tempError;
        std::string indexStr = findValue(objJson, "index", tempError);
        if (!tempError.empty()) {
            error = "Input object missing 'index': " + tempError;
            return false;
        }
        input.interface_index = extractInt(indexStr);
        
        std::string nameStr = findValue(objJson, "name", tempError);
        if (!tempError.empty()) {
            error = "Input object missing 'name': " + tempError;
            return false;
        }
        input.name = extractString(nameStr);
        
        std::string typeStr = findValue(objJson, "object_type", tempError);
        if (!tempError.empty()) {
            error = "Input object missing 'object_type': " + tempError;
            return false;
        }
        input.object_type = extractString(typeStr);
        
        std::string propStr = findValue(objJson, "property", tempError);
        if (!tempError.empty()) {
            error = "Input object missing 'property': " + tempError;
            return false;
        }
        input.property = extractString(propStr);
        
        // swmm_index is optional, defaults to -1
        input.swmm_index = -1;
        
        inputs.push_back(input);
        pos = objEnd;
    }
    
    return true;
}

// Parse an array of output objects from JSON
static bool parseOutputArray(const std::string& arrayJson, 
                            std::vector<MappingLoader::OutputMapping>& outputs,
                            std::string& error) {
    outputs.clear();
    
    // Find all objects in the array
    size_t pos = 0;
    while (pos < arrayJson.length()) {
        // Find next object start
        size_t objStart = arrayJson.find('{', pos);
        if (objStart == std::string::npos) break;
        
        // Find matching closing brace
        int depth = 1;
        size_t objEnd = objStart + 1;
        while (objEnd < arrayJson.length() && depth > 0) {
            if (arrayJson[objEnd] == '{') depth++;
            else if (arrayJson[objEnd] == '}') depth--;
            objEnd++;
        }
        
        if (depth != 0) {
            error = "Malformed JSON: unmatched braces in outputs array";
            return false;
        }
        
        // Parse this object
        std::string objJson = arrayJson.substr(objStart, objEnd - objStart);
        MappingLoader::OutputMapping output;
        
        std::string tempError;
        std::string indexStr = findValue(objJson, "index", tempError);
        if (!tempError.empty()) {
            error = "Output object missing 'index': " + tempError;
            return false;
        }
        output.interface_index = extractInt(indexStr);
        
        std::string nameStr = findValue(objJson, "name", tempError);
        if (!tempError.empty()) {
            error = "Output object missing 'name': " + tempError;
            return false;
        }
        output.name = extractString(nameStr);
        
        std::string typeStr = findValue(objJson, "object_type", tempError);
        if (!tempError.empty()) {
            error = "Output object missing 'object_type': " + tempError;
            return false;
        }
        output.object_type = extractString(typeStr);
        
        std::string propStr = findValue(objJson, "property", tempError);
        if (!tempError.empty()) {
            error = "Output object missing 'property': " + tempError;
            return false;
        }
        output.property = extractString(propStr);
        
        // swmm_index field
        std::string swmmIdxStr = findValue(objJson, "swmm_index", tempError);
        if (!tempError.empty()) {
            error = "Output object missing 'swmm_index': " + tempError;
            return false;
        }
        output.swmm_index = extractInt(swmmIdxStr);
        
        outputs.push_back(output);
        pos = objEnd;
    }
    
    return true;
}

//=============================================================================
// Constructor / Destructor
//=============================================================================

MappingLoader::MappingLoader()
    : inp_hash_("")
{
}

MappingLoader::~MappingLoader() {
}

//=============================================================================
// LoadFromFile - Load mapping from JSON file
//=============================================================================

bool MappingLoader::LoadFromFile(const std::string& path, std::string& error) {
    // Clear any existing data
    inputs_.clear();
    outputs_.clear();
    inp_hash_.clear();
    
    // Open and read the file
    std::ifstream file(path);
    if (!file.is_open()) {
        error = "Error: Mapping file not found\n"
                "Context: File path '" + path + "'\n" +
                "Suggestion: Ensure the mapping file exists and is accessible";
        return false;
    }
    
    // Read entire file into string
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string json = buffer.str();
    file.close();
    
    if (json.empty()) {
        error = "Error: Mapping file is empty\n"
                "Context: File path '" + path + "'\n" +
                "Suggestion: Ensure the mapping file contains valid JSON content";
        return false;
    }
    
    // Basic JSON validation - must start with { and end with }
    std::string trimmedJson = trim(json);
    if (trimmedJson.empty() || trimmedJson[0] != '{' || trimmedJson[trimmedJson.length() - 1] != '}') {
        error = "Error: Invalid mapping file format\n"
                "Context: File '" + path + "'\n" +
                "Suggestion: Ensure the file contains a valid JSON object (starts with '{' and ends with '}')";
        return false;
    }
    
    // Parse version
    std::string versionStr = findValue(json, "version", error);
    if (!error.empty()) {
        return false;
    }
    std::string version = extractString(versionStr);
    if (version != "1.0") {
        error = "Error: Unsupported mapping file version\n"
                "Context: Version '" + version + "' in file '" + path + "'\n" +
                "Suggestion: Regenerate the mapping file using the current version of the parser";
        return false;
    }
    
    // Parse inp_file_hash
    std::string hashStr = findValue(json, "inp_file_hash", error);
    if (!error.empty()) {
        return false;
    }
    inp_hash_ = extractString(hashStr);
    
    // Parse input_count
    std::string inputCountStr = findValue(json, "input_count", error);
    if (!error.empty()) {
        return false;
    }
    int expectedInputCount = extractInt(inputCountStr);
    
    // Parse output_count
    std::string outputCountStr = findValue(json, "output_count", error);
    if (!error.empty()) {
        return false;
    }
    int expectedOutputCount = extractInt(outputCountStr);
    
    // Parse inputs array
    std::string inputsArrayStr = findValue(json, "inputs", error);
    if (!error.empty()) {
        return false;
    }
    
    if (!parseInputArray(inputsArrayStr, inputs_, error)) {
        return false;
    }
    
    // Validate input count matches
    if (static_cast<int>(inputs_.size()) != expectedInputCount) {
        error = "Error: Input count mismatch\n"
                "Context: Expected " + std::to_string(expectedInputCount) + 
                " inputs, found " + std::to_string(inputs_.size()) + "\n" +
                "Suggestion: Regenerate the mapping file to ensure consistency";
        return false;
    }
    
    // Parse outputs array
    std::string outputsArrayStr = findValue(json, "outputs", error);
    if (!error.empty()) {
        return false;
    }
    
    if (!parseOutputArray(outputsArrayStr, outputs_, error)) {
        return false;
    }
    
    // Validate output count matches
    if (static_cast<int>(outputs_.size()) != expectedOutputCount) {
        error = "Error: Output count mismatch\n"
                "Context: Expected " + std::to_string(expectedOutputCount) + 
                " outputs, found " + std::to_string(outputs_.size()) + "\n" +
                "Suggestion: Regenerate the mapping file to ensure consistency";
        return false;
    }
    
    return true;
}

//=============================================================================
// Getter Methods
//=============================================================================

int MappingLoader::GetInputCount() const {
    return static_cast<int>(inputs_.size());
}

int MappingLoader::GetOutputCount() const {
    return static_cast<int>(outputs_.size());
}

const std::vector<MappingLoader::InputMapping>& MappingLoader::GetInputs() const {
    return inputs_;
}

const std::vector<MappingLoader::OutputMapping>& MappingLoader::GetOutputs() const {
    return outputs_;
}

const std::string& MappingLoader::GetHash() const {
    return inp_hash_;
}
