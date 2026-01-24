//-----------------------------------------------------------------------------
//   test_json_parsing.cpp
//
//   Unit tests for MappingLoader JSON parsing functionality
//   Tests LoadFromFile method with various JSON inputs
//-----------------------------------------------------------------------------

#include "../include/MappingLoader.h"
#include "gtest_minimal.h"
#include <iostream>
#include <fstream>

//=============================================================================
// Helper: Create a temporary JSON file for testing
//=============================================================================
void createTestJsonFile(const std::string& filename, const std::string& content) {
    std::ofstream file(filename);
    file << content;
    file.close();
}

//=============================================================================
// Test: Load valid JSON mapping file
//=============================================================================
void test_load_valid_json() {
    std::string testFile = "test_valid_mapping.json";
    std::string jsonContent = R"({
  "version": "1.0",
  "inp_file_hash": "abc123def456",
  "input_count": 2,
  "output_count": 3,
  "inputs": [
    {
      "index": 0,
      "name": "ElapsedTime",
      "object_type": "SYSTEM",
      "property": "ELAPSEDTIME"
    },
    {
      "index": 1,
      "name": "RG1",
      "object_type": "GAGE",
      "property": "RAINFALL"
    }
  ],
  "outputs": [
    {
      "index": 0,
      "name": "POND1",
      "object_type": "STORAGE",
      "property": "VOLUME",
      "swmm_index": 0
    },
    {
      "index": 1,
      "name": "OUT1",
      "object_type": "OUTFALL",
      "property": "FLOW",
      "swmm_index": 0
    },
    {
      "index": 2,
      "name": "S1",
      "object_type": "SUBCATCH",
      "property": "RUNOFF",
      "swmm_index": 0
    }
  ]
})";
    
    createTestJsonFile(testFile, jsonContent);
    
    MappingLoader loader;
    std::string error;
    bool result = loader.LoadFromFile(testFile, error);
    
    ASSERT_TRUE(result);
    ASSERT_TRUE(error.empty());
    ASSERT_EQ(loader.GetInputCount(), 2);
    ASSERT_EQ(loader.GetOutputCount(), 3);
    ASSERT_EQ(loader.GetHash(), "abc123def456");
    
    // Verify input mappings
    const auto& inputs = loader.GetInputs();
    ASSERT_EQ(inputs.size(), 2);
    ASSERT_EQ(inputs[0].interface_index, 0);
    ASSERT_EQ(inputs[0].name, "ElapsedTime");
    ASSERT_EQ(inputs[0].object_type, "SYSTEM");
    ASSERT_EQ(inputs[0].property, "ELAPSEDTIME");
    
    ASSERT_EQ(inputs[1].interface_index, 1);
    ASSERT_EQ(inputs[1].name, "RG1");
    ASSERT_EQ(inputs[1].object_type, "GAGE");
    ASSERT_EQ(inputs[1].property, "RAINFALL");
    
    // Verify output mappings
    const auto& outputs = loader.GetOutputs();
    ASSERT_EQ(outputs.size(), 3);
    ASSERT_EQ(outputs[0].interface_index, 0);
    ASSERT_EQ(outputs[0].name, "POND1");
    ASSERT_EQ(outputs[0].object_type, "STORAGE");
    ASSERT_EQ(outputs[0].property, "VOLUME");
    ASSERT_EQ(outputs[0].swmm_index, 0);
    
    ASSERT_EQ(outputs[1].interface_index, 1);
    ASSERT_EQ(outputs[1].name, "OUT1");
    ASSERT_EQ(outputs[1].object_type, "OUTFALL");
    ASSERT_EQ(outputs[1].property, "FLOW");
    
    ASSERT_EQ(outputs[2].interface_index, 2);
    ASSERT_EQ(outputs[2].name, "S1");
    ASSERT_EQ(outputs[2].object_type, "SUBCATCH");
    ASSERT_EQ(outputs[2].property, "RUNOFF");
    
    std::remove(testFile.c_str());
    std::cout << "PASS: Load valid JSON" << std::endl;
}

//=============================================================================
// Test: File not found error
//=============================================================================
void test_file_not_found() {
    MappingLoader loader;
    std::string error;
    bool result = loader.LoadFromFile("nonexistent_file.json", error);
    
    ASSERT_FALSE(result);
    ASSERT_FALSE(error.empty());
    ASSERT_TRUE(error.find("not found") != std::string::npos);
    
    std::cout << "PASS: File not found error" << std::endl;
}

//=============================================================================
// Test: Invalid JSON format
//=============================================================================
void test_invalid_json_format() {
    std::string testFile = "test_invalid.json";
    std::string jsonContent = "This is not valid JSON";
    
    createTestJsonFile(testFile, jsonContent);
    
    MappingLoader loader;
    std::string error;
    bool result = loader.LoadFromFile(testFile, error);
    
    ASSERT_FALSE(result);
    ASSERT_FALSE(error.empty());
    ASSERT_TRUE(error.find("Invalid") != std::string::npos || 
                error.find("format") != std::string::npos);
    
    std::remove(testFile.c_str());
    std::cout << "PASS: Invalid JSON format" << std::endl;
}

//=============================================================================
// Test: Missing required field
//=============================================================================
void test_missing_required_field() {
    std::string testFile = "test_missing_field.json";
    std::string jsonContent = R"({
  "version": "1.0",
  "input_count": 1,
  "output_count": 1,
  "inputs": [],
  "outputs": []
})";
    
    createTestJsonFile(testFile, jsonContent);
    
    MappingLoader loader;
    std::string error;
    bool result = loader.LoadFromFile(testFile, error);
    
    ASSERT_FALSE(result);
    ASSERT_FALSE(error.empty());
    ASSERT_TRUE(error.find("inp_file_hash") != std::string::npos);
    
    std::remove(testFile.c_str());
    std::cout << "PASS: Missing required field" << std::endl;
}

//=============================================================================
// Test: Count mismatch
//=============================================================================
void test_count_mismatch() {
    std::string testFile = "test_count_mismatch.json";
    std::string jsonContent = R"({
  "version": "1.0",
  "inp_file_hash": "test123",
  "input_count": 5,
  "output_count": 1,
  "inputs": [
    {
      "index": 0,
      "name": "ElapsedTime",
      "object_type": "SYSTEM",
      "property": "ELAPSEDTIME"
    }
  ],
  "outputs": []
})";
    
    createTestJsonFile(testFile, jsonContent);
    
    MappingLoader loader;
    std::string error;
    bool result = loader.LoadFromFile(testFile, error);
    
    ASSERT_FALSE(result);
    ASSERT_FALSE(error.empty());
    ASSERT_TRUE(error.find("mismatch") != std::string::npos);
    
    std::remove(testFile.c_str());
    std::cout << "PASS: Count mismatch" << std::endl;
}

//=============================================================================
// Test: Empty file
//=============================================================================
void test_empty_file() {
    std::string testFile = "test_empty.json";
    createTestJsonFile(testFile, "");
    
    MappingLoader loader;
    std::string error;
    bool result = loader.LoadFromFile(testFile, error);
    
    ASSERT_FALSE(result);
    ASSERT_FALSE(error.empty());
    ASSERT_TRUE(error.find("empty") != std::string::npos);
    
    std::remove(testFile.c_str());
    std::cout << "PASS: Empty file" << std::endl;
}

//=============================================================================
// Test: Load actual SwmmGoldSimBridge.json
//=============================================================================
void test_load_actual_mapping_file() {
    MappingLoader loader;
    std::string error;
    bool result = loader.LoadFromFile("SwmmGoldSimBridge.json", error);
    
    if (!result) {
        std::cout << "SKIP: Actual mapping file test (file may not exist): " << error << std::endl;
        return;
    }
    
    ASSERT_TRUE(result);
    ASSERT_TRUE(error.empty());
    ASSERT_TRUE(loader.GetInputCount() >= 1); // At least elapsed time
    ASSERT_FALSE(loader.GetHash().empty());
    
    std::cout << "PASS: Load actual mapping file" << std::endl;
    std::cout << "  Inputs: " << loader.GetInputCount() << std::endl;
    std::cout << "  Outputs: " << loader.GetOutputCount() << std::endl;
    std::cout << "  Hash: " << loader.GetHash() << std::endl;
}

//=============================================================================
// Main
//=============================================================================
int main() {
    std::cout << "=== MappingLoader JSON Parsing Tests ===" << std::endl;
    
    test_load_valid_json();
    test_file_not_found();
    test_invalid_json_format();
    test_missing_required_field();
    test_count_mismatch();
    test_empty_file();
    test_load_actual_mapping_file();
    
    std::cout << "\nAll tests passed!" << std::endl;
    return 0;
}
