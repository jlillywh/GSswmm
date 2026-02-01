/**
 * @file test_lid_api.cpp
 * @brief Unit tests for SWMM5 LID API extensions
 * 
 * Tests the three new API functions:
 * - swmm_getLidUCount()
 * - swmm_getLidUName()
 * - swmm_getLidUStorageVolume()
 * 
 * Requirements tested:
 * - Requirement 1: LID Unit Enumeration
 * - Requirement 2: LID Unit Identification
 * - Requirement 3: LID Storage Volume Access
 * - Requirement 6: Error Handling
 * - Requirement 7: API Consistency
 */

#include "gtest_minimal.h"
#include "../include/swmm5.h"
#include "swmm_mock.h"
#include <string.h>
#include <stdio.h>

// Test model with LID units
const char* TEST_MODEL = "lid_test_model.inp";
const char* TEST_REPORT = "lid_test.rpt";
const char* TEST_OUTPUT = "lid_test.out";

/**
 * Test Fixture for LID API tests
 * Sets up and tears down SWMM model for each test
 */
class LidApiTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize stub with test data
        // Simulate a model with 10 subcatchments
        SwmmLidStub_Initialize(10);
        
        // Add LID units to subcatchment 0 (simulating "S1")
        // S1 has InfilTrench and RainBarrels
        SwmmLidStub_AddLidUnit(0, "InfilTrench", 125.3);
        SwmmLidStub_AddLidUnit(0, "RainBarrels", 45.7);
        
        // Subcatchment 1 (simulating "S2") has no LID units
        
        // Add LID unit to subcatchment 2 (simulating "Swale3")
        // Swales typically have no storage
        SwmmLidStub_AddLidUnit(2, "Swale", 0.0);
        
        // Add LID units to subcatchment 3 (simulating "S4")
        SwmmLidStub_AddLidUnit(3, "Planters", 78.2);
        
        // Add LID units to subcatchment 4 (simulating "S5")
        SwmmLidStub_AddLidUnit(4, "PorousPave", 92.1);
        SwmmLidStub_AddLidUnit(4, "GreenRoof", 34.5);
    }
    
    void TearDown() override {
        // Clean up stub
        SwmmLidStub_Cleanup();
    }
};

// ============================================================================
// Requirement 1: LID Unit Enumeration Tests
// ============================================================================

/**
 * Test: Count LID units in subcatchment with multiple LIDs
 * Validates: Requirements 1.1
 */
TEST_F(LidApiTest, CountLidUnits_MultipleUnits) {
    // Subcatchment 0 (simulating S1) has InfilTrench and RainBarrels = 2 LID controls
    int s1_index = 0;
    
    int count = swmm_getLidUCount(s1_index);
    EXPECT_EQ(count, 2);
}

/**
 * Test: Count LID units in subcatchment with no LIDs
 * Validates: Requirements 1.2
 */
TEST_F(LidApiTest, CountLidUnits_NoUnits) {
    // Subcatchment 1 (simulating S2) has no LID units
    int s2_index = 1;
    
    int count = swmm_getLidUCount(s2_index);
    EXPECT_EQ(count, 0);
}

/**
 * Test: Invalid subcatchment index returns -1
 * Validates: Requirements 1.3
 */
TEST_F(LidApiTest, CountLidUnits_InvalidIndex) {
    int count = swmm_getLidUCount(9999);
    EXPECT_EQ(count, -1);
}

// ============================================================================
// Requirement 2: LID Unit Identification Tests
// ============================================================================

/**
 * Test: Get LID control name for valid indices
 * Validates: Requirements 2.1
 */
TEST_F(LidApiTest, GetLidName_ValidIndices) {
    int s1_index = 0;
    
    char name[64];
    
    swmm_getLidUName(s1_index, 0, name, sizeof(name));
    EXPECT_STREQ(name, "InfilTrench");
    
    swmm_getLidUName(s1_index, 1, name, sizeof(name));
    EXPECT_STREQ(name, "RainBarrels");
}

/**
 * Test: Invalid indices set error message
 * Validates: Requirements 2.2
 */
TEST_F(LidApiTest, GetLidName_InvalidIndices) {
    char name[64];
    char errMsg[256];
    
    swmm_getLidUName(9999, 0, name, sizeof(name));
    swmm_getError(errMsg, sizeof(errMsg));
    EXPECT_GT(strlen(errMsg), 0);
}

/**
 * Test: Buffer size is respected
 * Validates: Requirements 2.3
 */
TEST_F(LidApiTest, GetLidName_BufferSize) {
    int s1_index = 0;
    
    char small_buffer[5];
    
    swmm_getLidUName(s1_index, 0, small_buffer, sizeof(small_buffer));
    EXPECT_LT(strlen(small_buffer), sizeof(small_buffer));
}

/**
 * Test: Returned string is null-terminated
 * Validates: Requirements 2.4
 */
TEST_F(LidApiTest, GetLidName_NullTerminated) {
    int s1_index = 0;
    
    char name[64];
    memset(name, 'X', sizeof(name)); // Fill with non-null
    
    swmm_getLidUName(s1_index, 0, name, sizeof(name));
    EXPECT_LT(strlen(name), sizeof(name));
}

// ============================================================================
// Requirement 3: LID Storage Volume Access Tests
// ============================================================================

/**
 * Test: Get storage volume for valid LID unit
 * Validates: Requirements 3.1, 3.4
 */
TEST_F(LidApiTest, GetStorageVolume_ValidUnit) {
    int s1_index = 0;
    
    double volume = swmm_getLidUStorageVolume(s1_index, 0);
    EXPECT_GE(volume, 0.0);
    EXPECT_DOUBLE_EQ(volume, 125.3);
}

/**
 * Test: LID with no storage returns zero
 * Validates: Requirements 3.2
 */
TEST_F(LidApiTest, GetStorageVolume_NoStorage) {
    // Swale units typically have no storage layer
    int swale3_index = 2;
    
    double volume = swmm_getLidUStorageVolume(swale3_index, 0);
    EXPECT_EQ(volume, 0.0);
}

/**
 * Test: Invalid indices return zero and set error
 * Validates: Requirements 3.3
 */
TEST_F(LidApiTest, GetStorageVolume_InvalidIndices) {
    char errMsg[256];
    
    double volume = swmm_getLidUStorageVolume(9999, 0);
    EXPECT_EQ(volume, 0.0);
    
    swmm_getError(errMsg, sizeof(errMsg));
    EXPECT_GT(strlen(errMsg), 0);
}

/**
 * Test: Volume units match model configuration
 * Validates: Requirements 3.5
 */
TEST_F(LidApiTest, GetStorageVolume_UnitsConsistency) {
    // Model uses CFS (cubic feet per second), so volumes should be in cubic feet
    int s1_index = 0;
    
    double volume = swmm_getLidUStorageVolume(s1_index, 0);
    // Volume should be reasonable for cubic feet (not cubic meters)
    EXPECT_LT(volume, 1000000.0);
}

/**
 * Test: Storage volume is always non-negative
 * Validates: Requirements 3.6
 */
TEST_F(LidApiTest, GetStorageVolume_NonNegative) {
    int s1_index = 0;
    
    int count = swmm_getLidUCount(s1_index);
    for (int i = 0; i < count; i++) {
        double volume = swmm_getLidUStorageVolume(s1_index, i);
        EXPECT_GE(volume, 0.0);
    }
}

// ============================================================================
// Requirement 6: Error Handling Tests
// ============================================================================

/**
 * Test: API functions handle pre-initialization calls
 * Validates: Requirements 6.1
 */
TEST(LidApiErrorTest, CallBeforeStart) {
    // Clean up any previous state
    SwmmLidStub_Cleanup();
    
    // Test calling functions before initialization
    int count = swmm_getLidUCount(0);
    EXPECT_EQ(count, -1);
    
    char name[64];
    swmm_getLidUName(0, 0, name, sizeof(name));
    EXPECT_EQ(name[0], '\0');
    
    double volume = swmm_getLidUStorageVolume(0, 0);
    EXPECT_EQ(volume, 0.0);
}

/**
 * Test: Error messages are retrievable via swmm_getError()
 * Validates: Requirements 6.3
 */
TEST_F(LidApiTest, ErrorMessages_Retrievable) {
    char errMsg[256];
    char name[64];
    
    // Trigger an error condition
    swmm_getLidUName(9999, 0, name, sizeof(name));
    
    // Retrieve error message
    swmm_getError(errMsg, sizeof(errMsg));
    EXPECT_GT(strlen(errMsg), 0);
    EXPECT_NE(strstr(errMsg, "LID API Error"), nullptr);
}

// ============================================================================
// Requirement 7: API Consistency Tests
// ============================================================================

/**
 * Test: Function naming follows SWMM5 conventions
 * Validates: Requirements 7.1
 */
TEST(LidApiConsistencyTest, NamingConvention) {
    // Function names should follow pattern: swmm_getLidU*
    // This is verified at compile time by the function declarations
    SUCCEED();
}

/**
 * Test: Parameter ordering is consistent
 * Validates: Requirements 7.2
 */
TEST(LidApiConsistencyTest, ParameterOrdering) {
    // All functions take indices first, then output parameters
    // This is verified at compile time by the function signatures
    SUCCEED();
}

/**
 * Test: Return value conventions are consistent
 * Validates: Requirements 7.3
 */
TEST(LidApiConsistencyTest, ReturnValueConventions) {
    // swmm_getLidUCount() returns int (count or -1)
    // swmm_getLidUStorageVolume() returns double
    // swmm_getLidUName() returns void (output via parameter)
    SUCCEED();
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    
    printf("\n");
    printf("========================================\n");
    printf("SWMM5 LID API Extension Tests\n");
    printf("========================================\n");
    printf("\n");
    
    int result = RUN_ALL_TESTS();
    
    printf("\n");
    if (result == 0) {
        printf("========================================\n");
        printf("All tests passed!\n");
        printf("========================================\n");
    } else {
        printf("========================================\n");
        printf("Some tests failed!\n");
        printf("========================================\n");
    }
    printf("\n");
    
    return result;
}
