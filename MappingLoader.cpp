//-----------------------------------------------------------------------------
//   MappingLoader.cpp
//   Loads interface mapping from JSON configuration file
//-----------------------------------------------------------------------------

#include "include/MappingLoader.h"
#include <fstream>
#include <sstream>
#include <cctype>

static std::string trim(const std::string& str) {
    size_t start = 0, end = str.length();
    while (start < end && std::isspace((unsigned char)str[start])) start++;
    while (end > start && std::isspace((unsigned char)str[end - 1])) end--;
    return str.substr(start, end - start);
}

static std::string extractString(const std::string& val) {
    std::string t = trim(val);
    if (t.length() >= 2 && t[0] == '"' && t[t.length() - 1] == '"')
        return t.substr(1, t.length() - 2);
    return t;
}

static int extractInt(const std::string& val) {
    return std::atoi(trim(val).c_str());
}

static std::string findValue(const std::string& json, const std::string& key, std::string& error) {
    std::string searchKey = "\"" + key + "\"";
    size_t keyPos = json.find(searchKey);
    if (keyPos == std::string::npos) { error = "Missing: " + key; return ""; }
    
    size_t colonPos = json.find(':', keyPos);
    if (colonPos == std::string::npos) { error = "Invalid JSON for: " + key; return ""; }
    
    size_t valueStart = colonPos + 1;
    while (valueStart < json.length() && std::isspace((unsigned char)json[valueStart])) valueStart++;
    if (valueStart >= json.length()) { error = "No value for: " + key; return ""; }
    
    size_t valueEnd = valueStart;
    if (json[valueStart] == '"') {
        valueEnd = valueStart + 1;
        while (valueEnd < json.length() && json[valueEnd] != '"') {
            if (json[valueEnd] == '\\') valueEnd++;
            valueEnd++;
        }
        if (valueEnd < json.length()) valueEnd++;
    } else if (json[valueStart] == '[') {
        int depth = 1;
        valueEnd = valueStart + 1;
        while (valueEnd < json.length() && depth > 0) {
            if (json[valueEnd] == '[') depth++;
            else if (json[valueEnd] == ']') depth--;
            valueEnd++;
        }
    } else {
        while (valueEnd < json.length() && json[valueEnd] != ',' && json[valueEnd] != '}' && json[valueEnd] != ']')
            valueEnd++;
    }
    return json.substr(valueStart, valueEnd - valueStart);
}

template<typename T>
static bool parseArray(const std::string& arrayJson, std::vector<T>& items, std::string& error) {
    items.clear();
    size_t pos = 0;
    while (pos < arrayJson.length()) {
        size_t objStart = arrayJson.find('{', pos);
        if (objStart == std::string::npos) break;
        
        int depth = 1;
        size_t objEnd = objStart + 1;
        while (objEnd < arrayJson.length() && depth > 0) {
            if (arrayJson[objEnd] == '{') depth++;
            else if (arrayJson[objEnd] == '}') depth--;
            objEnd++;
        }
        if (depth != 0) { error = "Malformed JSON"; return false; }
        
        std::string objJson = arrayJson.substr(objStart, objEnd - objStart);
        T item;
        std::string err;
        
        item.interface_index = extractInt(findValue(objJson, "index", err));
        if (!err.empty()) { error = err; return false; }
        
        item.name = extractString(findValue(objJson, "name", err));
        if (!err.empty()) { error = err; return false; }
        
        item.object_type = extractString(findValue(objJson, "object_type", err));
        if (!err.empty()) { error = err; return false; }
        
        item.property = extractString(findValue(objJson, "property", err));
        if (!err.empty()) { error = err; return false; }
        
        item.swmm_index = -1;
        items.push_back(item);
        pos = objEnd;
    }
    return true;
}

MappingLoader::MappingLoader() : logging_level_("INFO") {}
MappingLoader::~MappingLoader() {}

bool MappingLoader::LoadFromFile(const std::string& path, std::string& error) {
    inputs_.clear();
    outputs_.clear();
    logging_level_ = "INFO";  // Default
    
    std::ifstream file(path);
    if (!file.is_open()) {
        error = "File not found: " + path + "\nRun: python generate_mapping.py model.inp";
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string json = buffer.str();
    file.close();
    
    if (json.empty()) { error = "Empty file: " + path; return false; }
    
    std::string t = trim(json);
    if (t.empty() || t[0] != '{' || t[t.length() - 1] != '}') {
        error = "Invalid JSON in: " + path;
        return false;
    }
    
    // Parse version
    std::string version = extractString(findValue(json, "version", error));
    if (!error.empty()) return false;
    if (version != "1.0") { error = "Unsupported version: " + version; return false; }
    
    // Parse inputs
    std::string inputsStr = findValue(json, "inputs", error);
    if (!error.empty()) return false;
    if (!parseArray(inputsStr, inputs_, error)) return false;
    
    // Parse outputs
    std::string outputsStr = findValue(json, "outputs", error);
    if (!error.empty()) return false;
    if (!parseArray(outputsStr, outputs_, error)) return false;
    
    // Parse logging_level (optional)
    std::string loggingStr = findValue(json, "logging_level", error);
    if (error.empty()) {
        logging_level_ = extractString(loggingStr);
    } else {
        logging_level_ = "INFO";  // Default if not specified
        error.clear();  // Clear error since it's optional
    }
    
    return true;
}

int MappingLoader::GetInputCount() const { return (int)inputs_.size(); }
int MappingLoader::GetOutputCount() const { return (int)outputs_.size(); }
const std::vector<MappingLoader::InputMapping>& MappingLoader::GetInputs() const { return inputs_; }
const std::vector<MappingLoader::OutputMapping>& MappingLoader::GetOutputs() const { return outputs_; }
const std::string& MappingLoader::GetLoggingLevel() const { return logging_level_; }
