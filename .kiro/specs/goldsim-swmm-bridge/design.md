# Design Document: GoldSim-SWMM Bridge DLL

## Overview

The GoldSim-SWMM Bridge DLL is a C++ dynamic link library that enables bidirectional communication between GoldSim simulation software and the EPA SWMM hydraulic engine. The bridge implements the GoldSim External Element API and manages the complete SWMM lifecycle, allowing GoldSim to control simulation timing, provide rainfall inputs, and receive runoff calculations.

The design follows a state-machine pattern to manage the SWMM engine lifecycle and ensures proper sequencing of API calls. The bridge operates on a one-to-one time step correspondence model where each GoldSim calculation step triggers exactly one SWMM routing step.

## Architecture

### Component Overview

```
┌─────────────────────────────────────────────────────────────┐
│                        GoldSim                              │
│                   (External Element)                        │
└────────────────────┬────────────────────────────────────────┘
                     │ Method ID, Status, Input/Output Arrays
                     ▼
┌─────────────────────────────────────────────────────────────┐
│              Bridge DLL Entry Point                         │
│         SwmmGoldSimBridge(methodID, status,                 │
│                          inargs, outargs)                   │
└────────────────────┬────────────────────────────────────────┘
                     │
        ┌────────────┴────────────┐
        │                         │
        ▼                         ▼
┌──────────────────┐    ┌──────────────────────┐
│  State Manager   │    │  Configuration       │
│  - is_running    │    │  - file paths        │
│  - subcatch_idx  │    │  - subcatch index    │
└────────┬─────────┘    └──────────────────────┘
         │
         ▼
┌─────────────────────────────────────────────────────────────┐
│                    SWMM API Wrapper                         │
│  - swmm_open()    - swmm_getValue()                         │
│  - swmm_start()   - swmm_setValue()                         │
│  - swmm_step()    - swmm_getError()                         │
│  - swmm_end()                                               │
│  - swmm_close()                                             │
└────────────────────┬────────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────────┐
│                    SWMM Engine                              │
│                   (swmm5.dll)                               │
└─────────────────────────────────────────────────────────────┘
```

### State Machine

The bridge maintains a simple state machine to track the SWMM engine lifecycle:

```
┌─────────────┐
│ UNINITIALIZED│
└──────┬───────┘
       │ XF_INITIALIZE
       │ swmm_open() + swmm_start()
       ▼
┌─────────────┐
│   RUNNING   │◄────┐
└──────┬───────┘     │
       │             │ XF_CALCULATE
       │             │ swmm_step()
       │             │
       │             └─────────────┐
       │                           │
       │ XF_CLEANUP or             │
       │ swmm_step() returns end   │
       │ swmm_end() + swmm_close() │
       ▼                           │
┌─────────────┐                    │
│ UNINITIALIZED│───────────────────┘
└─────────────┘
```

## Components and Interfaces

### 1. Bridge Entry Point

**Function Signature:**
```cpp
extern "C" void __declspec(dllexport) SwmmGoldSimBridge(
    int methodID,
    int* status,
    double* inargs,
    double* outargs
)
```

**Parameters:**
- `methodID`: Integer indicating the requested operation (0=Initialize, 1=Calculate, 2=Report Version, 3=Report Arguments, 99=Cleanup)
- `status`: Pointer to integer for returning operation status (0=success, >0=failure, -1=failure with message)
- `inargs`: Array of double-precision input arguments from GoldSim
- `outargs`: Array of double-precision output arguments to GoldSim

**Responsibilities:**
- Dispatch method calls based on methodID
- Manage global state variables
- Coordinate SWMM API calls
- Handle error conditions and reporting

### 2. Global State Variables

```cpp
static bool is_swmm_running = false;
static int subcatchment_index = 0;
static char input_file_path[260] = "model.inp";
static char report_file_path[260] = "model.rpt";
static char output_file_path[260] = "model.out";
static char error_message_buffer[200];
```

**State Variables:**
- `is_swmm_running`: Boolean flag tracking whether SWMM has been initialized and started
- `subcatchment_index`: Zero-based index of the SWMM subcatchment for rainfall/runoff exchange
- `input_file_path`: Path to SWMM input file (.inp)
- `report_file_path`: Path to SWMM report file (.rpt)
- `output_file_path`: Path to SWMM output file (.out)
- `error_message_buffer`: Static buffer for error messages returned to GoldSim

### 3. Method Handlers

#### XF_INITIALIZE (methodID = 0)

**Purpose:** Initialize SWMM engine at the start of each realization

**Logic:**
1. Check if SWMM is already running; if so, call cleanup sequence
2. Call `swmm_open(input_file_path, report_file_path, output_file_path)`
3. If swmm_open succeeds, call `swmm_start(1)` to begin simulation with output saving
4. If both succeed, set `is_swmm_running = true`
5. If either fails, retrieve error message and return XF_FAILURE

**Error Handling:**
- If swmm_open fails, return error immediately
- If swmm_start fails, call swmm_close before returning error

#### XF_CALCULATE (methodID = 1)

**Purpose:** Advance simulation one time step and exchange data

**Logic:**
1. Verify `is_swmm_running == true`; if false, return XF_FAILURE
2. Read rainfall intensity from `inargs[0]`
3. Call `swmm_setValue(swmm_SUBCATCH_RAINFALL, subcatchment_index, inargs[0])`
4. Call `swmm_step(&elapsed_time)` to advance SWMM one routing step
5. Check swmm_step return code:
   - If 0: simulation continues normally
   - If >0: simulation has ended, call cleanup sequence and return XF_SUCCESS
   - If <0: error occurred, retrieve error message and return XF_FAILURE
6. Call `swmm_getValue(swmm_SUBCATCH_RUNOFF, subcatchment_index)` to get runoff
7. Write runoff value to `outargs[0]`

**Data Flow:**
- Input: `inargs[0]` = rainfall intensity (inches/hour or mm/hour)
- Output: `outargs[0]` = runoff flow (CFS or CMS depending on SWMM flow units)

#### XF_REP_VERSION (methodID = 2)

**Purpose:** Report DLL version number to GoldSim

**Logic:**
1. Write version number to `outargs[0]` (e.g., 1.00)
2. Return XF_SUCCESS

#### XF_REP_ARGUMENTS (methodID = 3)

**Purpose:** Report number of input and output arguments

**Logic:**
1. Write number of inputs to `outargs[0]` (1 input: rainfall)
2. Write number of outputs to `outargs[1]` (1 output: runoff)
3. Return XF_SUCCESS

#### XF_CLEANUP (methodID = 99)

**Purpose:** Terminate SWMM simulation and release resources

**Logic:**
1. Check if `is_swmm_running == true`
2. If true:
   - Call `swmm_end()` to finalize simulation
   - Call `swmm_close()` to close files and release memory
   - Set `is_swmm_running = false`
3. Return XF_SUCCESS

### 4. Error Handling Module

**Function:** `HandleSwmmError(int error_code, double* outargs, int* status)`

**Purpose:** Retrieve SWMM error messages and format them for GoldSim

**Logic:**
1. Call `swmm_getError(error_message_buffer, sizeof(error_message_buffer))`
2. Ensure error message is null-terminated
3. Cast `outargs` as `ULONG_PTR*` and store address of `error_message_buffer`
4. Set `*status = XF_FAILURE_WITH_MSG` (-1)

**Memory Safety:**
- Uses static buffer with file scope to ensure memory remains valid when GoldSim reads it
- Buffer size limited to 200 characters to prevent overflow

## Data Models

### Input Arguments Array (inargs)

| Index | Type   | Description                    | Units                  |
|-------|--------|--------------------------------|------------------------|
| 0     | double | Rainfall intensity             | inches/hr or mm/hr     |

### Output Arguments Array (outargs)

| Index | Type   | Description                    | Units                  |
|-------|--------|--------------------------------|------------------------|
| 0     | double | Runoff flow rate               | CFS or CMS             |

**Special Cases:**
- When methodID = XF_REP_VERSION: outargs[0] = version number
- When methodID = XF_REP_ARGUMENTS: outargs[0] = num inputs, outargs[1] = num outputs
- When status = XF_FAILURE_WITH_MSG: outargs[0] = pointer to error message string

### Configuration Data

Configuration is currently hardcoded in global variables but designed for future extension:

```cpp
struct BridgeConfiguration {
    char input_file[260];      // SWMM .inp file path
    char report_file[260];     // SWMM .rpt file path
    char output_file[260];     // SWMM .out file path
    int subcatchment_index;    // Target subcatchment (0-based)
};
```

## Correctness Properties

*A property is a characteristic or behavior that should hold true across all valid executions of a system—essentially, a formal statement about what the system should do. Properties serve as the bridge between human-readable specifications and machine-verifiable correctness guarantees.*


### Property 1: Successful method calls return XF_SUCCESS

*For any* valid method call that completes without errors, the status code should be set to 0 (XF_SUCCESS).

**Validates: Requirements 1.4**

### Property 2: Failed method calls return appropriate error codes

*For any* method call that encounters a fatal error, the status code should be set to a value greater than 0 and less than 99, or -1 for errors with messages.

**Validates: Requirements 1.5**

### Property 3: SWMM API errors propagate to status

*For any* SWMM API function call that returns a non-zero error code, the bridge should set the status to XF_FAILURE.

**Validates: Requirements 8.1**

### Property 4: Subcatchment index consistency

*For any* configured subcatchment index, all swmm_getValue() and swmm_setValue() calls related to subcatchment properties should use that same index consistently throughout the simulation.

**Validates: Requirements 4.3, 5.3, 7.2**

### Property 5: File path validation

*For any* file path that is invalid or points to a non-existent file, calling XF_INITIALIZE should return XF_FAILURE.

**Validates: Requirements 6.5**

### Property 6: Subcatchment index validation

*For any* subcatchment index that is out of range for the loaded SWMM model, calling XF_INITIALIZE should return XF_FAILURE.

**Validates: Requirements 7.3**

### Property 7: State invariant maintenance

*For any* sequence of method calls, the is_swmm_running state variable should accurately reflect whether SWMM has been successfully initialized and not yet cleaned up.

**Validates: Requirements 9.1**

### Property 8: Path resolution consistency

*For any* relative file path, the bridge should resolve it relative to the same base directory consistently across all file operations.

**Validates: Requirements 6.4**

## Error Handling

### Error Detection

The bridge implements comprehensive error detection at multiple levels:

1. **SWMM API Errors**: All SWMM function calls are checked for non-zero return codes
2. **State Validation**: Method calls are validated against current state (e.g., cannot calculate before initialize)
3. **Parameter Validation**: File paths and subcatchment indices are validated during initialization
4. **Resource Cleanup**: Errors during initialization trigger cleanup of partially-initialized resources

### Error Reporting

The bridge uses the GoldSim error reporting mechanism:

```cpp
void HandleSwmmError(int error_code, double* outargs, int* status) {
    // Retrieve error message from SWMM
    swmm_getError(error_message_buffer, sizeof(error_message_buffer));
    
    // Ensure null termination
    error_message_buffer[sizeof(error_message_buffer) - 1] = '\0';
    
    // Cast outargs to pointer type and store buffer address
    ULONG_PTR* pAddr = (ULONG_PTR*)outargs;
    *pAddr = (ULONG_PTR)error_message_buffer;
    
    // Set status to indicate error with message
    *status = XF_FAILURE_WITH_MSG;
}
```

### Error Recovery

- **Initialization Errors**: If swmm_open succeeds but swmm_start fails, the bridge calls swmm_close before returning
- **Simulation End**: When swmm_step indicates simulation end, the bridge performs normal cleanup and returns success
- **Re-initialization**: If XF_INITIALIZE is called while SWMM is running, the bridge first performs cleanup before starting a new simulation

### Critical Error Scenarios

| Scenario | Detection | Response |
|----------|-----------|----------|
| Invalid input file | swmm_open returns error | Return XF_FAILURE_WITH_MSG with SWMM error |
| SWMM initialization failure | swmm_start returns error | Call swmm_close, return XF_FAILURE_WITH_MSG |
| Invalid subcatchment index | swmm_setValue/getValue fails | Return XF_FAILURE_WITH_MSG |
| Calculate before initialize | is_swmm_running == false | Return XF_FAILURE |
| Simulation end | swmm_step returns >0 | Call cleanup, return XF_SUCCESS |
| SWMM runtime error | swmm_step returns <0 | Return XF_FAILURE_WITH_MSG |

## Testing Strategy

### Dual Testing Approach

The testing strategy employs both unit testing and property-based testing to ensure comprehensive coverage:

**Unit Tests** focus on:
- Specific method call sequences (initialize → calculate → cleanup)
- Edge cases (cleanup when not running, calculate before initialize)
- Error conditions (invalid files, out-of-range indices)
- State transitions (uninitialized → running → uninitialized)
- Integration with mocked SWMM API

**Property-Based Tests** focus on:
- Universal properties across all valid inputs (status codes, index consistency)
- Error propagation across all SWMM API functions
- State invariant maintenance across arbitrary method sequences
- Path resolution consistency

### Property-Based Testing Configuration

**Library**: For C++, we will use RapidCheck (https://github.com/emil-e/rapidcheck), a QuickCheck-inspired property-based testing library.

**Configuration**:
- Minimum 100 iterations per property test
- Each test tagged with: **Feature: goldsim-swmm-bridge, Property N: [property text]**
- Tests will use mocked SWMM API to avoid file system dependencies

**Example Property Test Structure**:
```cpp
// Feature: goldsim-swmm-bridge, Property 1: Successful method calls return XF_SUCCESS
RC_GTEST_PROP(BridgeProperties, SuccessfulCallsReturnSuccess, ()) {
    // Generate valid method ID
    auto methodID = *rc::gen::element(0, 1, 2, 3, 99);
    
    // Setup mock SWMM to succeed
    MockSwmmSuccess();
    
    // Call bridge
    int status;
    double inargs[1] = {0.0};
    double outargs[2];
    SwmmGoldSimBridge(methodID, &status, inargs, outargs);
    
    // Verify status is XF_SUCCESS
    RC_ASSERT(status == 0);
}
```

### Unit Testing Strategy

**Test Framework**: Google Test (gtest) for C++ unit testing

**Mock Strategy**: 
- Create mock implementations of SWMM API functions
- Use function pointers or link-time substitution to inject mocks
- Mock functions track call counts and parameters for verification

**Test Categories**:

1. **Lifecycle Tests**
   - Test: Initialize → Calculate → Cleanup sequence
   - Test: Multiple realizations (Initialize → Calculate → Cleanup → Initialize)
   - Test: Re-initialization while running

2. **Method Handler Tests**
   - Test: XF_REP_VERSION returns correct version
   - Test: XF_REP_ARGUMENTS returns 1 input, 1 output
   - Test: XF_INITIALIZE opens and starts SWMM
   - Test: XF_CALCULATE sets rainfall, steps, gets runoff
   - Test: XF_CLEANUP ends and closes SWMM

3. **Error Handling Tests**
   - Test: Invalid file path during initialize
   - Test: swmm_start failure triggers cleanup
   - Test: Calculate before initialize returns failure
   - Test: Out-of-range subcatchment index
   - Test: SWMM runtime error propagates correctly

4. **State Management Tests**
   - Test: is_swmm_running flag transitions correctly
   - Test: Cleanup when not running succeeds without SWMM calls
   - Test: State persists across multiple calculate calls

5. **Data Exchange Tests**
   - Test: Rainfall value from inargs[0] passed to SWMM
   - Test: Runoff value from SWMM written to outargs[0]
   - Test: Correct subcatchment index used for all operations

### Integration Testing

While unit tests use mocked SWMM API, integration tests should:
- Use actual SWMM engine with test input files
- Verify end-to-end data flow
- Test with various SWMM model configurations
- Validate numerical results against known scenarios

**Test SWMM Models**:
- Simple single-subcatchment model for basic validation
- Multi-subcatchment model for index testing
- Model with various time steps for synchronization testing

### Test Execution

**Unit Tests**: Run on every build, fast execution with mocks
**Property Tests**: Run on every build, 100+ iterations per property
**Integration Tests**: Run before releases, slower execution with real SWMM

**Coverage Goals**:
- Line coverage: >90%
- Branch coverage: >85%
- All error paths exercised
- All state transitions tested
