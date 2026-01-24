//-----------------------------------------------------------------------------
//   swmm_mock.cpp
//
//   Implementation of SWMM API mock for unit testing
//-----------------------------------------------------------------------------

#include "swmm_mock.h"
#include <string.h>
#include <stdio.h>

//-----------------------------------------------------------------------------
// Global mock state
//-----------------------------------------------------------------------------
SwmmMockState g_mock_state;

//-----------------------------------------------------------------------------
// Mock Control Functions Implementation
//-----------------------------------------------------------------------------

void SwmmMock_Reset()
{
    // Reset call counts
    g_mock_state.open_call_count = 0;
    g_mock_state.start_call_count = 0;
    g_mock_state.step_call_count = 0;
    g_mock_state.end_call_count = 0;
    g_mock_state.close_call_count = 0;
    g_mock_state.getValue_call_count = 0;
    g_mock_state.setValue_call_count = 0;
    g_mock_state.getError_call_count = 0;
    g_mock_state.getCount_call_count = 0;
    
    // Reset parameter tracking
    g_mock_state.last_input_file = "";
    g_mock_state.last_report_file = "";
    g_mock_state.last_output_file = "";
    g_mock_state.last_start_save_flag = 0;
    g_mock_state.last_getValue_type = 0;
    g_mock_state.last_getValue_index = 0;
    g_mock_state.last_setValue_type = 0;
    g_mock_state.last_setValue_index = 0;
    g_mock_state.last_setValue_value = 0.0;
    g_mock_state.last_step_elapsed_time = 0.0;
    g_mock_state.last_getCount_type = 0;
    
    // Reset return values to success defaults
    g_mock_state.open_return_code = 0;
    g_mock_state.start_return_code = 0;
    g_mock_state.step_return_code = 0;
    g_mock_state.end_return_code = 0;
    g_mock_state.close_return_code = 0;
    g_mock_state.getValue_return_value = 0.0;
    g_mock_state.error_message = "";
    g_mock_state.getCount_return_value = 1;  // Default to 1 subcatchment
    
    // Reset step behavior
    g_mock_state.step_calls_until_end = 0;
    g_mock_state.step_calls_until_error = 0;
    
    // Reset state flags
    g_mock_state.is_opened = false;
    g_mock_state.is_started = false;
}

void SwmmMock_SetSuccessMode()
{
    g_mock_state.open_return_code = 0;
    g_mock_state.start_return_code = 0;
    g_mock_state.step_return_code = 0;
    g_mock_state.end_return_code = 0;
    g_mock_state.close_return_code = 0;
    g_mock_state.error_message = "";
    g_mock_state.step_calls_until_end = 0;
    g_mock_state.step_calls_until_error = 0;
}

void SwmmMock_SetOpenFailure(int error_code, const char* error_msg)
{
    g_mock_state.open_return_code = error_code;
    g_mock_state.error_message = error_msg ? error_msg : "Mock open error";
}

void SwmmMock_SetStartFailure(int error_code, const char* error_msg)
{
    g_mock_state.start_return_code = error_code;
    g_mock_state.error_message = error_msg ? error_msg : "Mock start error";
}

void SwmmMock_SetStepFailure(int error_code, const char* error_msg)
{
    g_mock_state.step_return_code = error_code;
    g_mock_state.error_message = error_msg ? error_msg : "Mock step error";
}

void SwmmMock_SetEndFailure(int error_code, const char* error_msg)
{
    g_mock_state.end_return_code = error_code;
    g_mock_state.error_message = error_msg ? error_msg : "Mock end error";
}

void SwmmMock_SetCloseFailure(int error_code, const char* error_msg)
{
    g_mock_state.close_return_code = error_code;
    g_mock_state.error_message = error_msg ? error_msg : "Mock close error";
}

void SwmmMock_SetStepEndAfter(int num_calls)
{
    g_mock_state.step_calls_until_end = num_calls;
}

void SwmmMock_SetStepErrorAfter(int num_calls)
{
    g_mock_state.step_calls_until_error = num_calls;
}

void SwmmMock_SetGetValueReturn(double value)
{
    g_mock_state.getValue_return_value = value;
}

void SwmmMock_SetGetCountReturn(int count)
{
    g_mock_state.getCount_return_value = count;
}

int SwmmMock_GetOpenCallCount()
{
    return g_mock_state.open_call_count;
}

int SwmmMock_GetStartCallCount()
{
    return g_mock_state.start_call_count;
}

int SwmmMock_GetStepCallCount()
{
    return g_mock_state.step_call_count;
}

int SwmmMock_GetEndCallCount()
{
    return g_mock_state.end_call_count;
}

int SwmmMock_GetCloseCallCount()
{
    return g_mock_state.close_call_count;
}

int SwmmMock_GetValueCallCount()
{
    return g_mock_state.getValue_call_count;
}

int SwmmMock_GetSetValueCallCount()
{
    return g_mock_state.setValue_call_count;
}

const char* SwmmMock_GetLastInputFile()
{
    return g_mock_state.last_input_file.c_str();
}

const char* SwmmMock_GetLastReportFile()
{
    return g_mock_state.last_report_file.c_str();
}

const char* SwmmMock_GetLastOutputFile()
{
    return g_mock_state.last_output_file.c_str();
}

int SwmmMock_GetLastStartSaveFlag()
{
    return g_mock_state.last_start_save_flag;
}

int SwmmMock_GetLastGetValueType()
{
    return g_mock_state.last_getValue_type;
}

int SwmmMock_GetLastGetValueIndex()
{
    return g_mock_state.last_getValue_index;
}

int SwmmMock_GetLastSetValueType()
{
    return g_mock_state.last_setValue_type;
}

int SwmmMock_GetLastSetValueIndex()
{
    return g_mock_state.last_setValue_index;
}

double SwmmMock_GetLastSetValueValue()
{
    return g_mock_state.last_setValue_value;
}

//-----------------------------------------------------------------------------
// Mock SWMM API Functions Implementation
//-----------------------------------------------------------------------------

extern "C" int swmm_open(const char* f1, const char* f2, const char* f3)
{
    g_mock_state.open_call_count++;
    g_mock_state.last_input_file = f1 ? f1 : "";
    g_mock_state.last_report_file = f2 ? f2 : "";
    g_mock_state.last_output_file = f3 ? f3 : "";
    
    if (g_mock_state.open_return_code == 0)
    {
        g_mock_state.is_opened = true;
    }
    
    return g_mock_state.open_return_code;
}

extern "C" int swmm_start(int saveFlag)
{
    g_mock_state.start_call_count++;
    g_mock_state.last_start_save_flag = saveFlag;
    
    if (g_mock_state.start_return_code == 0)
    {
        g_mock_state.is_started = true;
    }
    
    return g_mock_state.start_return_code;
}

extern "C" int swmm_step(double* elapsedTime)
{
    g_mock_state.step_call_count++;
    
    // Update elapsed time (simulate time progression)
    g_mock_state.last_step_elapsed_time += 300.0; // 5 minutes in seconds
    if (elapsedTime)
    {
        *elapsedTime = g_mock_state.last_step_elapsed_time;
    }
    
    // Check if we should simulate simulation end
    if (g_mock_state.step_calls_until_end > 0 && 
        g_mock_state.step_call_count >= g_mock_state.step_calls_until_end)
    {
        return 1; // Simulation ended
    }
    
    // Check if we should simulate error
    if (g_mock_state.step_calls_until_error > 0 && 
        g_mock_state.step_call_count >= g_mock_state.step_calls_until_error)
    {
        return -1; // Error occurred
    }
    
    return g_mock_state.step_return_code;
}

extern "C" int swmm_end()
{
    g_mock_state.end_call_count++;
    g_mock_state.is_started = false;
    return g_mock_state.end_return_code;
}

extern "C" int swmm_close()
{
    g_mock_state.close_call_count++;
    g_mock_state.is_opened = false;
    return g_mock_state.close_return_code;
}

extern "C" void swmm_setValue(int type, int index, double value)
{
    g_mock_state.setValue_call_count++;
    g_mock_state.last_setValue_type = type;
    g_mock_state.last_setValue_index = index;
    g_mock_state.last_setValue_value = value;
}

extern "C" double swmm_getValue(int type, int index)
{
    g_mock_state.getValue_call_count++;
    g_mock_state.last_getValue_type = type;
    g_mock_state.last_getValue_index = index;
    return g_mock_state.getValue_return_value;
}

extern "C" int swmm_getError(char* errMsg, int msgLen)
{
    g_mock_state.getError_call_count++;
    
    if (errMsg && msgLen > 0)
    {
        strncpy(errMsg, g_mock_state.error_message.c_str(), msgLen - 1);
        errMsg[msgLen - 1] = '\0';
    }
    
    return 0;
}

extern "C" int swmm_getCount(int objType)
{
    g_mock_state.getCount_call_count++;
    g_mock_state.last_getCount_type = objType;
    return g_mock_state.getCount_return_value;
}
