//-----------------------------------------------------------------------------
//   gtest_minimal.h
//
//   Minimal Google Test-compatible framework for unit testing
//   Provides basic TEST macro and assertion macros
//-----------------------------------------------------------------------------

#ifndef GTEST_MINIMAL_H
#define GTEST_MINIMAL_H

#include <iostream>
#include <string>
#include <vector>
#include <sstream>

//-----------------------------------------------------------------------------
// Test Registry
//-----------------------------------------------------------------------------
struct TestInfo {
    std::string test_suite_name;
    std::string test_name;
    void (*test_func)();
};

class TestRegistry {
public:
    static TestRegistry& Instance() {
        static TestRegistry instance;
        return instance;
    }
    
    void RegisterTest(const std::string& suite_name, const std::string& test_name, void (*func)()) {
        TestInfo info;
        info.test_suite_name = suite_name;
        info.test_name = test_name;
        info.test_func = func;
        tests.push_back(info);
    }
    
    int RunAllTests() {
        int total = 0;
        int passed = 0;
        int failed = 0;
        
        std::cout << "[==========] Running " << tests.size() << " tests." << std::endl;
        
        for (size_t i = 0; i < tests.size(); i++) {
            const TestInfo& test = tests[i];
            total++;
            
            std::cout << "[ RUN      ] " << test.test_suite_name << "." << test.test_name << std::endl;
            
            current_test_failed = false;
            current_test_name = test.test_suite_name + "." + test.test_name;
            
            try {
                test.test_func();
                
                if (!current_test_failed) {
                    std::cout << "[       OK ] " << test.test_suite_name << "." << test.test_name << std::endl;
                    passed++;
                } else {
                    std::cout << "[  FAILED  ] " << test.test_suite_name << "." << test.test_name << std::endl;
                    failed++;
                }
            } catch (const std::exception& e) {
                std::cout << "[  FAILED  ] " << test.test_suite_name << "." << test.test_name << std::endl;
                std::cout << "           Exception: " << e.what() << std::endl;
                failed++;
            }
        }
        
        std::cout << "[==========] " << total << " tests ran." << std::endl;
        std::cout << "[  PASSED  ] " << passed << " tests." << std::endl;
        
        if (failed > 0) {
            std::cout << "[  FAILED  ] " << failed << " tests." << std::endl;
        }
        
        return failed;
    }
    
    void RecordFailure(const std::string& file, int line, const std::string& message) {
        current_test_failed = true;
        std::cout << file << ":" << line << ": Failure" << std::endl;
        std::cout << message << std::endl;
    }
    
private:
    std::vector<TestInfo> tests;
    bool current_test_failed;
    std::string current_test_name;
};

//-----------------------------------------------------------------------------
// Test Registration Helper
//-----------------------------------------------------------------------------
class TestRegistrar {
public:
    TestRegistrar(const std::string& suite_name, const std::string& test_name, void (*func)()) {
        TestRegistry::Instance().RegisterTest(suite_name, test_name, func);
    }
};

//-----------------------------------------------------------------------------
// Assertion Macros
//-----------------------------------------------------------------------------
#define EXPECT_EQ(val1, val2) \
    do { \
        auto v1 = (val1); \
        auto v2 = (val2); \
        if (v1 != v2) { \
            std::ostringstream oss; \
            oss << "Expected: " << #val1 << " == " << #val2 << std::endl; \
            oss << "  Actual: " << v1 << " vs " << v2; \
            TestRegistry::Instance().RecordFailure(__FILE__, __LINE__, oss.str()); \
        } \
    } while (0)

#define EXPECT_NE(val1, val2) \
    do { \
        auto v1 = (val1); \
        auto v2 = (val2); \
        if (v1 == v2) { \
            std::ostringstream oss; \
            oss << "Expected: " << #val1 << " != " << #val2 << std::endl; \
            oss << "  Actual: " << v1 << " vs " << v2; \
            TestRegistry::Instance().RecordFailure(__FILE__, __LINE__, oss.str()); \
        } \
    } while (0)

#define EXPECT_DOUBLE_EQ(val1, val2) \
    do { \
        double v1 = (val1); \
        double v2 = (val2); \
        if (v1 != v2) { \
            std::ostringstream oss; \
            oss << "Expected: " << #val1 << " == " << #val2 << std::endl; \
            oss << "  Actual: " << v1 << " vs " << v2; \
            TestRegistry::Instance().RecordFailure(__FILE__, __LINE__, oss.str()); \
        } \
    } while (0)

#define EXPECT_TRUE(condition) \
    do { \
        if (!(condition)) { \
            std::ostringstream oss; \
            oss << "Expected: " << #condition << " is true" << std::endl; \
            oss << "  Actual: false"; \
            TestRegistry::Instance().RecordFailure(__FILE__, __LINE__, oss.str()); \
        } \
    } while (0)

#define EXPECT_FALSE(condition) \
    do { \
        if (condition) { \
            std::ostringstream oss; \
            oss << "Expected: " << #condition << " is false" << std::endl; \
            oss << "  Actual: true"; \
            TestRegistry::Instance().RecordFailure(__FILE__, __LINE__, oss.str()); \
        } \
    } while (0)

#define EXPECT_LT(val1, val2) \
    do { \
        auto v1 = (val1); \
        auto v2 = (val2); \
        if (!(v1 < v2)) { \
            std::ostringstream oss; \
            oss << "Expected: " << #val1 << " < " << #val2 << std::endl; \
            oss << "  Actual: " << v1 << " vs " << v2; \
            TestRegistry::Instance().RecordFailure(__FILE__, __LINE__, oss.str()); \
        } \
    } while (0)

#define EXPECT_GT(val1, val2) \
    do { \
        auto v1 = (val1); \
        auto v2 = (val2); \
        if (!(v1 > v2)) { \
            std::ostringstream oss; \
            oss << "Expected: " << #val1 << " > " << #val2 << std::endl; \
            oss << "  Actual: " << v1 << " vs " << v2; \
            TestRegistry::Instance().RecordFailure(__FILE__, __LINE__, oss.str()); \
        } \
    } while (0)

#define EXPECT_STREQ(str1, str2) \
    do { \
        const char* s1 = (str1); \
        const char* s2 = (str2); \
        if (strcmp(s1, s2) != 0) { \
            std::ostringstream oss; \
            oss << "Expected: " << #str1 << " == " << #str2 << std::endl; \
            oss << "  Actual: \"" << s1 << "\" vs \"" << s2 << "\""; \
            TestRegistry::Instance().RecordFailure(__FILE__, __LINE__, oss.str()); \
        } \
    } while (0)

#define ASSERT_EQ(val1, val2) \
    do { \
        auto v1 = (val1); \
        auto v2 = (val2); \
        if (v1 != v2) { \
            std::ostringstream oss; \
            oss << "Expected: " << #val1 << " == " << #val2 << std::endl; \
            oss << "  Actual: " << v1 << " vs " << v2; \
            TestRegistry::Instance().RecordFailure(__FILE__, __LINE__, oss.str()); \
            return; \
        } \
    } while (0)

#define ASSERT_TRUE(condition) \
    do { \
        if (!(condition)) { \
            std::ostringstream oss; \
            oss << "Expected: " << #condition << " is true" << std::endl; \
            oss << "  Actual: false"; \
            TestRegistry::Instance().RecordFailure(__FILE__, __LINE__, oss.str()); \
            return; \
        } \
    } while (0)

#define ASSERT_GE(val1, val2) \
    do { \
        auto v1 = (val1); \
        auto v2 = (val2); \
        if (!(v1 >= v2)) { \
            std::ostringstream oss; \
            oss << "Expected: " << #val1 << " >= " << #val2 << std::endl; \
            oss << "  Actual: " << v1 << " vs " << v2; \
            TestRegistry::Instance().RecordFailure(__FILE__, __LINE__, oss.str()); \
            return; \
        } \
    } while (0)

#define EXPECT_GE(val1, val2) \
    do { \
        auto v1 = (val1); \
        auto v2 = (val2); \
        if (!(v1 >= v2)) { \
            std::ostringstream oss; \
            oss << "Expected: " << #val1 << " >= " << #val2 << std::endl; \
            oss << "  Actual: " << v1 << " vs " << v2; \
            TestRegistry::Instance().RecordFailure(__FILE__, __LINE__, oss.str()); \
        } \
    } while (0)

#define EXPECT_LE(val1, val2) \
    do { \
        auto v1 = (val1); \
        auto v2 = (val2); \
        if (!(v1 <= v2)) { \
            std::ostringstream oss; \
            oss << "Expected: " << #val1 << " <= " << #val2 << std::endl; \
            oss << "  Actual: " << v1 << " vs " << v2; \
            TestRegistry::Instance().RecordFailure(__FILE__, __LINE__, oss.str()); \
        } \
    } while (0)

#define SUCCEED() \
    do { \
        /* Test passes - no action needed */ \
    } while (0)

//-----------------------------------------------------------------------------
// Test Fixture Support
//-----------------------------------------------------------------------------
namespace testing {
    class Test {
    public:
        virtual ~Test() {}
        virtual void SetUp() {}
        virtual void TearDown() {}
    };
}

//-----------------------------------------------------------------------------
// Test Definition Macros
//-----------------------------------------------------------------------------
#define TEST(test_suite_name, test_name) \
    void test_suite_name##_##test_name##_TestBody(); \
    TestRegistrar test_suite_name##_##test_name##_registrar( \
        #test_suite_name, #test_name, test_suite_name##_##test_name##_TestBody); \
    void test_suite_name##_##test_name##_TestBody()

#define TEST_F(test_fixture, test_name) \
    class test_fixture##_##test_name##_Test : public test_fixture { \
    public: \
        test_fixture##_##test_name##_Test() {} \
        void RunTest() { \
            this->SetUp(); \
            this->TestBody(); \
            this->TearDown(); \
        } \
        void TestBody(); \
    }; \
    void test_fixture##_##test_name##_TestFunc() { \
        test_fixture##_##test_name##_Test test; \
        test.RunTest(); \
    } \
    TestRegistrar test_fixture##_##test_name##_registrar( \
        #test_fixture, #test_name, test_fixture##_##test_name##_TestFunc); \
    void test_fixture##_##test_name##_Test::TestBody()

//-----------------------------------------------------------------------------
// Google Test Compatibility
//-----------------------------------------------------------------------------
namespace testing {
    inline void InitGoogleTest(int* argc, char** argv) {
        // Minimal implementation - just parse basic flags if needed
    }
}

//-----------------------------------------------------------------------------
// Main Test Runner
//-----------------------------------------------------------------------------
#define RUN_ALL_TESTS() TestRegistry::Instance().RunAllTests()

#endif // GTEST_MINIMAL_H
