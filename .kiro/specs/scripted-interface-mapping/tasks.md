# Implementation Plan: Scripted Interface Mapping

## Overview

This implementation plan breaks down the scripted interface mapping feature into discrete coding tasks. The approach follows a bottom-up strategy: build the Python parser first, then enhance the DLL to consume the generated mappings. Each task builds on previous work, with checkpoints to validate functionality before proceeding.

## Tasks

- [x] 1. Create Python parser script foundation
  - Create `generate_mapping.py` in project root
  - Implement command-line argument parsing
  - Implement file existence validation
  - Add usage instructions for no-argument case
  - _Requirements: 1.2, 9.1, 9.2_

- [ ]* 1.1 Write unit tests for CLI interface
  - Test argument parsing
  - Test usage message display
  - Test file validation errors
  - _Requirements: 1.2, 9.2_

- [ ] 2. Implement SWMM .inp file parser
  - [x] 2.1 Implement section parser
    - Parse section headers in square brackets
    - Skip comment lines starting with semicolons
    - Extract data lines with whitespace separation
    - Handle missing sections gracefully
    - _Requirements: 1.1, 1.4, 1.5, 10.1_
  
  - [ ]* 2.2 Write property test for section parsing
    - **Property 2: Comments are excluded from parsing**
    - **Property 3: Section headers are correctly identified**
    - **Validates: Requirements 1.4, 1.5**
  
  - [x] 2.3 Implement error handling for invalid syntax
    - Detect malformed section headers
    - Return descriptive error messages
    - _Requirements: 1.3_
  
  - [ ]* 2.4 Write property test for error handling
    - **Property 16: Invalid syntax produces errors**
    - **Validates: Requirements 1.3**

- [ ] 3. Implement input discovery logic
  - [x] 3.1 Implement elapsed time input (always index 0)
    - Create InputElement data structure
    - Add elapsed time as first input
    - _Requirements: 2.1_
  
  - [x] 3.2 Implement DUMMY rain gage discovery
    - Parse [RAINGAGES] section
    - Filter for TIMESERIES named DUMMY
    - Assign sequential indices starting from 1
    - Handle missing [RAINGAGES] section
    - _Requirements: 2.2, 2.3, 2.4, 2.5_
  
  - [ ]* 3.3 Write property tests for input discovery
    - **Property 4: Elapsed time is always first input**
    - **Property 5: DUMMY rain gages are discovered as inputs**
    - **Property 6: Non-DUMMY rain gages are excluded**
    - **Validates: Requirements 2.1, 2.2, 2.3, 2.5**

- [ ] 4. Implement output discovery logic
  - [x] 4.1 Implement output element discovery
    - Create OutputElement data structure
    - Parse [STORAGE] section for volume outputs
    - Parse [OUTFALLS] section for flow outputs
    - Parse [ORIFICES] section for flow outputs
    - Parse [WEIRS] section for flow outputs
    - Parse [SUBCATCHMENTS] section for runoff outputs
    - Assign indices in priority order
    - Preserve element order within sections
    - _Requirements: 3.1, 3.2, 3.3, 3.4, 3.5, 3.6, 3.7_
  
  - [ ]* 4.2 Write property tests for output discovery
    - **Property 7: All output elements are discovered**
    - **Property 8: Output priority ordering is preserved**
    - **Property 9: Element order within sections is preserved**
    - **Validates: Requirements 3.1-3.7**

- [x] 5. Checkpoint - Verify parser discovers elements correctly
  - Run parser on tests/test_model.inp
  - Verify correct input/output counts
  - Ensure all tests pass, ask the user if questions arise.

- [ ] 6. Implement hash computation
  - [x] 6.1 Implement content hash function
    - Read .inp file contents
    - Strip comments and normalize whitespace
    - Compute MD5 hash
    - _Requirements: 5.1, 5.3_
  
  - [ ]* 6.2 Write property tests for hash computation
    - **Property 12: Hash computation is deterministic**
    - **Property 13: Hash excludes comments and whitespace**
    - **Property 14: Hash is sensitive to content changes**
    - **Validates: Requirements 5.1, 5.3, 5.4**

- [ ] 7. Implement JSON mapping file generation
  - [x] 7.1 Implement JSON structure generation
    - Create mapping dictionary with version, hash, counts
    - Build inputs array with index, name, object_type, property
    - Build outputs array with index, name, object_type, property, swmm_index
    - Write to SwmmGoldSimBridge.json
    - _Requirements: 4.1, 4.2, 4.3, 4.4, 4.5, 4.6, 4.7, 4.8_
  
  - [ ]* 7.2 Write property tests for JSON generation
    - **Property 10: Generated mapping is valid JSON**
    - **Property 11: Mapping contains required structure**
    - **Property 25: Mapping generation is idempotent**
    - **Validates: Requirements 4.1, 4.3-4.7, 4.8**

- [ ] 8. Implement parser output and error reporting
  - [x] 8.1 Add success output messages
    - Print output file path
    - Print discovered input/output counts
    - _Requirements: 9.3, 9.4_
  
  - [x] 8.2 Add error handling and exit codes
    - Return non-zero exit code on errors
    - Print error messages to stderr
    - _Requirements: 9.5_
  
  - [ ]* 8.3 Write property test for error exit codes
    - **Property 17: Error conditions produce non-zero exit codes**
    - **Validates: Requirements 9.5**

- [x] 9. Checkpoint - Verify complete parser functionality
  - Generate mapping from tests/test_model.inp
  - Validate JSON structure and content
  - Test error cases (missing file, invalid syntax)
  - Ensure all tests pass, ask the user if questions arise.

- [x] 10. Implement DLL MappingLoader class
  - [x] 10.1 Create MappingLoader class structure
    - Define InputMapping and OutputMapping structs
    - Add member variables for mappings and hash
    - Implement getter methods
    - _Requirements: 6.1, 6.4, 6.5_
  
  - [x] 10.2 Implement JSON parsing in MappingLoader
    - Add JSON library dependency (e.g., nlohmann/json)
    - Implement LoadFromFile method
    - Parse version, hash, counts, inputs, outputs
    - Validate required fields exist
    - _Requirements: 6.1, 6.3_
  
  - [ ]* 10.3 Write unit tests for MappingLoader
    - Test loading valid JSON
    - Test handling invalid JSON
    - Test handling missing file
    - Test field validation
    - _Requirements: 6.2, 6.3_

- [x] 11. Update BridgeHandler for XF_REP_ARGUMENTS
  - [x] 11.1 Modify HandleReportArguments method
    - Load mapping file in HandleReportArguments
    - Return input_count and output_count from mapping
    - Handle file not found error
    - Handle invalid JSON error
    - _Requirements: 6.1, 6.2, 6.3, 6.4, 6.5_
  
  - [ ]* 11.2 Write property test for argument reporting
    - **Property 18: Valid mappings return correct argument counts**
    - **Property 19: Invalid JSON produces errors**
    - **Validates: Requirements 6.3, 6.4, 6.5**

- [x] 12. Update SwmmSimulation for dynamic initialization
  - [x] 12.1 Add mapping validation method
    - Implement ValidateMapping method
    - Check that all element names exist in SWMM model
    - Use swmm_getIndex to resolve element names
    - Store resolved indices
    - _Requirements: 7.1, 7.2, 7.3, 7.4_
  
  - [x] 12.2 Modify Initialize method
    - Accept MappingLoader parameter
    - Call ValidateMapping
    - Store element handles for calculation
    - Add hash mismatch warning (optional)
    - _Requirements: 7.1, 7.2, 7.3, 7.4, 7.5, 7.6_
  
  - [ ]* 12.3 Write property tests for initialization
    - **Property 20: All mapped elements resolve to handles**
    - **Property 21: Invalid element names produce errors**
    - **Validates: Requirements 7.2, 7.3, 7.4**

- [x] 13. Update SwmmSimulation for dynamic calculation
  - [x] 13.1 Modify Calculate method to use mapping
    - Accept MappingLoader parameter
    - Iterate through input mappings
    - Call swmm_setValue for each input (except elapsed time)
    - Iterate through output mappings
    - Call swmm_getValue for each output
    - Populate outargs in mapping order
    - _Requirements: 8.1, 8.2, 8.3, 8.4, 8.5, 8.6_
  
  - [ ]* 13.2 Write property tests for calculation
    - **Property 22: All inputs are processed during calculation**
    - **Property 23: All outputs are retrieved during calculation**
    - **Property 24: Invalid handles produce errors**
    - **Validates: Requirements 8.1-8.6**

- [x] 14. Update BridgeHandler to use MappingLoader
  - [x] 14.1 Add MappingLoader member variable
    - Add mapping_ member to BridgeHandler
    - Add mapping_loaded_ flag
    - Load mapping once during first use
    - _Requirements: 6.1, 7.1_
  
  - [x] 14.2 Update HandleInitialize to pass mapping
    - Pass mapping to simulation_.Initialize()
    - Handle mapping load errors
    - _Requirements: 7.1_
  
  - [x] 14.3 Update HandleCalculate to pass mapping
    - Pass mapping to simulation_.Calculate()
    - _Requirements: 8.1, 8.3_

- [x] 15. Checkpoint - Verify DLL integration
  - Build DLL with new MappingLoader
  - Test XF_REP_ARGUMENTS with generated mapping
  - Test XF_INITIALIZE with valid mapping
  - Test XF_CALCULATE with test model
  - Ensure all tests pass, ask the user if questions arise.

- [x] 16. Create integration tests
  - [x]* 16.1 Write end-to-end integration test
    - Generate mapping from test_model.inp
    - Load DLL and call XF_REP_ARGUMENTS
    - Verify counts match parser output
    - Call XF_INITIALIZE and verify success
    - Call XF_CALCULATE and verify outputs
    - _Requirements: All_
  
  - [x]* 16.2 Write property test for round-trip consistency
    - **Property 26: Round-trip consistency**
    - **Validates: Requirements 2.1-2.5, 3.1-3.7, 6.4-6.5**

- [x] 17. Update documentation
  - [x] 17.1 Update README.md
    - Add section on generating mapping files
    - Document Python script usage
    - Update DLL configuration instructions
    - Add troubleshooting for mapping errors
    - _Requirements: 9.1, 9.2, 9.3_
  
  - [x] 17.2 Create example workflow documentation
    - Document step-by-step process
    - Include example commands
    - Show expected output

- [x] 18. Final checkpoint - Complete system validation
  - Run full test suite (parser + DLL)
  - Generate mapping for demo_pond_pump.inp
  - Verify DLL works with complex model
  - Test error cases end-to-end
  - Ensure all tests pass, ask the user if questions arise.

## Notes

- Tasks marked with `*` are optional and can be skipped for faster MVP
- Python script uses standard library only (no external dependencies except for testing)
- DLL requires JSON parsing library (recommend nlohmann/json header-only library)
- Each property test should run minimum 100 iterations
- Checkpoints ensure incremental validation before proceeding
- All tasks reference specific requirements for traceability
