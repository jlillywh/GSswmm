//-----------------------------------------------------------------------------
//   swmm_mock.h
//
//   Mock implementation of SWMM API for unit testing
//   Provides call tracking, parameter verification, and configurable return values
//-----------------------------------------------------------------------------

#ifndef SWMM_MOCK_H
#define SWMM_MOCK_H

#include <string>
#include <vector>

//-----------------------------------------------------------------------------
// SWMM API Constants (from swmm5.h)
//-----------------------------------------------------------------------------
enum SM_ObjectType {
    swmm_GAGE = 0,
    swmm_SUBCATCH = 1,
    swmm_NODE = 2,
    swmm_LINK = 3,
    swmm_POLLUT = 4,
    swmm_LANDUSE = 5,
    swmm_TIMEPATTERN = 6,
    swmm_CURVE = 7,
    swmm_TSERIES = 8,
    swmm_CONTROL = 9,
    swmm_TRANSECT = 10,
    swmm_AQUIFER = 11,
    swmm_UNITHYD = 12,
    swmm_SNOWMELT = 13,
    swmm_SHAPE = 14,
    swmm_LID = 15
};

enum SM_SubcatchProperty {
    swmm_SUBCATCH_RAINFALL = 0,
    swmm_SUBCATCH_EVAP = 1,
    swmm_SUBCATCH_INFIL = 2,
    swmm_SUBCATCH_RUNOFF = 3,
    swmm_SUBCATCH_GW_FLOW = 4,
    swmm_SUBCATCH_GW_ELEV = 5,
    swmm_SUBCATCH_SOIL_MOIST = 6,
    swmm_SUBCATCH_WASHOFF = 7
};

//-----------------------------------------------------------------------------
// Mock State and Configuration
//-----------------------------------------------------------------------------
struct SwmmMockState {
    // Call tracking
    int open_call_count;
    int start_call_count;
    int step_call_count;
    int end_call_count;
    int close_call_count;
    int getValue_call_count;
    int setValue_call_count;
    int getError_call_count;
    int getCount_call_count;
    
    // Parameter tracking for last call
    std::string last_input_file;
    std::string last_report_file;
    std::string last_output_file;
    int last_start_save_flag;
    int last_getValue_type;
    int last_getValue_index;
    int last_setValue_type;
    int last_setValue_index;
    double last_setValue_value;
    double last_step_elapsed_time;
    int last_getCount_type;
    
    // Configurable return values
    int open_return_code;
    int start_return_code;
    int step_return_code;
    int end_return_code;
    int close_return_code;
    double getValue_return_value;
    std::string error_message;
    int getCount_return_value;
    
    // Step behavior configuration
    int step_calls_until_end;  // Return >0 after this many calls (0 = never end)
    int step_calls_until_error; // Return <0 after this many calls (0 = never error)
    
    // State flags
    bool is_opened;
    bool is_started;
};

//-----------------------------------------------------------------------------
// Global mock state instance
//-----------------------------------------------------------------------------
extern SwmmMockState g_mock_state;

//-----------------------------------------------------------------------------
// Mock Control Functions
//-----------------------------------------------------------------------------

// Reset all mock state to defaults
void SwmmMock_Reset();

// Configure mock to succeed for all operations
void SwmmMock_SetSuccessMode();

// Configure mock to fail at specific operation
void SwmmMock_SetOpenFailure(int error_code, const char* error_msg);
void SwmmMock_SetStartFailure(int error_code, const char* error_msg);
void SwmmMock_SetStepFailure(int error_code, const char* error_msg);
void SwmmMock_SetEndFailure(int error_code, const char* error_msg);
void SwmmMock_SetCloseFailure(int error_code, const char* error_msg);

// Configure step behavior
void SwmmMock_SetStepEndAfter(int num_calls);  // Simulation ends after N calls
void SwmmMock_SetStepErrorAfter(int num_calls); // Error occurs after N calls

// Configure getValue return value
void SwmmMock_SetGetValueReturn(double value);

// Configure getCount return value
void SwmmMock_SetGetCountReturn(int count);

// Get call counts for verification
int SwmmMock_GetOpenCallCount();
int SwmmMock_GetStartCallCount();
int SwmmMock_GetStepCallCount();
int SwmmMock_GetEndCallCount();
int SwmmMock_GetCloseCallCount();
int SwmmMock_GetValueCallCount();
int SwmmMock_GetSetValueCallCount();

// Get last call parameters for verification
const char* SwmmMock_GetLastInputFile();
const char* SwmmMock_GetLastReportFile();
const char* SwmmMock_GetLastOutputFile();
int SwmmMock_GetLastStartSaveFlag();
int SwmmMock_GetLastGetValueType();
int SwmmMock_GetLastGetValueIndex();
int SwmmMock_GetLastSetValueType();
int SwmmMock_GetLastSetValueIndex();
double SwmmMock_GetLastSetValueValue();

//-----------------------------------------------------------------------------
// Mock SWMM API Functions
//-----------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif

int swmm_open(const char* f1, const char* f2, const char* f3);
int swmm_start(int saveFlag);
int swmm_step(double* elapsedTime);
int swmm_end();
int swmm_close();
void swmm_setValue(int type, int index, double value);
double swmm_getValue(int type, int index);
int swmm_getError(char* errMsg, int msgLen);
int swmm_getCount(int objType);

#ifdef __cplusplus
}
#endif

#endif // SWMM_MOCK_H
