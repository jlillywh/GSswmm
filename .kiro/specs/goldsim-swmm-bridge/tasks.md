# Implementation Plan: GoldSim-SWMM Bridge DLL

## Overview

This implementation plan breaks down the GoldSim-SWMM Bridge DLL into discrete coding tasks. The approach follows an incremental development pattern: establish the core interface, implement lifecycle management, add data exchange, implement error handling, and finally add comprehensive testing. Each task builds on previous work and includes validation through tests.

## Tasks

- [x] 1. Set up project structure and core interface
  - Create main source file (SwmmGoldSimBridge.cpp) with entry point function
  - Define GoldSim method ID enumerations (XF_INITIALIZE, XF_CALCULATE, etc.)
  - Define status code enumerations (XF_SUCCESS, XF_FAILURE, etc.)
  - Implement basic switch-case dispatcher for method IDs
  - Add SWMM header includes and library linkage
  - _Requirements: 1.1, 1.2, 1.3_

- [ ] 2. Implement version and argument reporting
  - [x] 2.1 Implement XF_REP_VERSION handler
    - Return version number 1.00 in outargs[0]
    - Set status to XF_SUCCESS
    - _Requirements: 1.2_
  
  - [x] 2.2 Implement XF_REP_ARGUMENTS handler
    - Return 1 input argument in outargs[0]
    - Return 1 output argument in outargs[1]
    - Set status to XF_SUCCESS
    - _Requirements: 1.3_
  
  - [ ]* 2.3 Write unit tests for version and argument reporting
    - Test XF_REP_VERSION returns 1.00
    - Test XF_REP_ARGUMENTS returns (1, 1)
    - Test both set status to XF_SUCCESS
    - _Requirements: 1.2, 1.3_

- [ ] 3. Implement global state management
  - [x] 3.1 Define global state variables
    - Add is_swmm_running boolean flag
    - Add subcatchment_index integer (default 0)
    - Add file path character arrays (input, report, output)
    - Add error_message_buffer static array
    - _Requirements: 9.1, 6.1, 6.2, 6.3_
  
  - [ ]* 3.2 Write property test for state invariant maintenance
    - **Property 7: State invariant maintenance**
    - **Validates: Requirements 9.1**
    - Generate random sequences of method calls
    - Verify is_swmm_running accurately reflects SWMM state
    - _Requirements: 9.1_

- [ ] 4. Implement SWMM lifecycle management
  - [x] 4.1 Implement XF_INITIALIZE handler
    - Check if SWMM is already running; if so, call cleanup
    - Call swmm_open() with file paths
    - If successful, call swmm_start(1)
    - Set is_swmm_running = true on success
    - Handle errors and return appropriate status
    - _Requirements: 2.1, 2.2, 2.3, 9.2_
  
  - [x] 4.2 Implement XF_CLEANUP handler
    - Check if is_swmm_running is true
    - If true, call swmm_end() then swmm_close()
    - Set is_swmm_running = false
    - Return XF_SUCCESS
    - _Requirements: 2.4, 2.5, 9.4_
  
  - [ ]* 4.3 Write unit tests for lifecycle management
    - Test initialize opens and starts SWMM
    - Test cleanup ends and closes SWMM
    - Test re-initialization while running
    - Test cleanup when not running
    - _Requirements: 2.1, 2.2, 2.3, 2.4, 2.5, 9.2, 9.4_
  
  - [ ]* 4.4 Write property test for SWMM API error propagation
    - **Property 3: SWMM API errors propagate to status**
    - **Validates: Requirements 8.1**
    - Mock various SWMM functions to return errors
    - Verify all errors result in XF_FAILURE status
    - _Requirements: 8.1_

- [x] 5. Checkpoint - Ensure lifecycle tests pass
  - Ensure all tests pass, ask the user if questions arise.

- [ ] 6. Implement time step synchronization and data exchange
  - [x] 6.1 Implement XF_CALCULATE handler core logic
    - Verify is_swmm_running is true; return XF_FAILURE if false
    - Read rainfall intensity from inargs[0]
    - Call swmm_setValue(swmm_SUBCATCH_RAINFALL, subcatchment_index, rainfall)
    - Call swmm_step(&elapsed_time) exactly once
    - Handle swmm_step return codes (0=continue, >0=end, <0=error)
    - Call swmm_getValue(swmm_SUBCATCH_RUNOFF, subcatchment_index)
    - Write runoff value to outargs[0]
    - _Requirements: 3.1, 3.2, 3.3, 3.4, 4.1, 4.2, 5.1, 5.2, 9.3_
  
  - [ ]* 6.2 Write unit tests for calculate handler
    - Test calculate before initialize returns failure
    - Test rainfall value passed to swmm_setValue
    - Test swmm_step called exactly once
    - Test runoff value written to outargs[0]
    - Test simulation end handling
    - _Requirements: 3.1, 3.3, 3.4, 4.1, 4.2, 5.1, 5.2, 9.3_
  
  - [ ]* 6.3 Write property test for subcatchment index consistency
    - **Property 4: Subcatchment index consistency**
    - **Validates: Requirements 4.3, 5.3, 7.2**
    - Generate random subcatchment indices
    - Verify same index used for all getValue/setValue calls
    - _Requirements: 4.3, 5.3, 7.2_

- [ ] 7. Implement error handling and reporting
  - [x] 7.1 Create HandleSwmmError helper function
    - Call swmm_getError() to retrieve error message
    - Store message in error_message_buffer
    - Ensure null termination
    - Cast outargs to ULONG_PTR* and store buffer address
    - Set status to XF_FAILURE_WITH_MSG (-1)
    - _Requirements: 8.1, 8.2, 8.4, 8.5_
  
  - [x] 7.2 Integrate error handling into all method handlers
    - Add HandleSwmmError calls for all SWMM API failures
    - Ensure cleanup on initialization errors
    - Add error handling for invalid state transitions
    - _Requirements: 2.6, 8.1, 8.3_
  
  - [ ]* 7.3 Write unit tests for error handling
    - Test error message retrieval and formatting
    - Test cleanup on initialization failure
    - Test error message buffer address in outargs[0]
    - Test status set to XF_FAILURE_WITH_MSG
    - _Requirements: 8.1, 8.2, 8.3, 8.4, 8.5_
  
  - [ ]* 7.4 Write property test for successful calls return success
    - **Property 1: Successful method calls return XF_SUCCESS**
    - **Validates: Requirements 1.4**
    - Generate random valid method calls
    - Mock SWMM to succeed
    - Verify status is always 0
    - _Requirements: 1.4_
  
  - [ ]* 7.5 Write property test for failed calls return error codes
    - **Property 2: Failed method calls return appropriate error codes**
    - **Validates: Requirements 1.5**
    - Generate error conditions
    - Verify status is >0 and <99, or -1 for messages
    - _Requirements: 1.5_

- [ ] 8. Implement validation and configuration
  - [x] 8.1 Add file path validation
    - Validate file paths during XF_INITIALIZE
    - Check if input file exists
    - Return XF_FAILURE for invalid paths
    - _Requirements: 6.4, 6.5_
  
  - [x] 8.2 Add subcatchment index validation
    - Validate subcatchment index during XF_INITIALIZE
    - Use swmm_getCount(swmm_SUBCATCH) to check range
    - Return XF_FAILURE for out-of-range indices
    - Implement default to index 0 if not specified
    - _Requirements: 7.2, 7.3, 7.4_
  
  - [ ]* 8.3 Write property test for file path validation
    - **Property 5: File path validation**
    - **Validates: Requirements 6.5**
    - Generate invalid file paths
    - Verify XF_INITIALIZE returns XF_FAILURE
    - _Requirements: 6.5_
  
  - [ ]* 8.4 Write property test for subcatchment index validation
    - **Property 6: Subcatchment index validation**
    - **Validates: Requirements 7.3**
    - Generate out-of-range indices
    - Verify XF_INITIALIZE returns XF_FAILURE
    - _Requirements: 7.3_
  
  - [ ]* 8.5 Write property test for path resolution consistency
    - **Property 8: Path resolution consistency**
    - **Validates: Requirements 6.4**
    - Generate relative file paths
    - Verify consistent resolution across operations
    - _Requirements: 6.4_

- [x] 9. Checkpoint - Ensure all tests pass
  - Ensure all tests pass, ask the user if questions arise.

- [x] 10. Create test infrastructure and mocks
  - [x] 10.1 Create SWMM API mock implementation
    - Implement mock versions of swmm_open, swmm_start, swmm_step, etc.
    - Add call tracking and parameter verification
    - Add configurable return values for testing
    - _Requirements: All testing requirements_
  
  - [x] 10.2 Set up Google Test framework
    - Add gtest library to project
    - Create test runner main function
    - Configure build system for tests
    - _Requirements: All testing requirements_
  
  - [x] 10.3 Set up RapidCheck framework
    - Add RapidCheck library to project
    - Configure for minimum 100 iterations per property
    - Create property test utilities and generators
    - _Requirements: All property testing requirements_

- [ ] 11. Integration and final validation
  - [ ] 11.1 Create integration test with real SWMM
    - Create simple test SWMM model (single subcatchment)
    - Test full lifecycle with actual SWMM engine
    - Verify numerical results
    - _Requirements: All requirements_
  
  - [ ] 11.2 Add build configuration
    - Configure Visual Studio project for x64 release build
    - Ensure proper linking with swmm5.lib
    - Verify DLL exports function correctly
    - _Requirements: 10.1, 10.2, 10.3, 10.4_
  
  - [ ]* 11.3 Write end-to-end integration tests
    - Test complete simulation sequence
    - Test multiple realizations
    - Test various SWMM model configurations
    - _Requirements: All requirements_

- [ ] 12. Final checkpoint - Ensure all tests pass
  - Ensure all tests pass, ask the user if questions arise.

## Notes

- Tasks marked with `*` are optional and can be skipped for faster MVP
- Each task references specific requirements for traceability
- Checkpoints ensure incremental validation at key milestones
- Property tests validate universal correctness properties with 100+ iterations
- Unit tests validate specific examples, edge cases, and error conditions
- Mock SWMM API allows fast unit testing without file system dependencies
- Integration tests with real SWMM validate end-to-end functionality
