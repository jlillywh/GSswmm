# Requirements Document

## Introduction

This document specifies the requirements for a GoldSim-SWMM Bridge DLL that enables bidirectional communication between GoldSim simulation software and the EPA SWMM (Storm Water Management Model) hydraulic engine. The bridge allows GoldSim to control the simulation clock, provide rainfall inputs, and receive runoff calculations from SWMM, enabling integrated hydrologic-hydraulic modeling workflows.

## Glossary

- **GoldSim**: A probabilistic simulation software platform that supports external DLL integration through External elements
- **SWMM**: EPA Storm Water Management Model - a dynamic rainfall-runoff simulation model used for single event or long-term simulation of runoff quantity and quality from primarily urban areas
- **Bridge_DLL**: The dynamic link library that implements the interface between GoldSim and SWMM
- **External_Element**: GoldSim's mechanism for calling external DLL functions with specific method IDs
- **SWMM_Engine**: The SWMM computational engine accessed through the SWMM5 C API
- **Subcatchment**: A SWMM model component representing a drainage area that receives rainfall and generates runoff
- **Method_ID**: Integer value passed by GoldSim to indicate which operation the DLL should perform (initialize, calculate, report version, report arguments, cleanup)
- **Simulation_State**: The current lifecycle phase of the SWMM engine (uninitialized, opened, started, running, ended, closed)
- **Time_Step**: The simulation time increment at which calculations are performed
- **Rainfall_Intensity**: Precipitation rate provided by GoldSim to SWMM subcatchments (typically in inches/hour or mm/hour)
- **Runoff_Flow**: The calculated surface runoff from a subcatchment returned by SWMM to GoldSim (typically in CFS or CMS)

## Requirements

### Requirement 1: GoldSim External Element Interface Compliance

**User Story:** As a GoldSim user, I want the bridge DLL to comply with the GoldSim External Element API, so that it can be loaded and called correctly by GoldSim.

#### Acceptance Criteria

1. THE Bridge_DLL SHALL export a function with C linkage using the signature: `void __declspec(dllexport) FunctionName(int methodID, int* status, double* inargs, double* outargs)`
2. WHEN GoldSim calls the function with XF_REP_VERSION (methodID = 2), THE Bridge_DLL SHALL return the version number in outargs[0]
3. WHEN GoldSim calls the function with XF_REP_ARGUMENTS (methodID = 3), THE Bridge_DLL SHALL return the number of input arguments in outargs[0] and the number of output arguments in outargs[1]
4. WHEN any method call completes successfully, THE Bridge_DLL SHALL set status to 0 (XF_SUCCESS)
5. WHEN any method call encounters a fatal error, THE Bridge_DLL SHALL set status to a value greater than 0 and less than 99

### Requirement 2: SWMM Lifecycle Management

**User Story:** As a simulation engineer, I want the bridge to properly manage the SWMM engine lifecycle, so that SWMM simulations execute correctly and resources are properly released.

#### Acceptance Criteria

1. WHEN GoldSim calls XF_INITIALIZE (methodID = 0), THE Bridge_DLL SHALL call swmm_open() with the input file path, report file path, and output file path
2. WHEN swmm_open() succeeds, THE Bridge_DLL SHALL call swmm_start() to initialize the simulation
3. WHEN swmm_start() succeeds, THE Bridge_DLL SHALL transition to a running state where SWMM is ready to accept time step calculations
4. WHEN GoldSim calls XF_CLEANUP (methodID = 99), THE Bridge_DLL SHALL call swmm_end() followed by swmm_close() to properly terminate the SWMM simulation
5. WHEN swmm_end() or swmm_close() is called, THE Bridge_DLL SHALL transition to an uninitialized state
6. IF swmm_open(), swmm_start(), swmm_end(), or swmm_close() returns a non-zero error code, THEN THE Bridge_DLL SHALL set status to XF_FAILURE and return an error

### Requirement 3: Time Step Synchronization

**User Story:** As a modeler, I want the bridge to synchronize time steps between GoldSim and SWMM, so that both models advance together through the simulation period with one-to-one correspondence.

#### Acceptance Criteria

1. WHEN GoldSim calls XF_CALCULATE (methodID = 1), THE Bridge_DLL SHALL call swmm_step() exactly once to advance SWMM by one routing time step
2. WHEN swmm_step() returns, THE Bridge_DLL SHALL receive the elapsed simulation time from SWMM
3. IF swmm_step() returns a non-zero error code indicating the simulation has ended, THEN THE Bridge_DLL SHALL call swmm_end() and swmm_close() to properly terminate and return XF_SUCCESS
4. WHEN swmm_step() completes successfully, THE Bridge_DLL SHALL continue accepting subsequent XF_CALCULATE calls
5. THE Bridge_DLL SHALL assume that the GoldSim time step matches the SWMM ROUTING_STEP exactly for one-to-one time step correspondence

### Requirement 4: Rainfall Data Transfer from GoldSim to SWMM

**User Story:** As a hydrologist, I want to pass rainfall intensity values from GoldSim to SWMM subcatchments, so that SWMM can calculate runoff based on GoldSim-controlled precipitation.

#### Acceptance Criteria

1. WHEN GoldSim calls XF_CALCULATE (methodID = 1), THE Bridge_DLL SHALL read the rainfall intensity value from inargs[0]
2. WHEN a rainfall intensity value is received, THE Bridge_DLL SHALL call swmm_setValue() with property swmm_SUBCATCH_RAINFALL to set the rainfall for the target subcatchment
3. WHEN setting rainfall values, THE Bridge_DLL SHALL use the subcatchment index specified in the configuration
4. IF swmm_setValue() returns an error, THEN THE Bridge_DLL SHALL set status to XF_FAILURE

### Requirement 5: Runoff Data Transfer from SWMM to GoldSim

**User Story:** As a modeler, I want to retrieve runoff flow values from SWMM subcatchments and return them to GoldSim, so that GoldSim can use SWMM's hydraulic calculations in its simulation logic.

#### Acceptance Criteria

1. WHEN swmm_step() completes successfully, THE Bridge_DLL SHALL call swmm_getValue() with property swmm_SUBCATCH_RUNOFF to retrieve the runoff flow from the target subcatchment
2. WHEN a runoff value is retrieved, THE Bridge_DLL SHALL write the value to outargs[0]
3. WHEN retrieving runoff values, THE Bridge_DLL SHALL use the subcatchment index specified in the configuration
4. IF swmm_getValue() fails to retrieve a valid value, THEN THE Bridge_DLL SHALL set status to XF_FAILURE

### Requirement 6: File Path Configuration

**User Story:** As a user, I want to specify the SWMM input file and output file paths, so that the bridge can locate and use my SWMM model files.

#### Acceptance Criteria

1. THE Bridge_DLL SHALL accept the SWMM input file path (.inp) as a configuration parameter
2. THE Bridge_DLL SHALL accept the SWMM report file path (.rpt) as a configuration parameter
3. THE Bridge_DLL SHALL accept the SWMM output file path (.out) as a configuration parameter
4. WHEN file paths are relative, THE Bridge_DLL SHALL resolve them relative to the directory containing the GoldSim model file
5. IF any required file path is not provided or is invalid, THEN THE Bridge_DLL SHALL return XF_FAILURE during XF_INITIALIZE

### Requirement 7: Multiple Subcatchment Support

**User Story:** As a watershed modeler, I want to specify which SWMM subcatchment receives rainfall and provides runoff data, so that I can model different drainage areas within my SWMM model.

#### Acceptance Criteria

1. THE Bridge_DLL SHALL accept a subcatchment index as a configuration parameter
2. WHEN a subcatchment index is provided, THE Bridge_DLL SHALL use that index for all swmm_getValue() and swmm_setValue() calls related to subcatchment properties
3. IF the subcatchment index is out of range for the loaded SWMM model, THEN THE Bridge_DLL SHALL return XF_FAILURE during XF_INITIALIZE
4. WHERE no subcatchment index is specified, THE Bridge_DLL SHALL default to subcatchment index 0

### Requirement 8: Error Handling and Reporting

**User Story:** As a developer, I want clear error messages when the bridge encounters problems, so that I can diagnose and fix configuration or runtime issues.

#### Acceptance Criteria

1. WHEN any SWMM API function returns a non-zero error code, THE Bridge_DLL SHALL set status to XF_FAILURE
2. WHEN a fatal error occurs, THE Bridge_DLL SHALL call swmm_getError() to retrieve the SWMM error message
3. IF the Bridge_DLL encounters an error during XF_INITIALIZE, THEN THE Bridge_DLL SHALL ensure SWMM is properly closed before returning
4. WHEN an error occurs, THE Bridge_DLL SHALL return a descriptive error message to GoldSim using the XF_FAILURE_WITH_MSG mechanism (status = -1)
5. WHEN returning an error message, THE Bridge_DLL SHALL store the message in a static buffer and return its address in outargs[0]

### Requirement 9: State Management

**User Story:** As a simulation engineer, I want the bridge to track its internal state, so that it can prevent invalid operations and ensure proper sequencing of SWMM API calls.

#### Acceptance Criteria

1. THE Bridge_DLL SHALL maintain a state variable tracking whether SWMM is currently running
2. WHEN XF_INITIALIZE is called while SWMM is already running, THE Bridge_DLL SHALL first call swmm_end() and swmm_close() before opening a new simulation
3. WHEN XF_CALCULATE is called before SWMM has been initialized, THE Bridge_DLL SHALL return XF_FAILURE
4. WHEN XF_CLEANUP is called while SWMM is not running, THE Bridge_DLL SHALL return XF_SUCCESS without attempting to close SWMM

### Requirement 10: Platform and Architecture Requirements

**User Story:** As a GoldSim user on Windows, I want the bridge DLL to be compatible with my system architecture, so that it loads and runs correctly.

#### Acceptance Criteria

1. THE Bridge_DLL SHALL be compiled as a Windows x64 DLL for compatibility with modern GoldSim versions
2. THE Bridge_DLL SHALL link against the x64 version of swmm5.lib
3. THE Bridge_DLL SHALL be loadable by GoldSim using the standard LoadLibrary mechanism
4. THE Bridge_DLL SHALL export its function name in a case-sensitive manner matching the name specified in the GoldSim External element
