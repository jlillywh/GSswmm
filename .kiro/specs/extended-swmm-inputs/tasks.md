# Implementation Plan: Extended SWMM Inputs

## Overview

This implementation extends the SWMM-GoldSim bridge parser and DLL to support dynamic control of pumps, valves (orifices/weirs), and node inflows in addition to rain gages. The implementation follows a phased approach: first extending the parser to discover new element types, then updating the bridge DLL to handle them, and finally adding comprehensive tests.

## Tasks

- [x] 1. Extend parser to discover pump inputs
  - [x] 1.1 Implement discover_pumps() function
    - Parse [PUMPS] section to find pumps with Pcurve "DUMMY"
    - Return list of InputElement objects with object_type "PUMP" and property "SETTING"
    - Handle missing [PUMPS] section gracefully
    - _Requirements: 1.1, 1.2, 1.3, 1.5, 1.6_
  
  - [ ]* 1.2 Write property test for pump discovery
    - **Property 1: DUMMY Element Discovery (Pumps)**
    - **Validates: Requirements 1.1, 1.6**
  
  - [x]* 1.3 Write unit tests for discover_pumps()
    - Test with DUMMY pumps
    - Test with non-DUMMY pumps
    - Test with missing [PUMPS] section
    - Test with malformed pump entries
    - _Requirements: 1.1, 1.5, 1.6_

- [x] 2. Extend parser to discover valve inputs (orifices and weirs)
  - [x] 2.1 Implement discover_orifices() function
    - Parse [CONTROLS] section to find orifices controlled by DUMMY curves
    - Return list of InputElement objects with object_type "ORIFICE" and property "SETTING"
    - Handle missing sections gracefully
    - _Requirements: 2.1, 2.3, 2.5, 2.6_
  
  - [x] 2.2 Implement discover_weirs() function
    - Parse [CONTROLS] section to find weirs controlled by DUMMY curves
    - Return list of InputElement objects with object_type "WEIR" and property "SETTING"
    - Handle missing sections gracefully
    - _Requirements: 2.2, 2.4, 2.5, 2.6_
  
  - [ ]* 2.3 Write property test for valve discovery
    - **Property 1: DUMMY Element Discovery (Valves)**
    - **Validates: Requirements 2.1, 2.2**
  
  - [ ]* 2.4 Write unit tests for valve discovery functions
    - Test orifices with DUMMY controls
    - Test weirs with DUMMY controls
    - Test with missing sections
    - _Requirements: 2.1, 2.2, 2.6_


- [x] 3. Extend parser to discover node inflow inputs
  - [x] 3.1 Implement discover_node_inflows() function
    - Parse [DWF] section to find nodes with DUMMY patterns
    - Return list of InputElement objects with object_type "NODE" and property "LATFLOW"
    - Handle missing [DWF] section gracefully
    - Avoid duplicate nodes if multiple DWF entries exist
    - _Requirements: 3.1, 3.2, 3.3, 3.5, 3.6_
  
  - [ ]* 3.2 Write property test for node inflow discovery
    - **Property 1: DUMMY Element Discovery (Nodes)**
    - **Validates: Requirements 3.1, 3.6**
  
  - [x]* 3.3 Write unit tests for discover_node_inflows()
    - Test nodes with DUMMY patterns
    - Test nodes with non-DUMMY patterns
    - Test with missing [DWF] section
    - Test duplicate node handling
    - _Requirements: 3.1, 3.5, 3.6_

- [x] 4. Refactor discover_inputs() to use priority-based ordering
  - [x] 4.1 Update discover_inputs() to call all discovery functions
    - Call discovery functions in priority order: rain gages, pumps, orifices, weirs, nodes
    - Assign sequential indices starting after elapsed time
    - Maintain elapsed time at index 0
    - _Requirements: 4.1, 4.2, 4.5_
  
  - [ ]* 4.2 Write property test for index ordering
    - **Property 3: Priority-Based Index Ordering**
    - **Validates: Requirements 4.1, 4.2, 4.5**
  
  - [ ]* 4.3 Write property test for intra-priority order preservation
    - **Property 4: Intra-Priority Order Preservation**
    - **Validates: Requirements 4.3**
  
  - [ ]* 4.4 Write unit tests for discover_inputs()
    - Test with mixed element types
    - Test with only rain gages (backward compatibility)
    - Test with no DUMMY elements
    - _Requirements: 4.1, 4.2, 4.3_

- [x] 5. Update generate_mapping_file() for new input types
  - [x] 5.1 Add property mapping for new object types
    - Map PUMP → SETTING
    - Map ORIFICE → SETTING
    - Map WEIR → SETTING
    - Map NODE → LATFLOW
    - Maintain existing GAGE → RAINFALL and SYSTEM → ELAPSEDTIME
    - _Requirements: 1.3, 2.5, 3.3_
  
  - [ ]* 5.2 Write property test for correct type and property assignment
    - **Property 2: Correct Type and Property Assignment**
    - **Validates: Requirements 1.2, 1.3, 2.3, 2.4, 2.5, 3.2, 3.3**
  
  - [ ]* 5.3 Write property test for mapping file index sorting
    - **Property 5: Mapping File Index Sorting**
    - **Validates: Requirements 4.4**
  
  - [ ]* 5.4 Write unit tests for generate_mapping_file()
    - Test JSON structure with new input types
    - Test property mapping correctness
    - Test index sorting in output
    - _Requirements: 1.3, 2.5, 3.3, 4.4_


- [x] 6. Checkpoint - Parser implementation complete
  - Ensure all parser tests pass, ask the user if questions arise.

- [x] 7. Extend Bridge DLL ValidateMapping() for new input types
  - [x] 7.1 Update ValidateMapping() to handle new object types
    - Add cases for PUMP, ORIFICE, WEIR (all use swmm_LINK)
    - Add case for NODE (uses swmm_NODE)
    - Maintain existing GAGE handling
    - Return descriptive errors for unknown object types
    - _Requirements: 6.1, 6.2, 6.3, 6.4, 6.5, 6.6_
  
  - [ ]* 7.2 Write unit tests for ValidateMapping()
    - Test resolution of each object type
    - Test error handling for missing elements
    - Test error handling for unknown object types
    - _Requirements: 6.1, 6.2, 6.3, 6.4, 6.5_

- [x] 8. Extend Bridge DLL Calculate() for new input types
  - [x] 8.1 Update Calculate() input processing loop
    - Add property mapping for PUMP/ORIFICE/WEIR → swmm_LINK_SETTING
    - Add property mapping for NODE → swmm_NODE_LATFLOW
    - Maintain existing GAGE → swmm_GAGE_RAINFALL
    - Return descriptive errors for unknown property combinations
    - _Requirements: 5.1, 5.2, 5.3, 5.4, 5.5, 5.6_
  
  - [ ]* 8.2 Write unit tests for Calculate() input processing
    - Test swmm_setValue calls for each input type
    - Test error handling for unknown properties
    - Test that correct SWMM API constants are used
    - _Requirements: 5.1, 5.2, 5.3, 5.4, 5.5, 5.6_

- [x] 9. Add error handling and validation
  - [x] 9.1 Implement descriptive error messages in Bridge DLL
    - Format: "Error: <description>\nContext: <element info>\nSuggestion: <fix>"
    - Add error messages for missing elements
    - Add error messages for unknown types
    - Add error messages for invalid properties
    - _Requirements: 8.1, 8.2, 8.3_
  
  - [x] 9.2 Add parser validation for DUMMY elements
    - Verify DUMMY elements exist in their respective sections
    - Skip invalid DUMMY references with warning
    - _Requirements: 8.6_
  
  - [ ]* 9.3 Write unit tests for error handling
    - Test error message format and content
    - Test parser validation of DUMMY elements
    - Test Bridge DLL error handling for various failure modes
    - _Requirements: 8.1, 8.2, 8.3, 8.6_


- [x] 10. Create test .inp files for integration testing
  - [x] 10.1 Create test_model_pumps.inp
    - Include pumps with DUMMY curves
    - Include pumps with non-DUMMY curves
    - _Requirements: 1.1, 1.6_
  
  - [x] 10.2 Create test_model_valves.inp
    - Include orifices and weirs with DUMMY controls
    - Include orifices and weirs with non-DUMMY controls
    - _Requirements: 2.1, 2.2_
  
  - [x] 10.3 Create test_model_nodes.inp
    - Include nodes with DUMMY DWF patterns
    - Include nodes with non-DUMMY patterns
    - _Requirements: 3.1, 3.6_
  
  - [x] 10.4 Create test_model_mixed.inp
    - Include all element types with DUMMY references
    - Test priority ordering and mixed discovery
    - _Requirements: 4.2, 7.4_
  
  - [x] 10.5 Create test_model_backward_compat.inp
    - Include only DUMMY rain gages (existing functionality)
    - Use for backward compatibility testing
    - _Requirements: 7.1, 7.3_

- [ ] 11. Integration and end-to-end testing
  - [ ]* 11.1 Write property test for backward compatibility
    - **Property 6: Backward Compatibility - Rain Gage Only Models**
    - **Validates: Requirements 7.1, 7.3**
  
  - [ ]* 11.2 Write property test for mixed element discovery
    - **Property 7: Mixed Element Discovery**
    - **Validates: Requirements 7.4**
  
  - [ ]* 11.3 Write property test for element validation
    - **Property 8: Element Validation**
    - **Validates: Requirements 8.6**
  
  - [ ]* 11.4 Write end-to-end integration tests
    - Test parser → mapping file → Bridge DLL flow
    - Test with each test .inp file
    - Verify correct swmm_setValue calls via mocking
    - _Requirements: 5.1, 5.2, 5.3, 5.4, 6.1, 6.2, 6.3, 6.4_
  
  - [ ]* 11.5 Write backward compatibility tests
    - Run existing test models through new parser
    - Verify output matches current implementation
    - Run existing test models through new Bridge DLL
    - Verify behavior is unchanged
    - _Requirements: 7.1, 7.2, 7.3, 7.5_

- [x] 12. Final checkpoint - Ensure all tests pass
  - Ensure all tests pass, ask the user if questions arise.

## Notes

- Tasks marked with `*` are optional and can be skipped for faster MVP
- Each task references specific requirements for traceability
- Checkpoints ensure incremental validation
- Property tests validate universal correctness properties across input space
- Unit tests validate specific examples, edge cases, and error conditions
- Integration tests validate the complete parser → DLL flow
