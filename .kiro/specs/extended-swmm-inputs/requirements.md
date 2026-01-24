# Requirements Document

## Introduction

This specification extends the SWMM-GoldSim bridge to support additional dynamic inputs beyond rain gages. The current implementation only allows GoldSim to control rain gage rainfall values during simulation. This extension enables GoldSim to dynamically control other SWMM elements such as pump flow rates, valve settings, and node inflows through the SWMM API.

The extension maintains backward compatibility with existing rain gage functionality while adding a flexible mechanism for marking any controllable SWMM element as a dynamic input from GoldSim.

## Glossary

- **Parser**: The Python script (generate_mapping.py) that analyzes SWMM .inp files and generates the mapping JSON file
- **Bridge_DLL**: The C++ dynamic library (SwmmGoldSimBridge.cpp) that interfaces between GoldSim and SWMM
- **Mapping_File**: The JSON file (SwmmGoldSimBridge.json) that defines the interface contract between GoldSim and SWMM
- **SWMM_API**: The EPA SWMM5 C API defined in swmm5.h
- **Dynamic_Input**: A SWMM element whose value is controlled by GoldSim during simulation rather than being static or time-series based
- **DUMMY_Convention**: The naming convention used to mark elements as controllable from GoldSim (e.g., TIMESERIES DUMMY for rain gages)
- **Control_Element**: A SWMM element (pump, valve, node) that can be dynamically controlled during simulation
- **GoldSim**: The simulation software that drives the SWMM model through the bridge DLL

## Requirements

### Requirement 1: Parser Discovery of Pump Controls

**User Story:** As a modeler, I want to mark pumps in my SWMM model as controllable from GoldSim, so that I can dynamically adjust pump flow rates during simulation.

#### Acceptance Criteria

1. WHEN a pump in the [PUMPS] section references a curve named "DUMMY", THE Parser SHALL identify it as a dynamic input
2. WHEN a DUMMY pump is discovered, THE Parser SHALL add it to the inputs array with object_type "PUMP"
3. WHEN a DUMMY pump is discovered, THE Parser SHALL add it to the inputs array with property "SETTING"
4. WHEN a DUMMY pump is discovered, THE Parser SHALL assign it a sequential interface index after elapsed time and rain gages
5. WHEN the [PUMPS] section is missing, THE Parser SHALL continue processing without error
6. WHEN a pump references a curve other than "DUMMY", THE Parser SHALL not include it as a dynamic input

### Requirement 2: Parser Discovery of Valve Controls

**User Story:** As a modeler, I want to mark valves (orifices, weirs) in my SWMM model as controllable from GoldSim, so that I can dynamically adjust valve settings during simulation.

#### Acceptance Criteria

1. WHEN an orifice in the [ORIFICES] section has a control curve named "DUMMY", THE Parser SHALL identify it as a dynamic input
2. WHEN a weir in the [WEIRS] section has a control curve named "DUMMY", THE Parser SHALL identify it as a dynamic input
3. WHEN a DUMMY orifice is discovered, THE Parser SHALL add it to the inputs array with object_type "ORIFICE"
4. WHEN a DUMMY weir is discovered, THE Parser SHALL add it to the inputs array with object_type "WEIR"
5. WHEN a DUMMY valve is discovered, THE Parser SHALL add it to the inputs array with property "SETTING"
6. WHEN valve sections are missing, THE Parser SHALL continue processing without error

### Requirement 3: Parser Discovery of Node Inflows

**User Story:** As a modeler, I want to mark nodes in my SWMM model as having dynamic inflows from GoldSim, so that I can control external inflows during simulation.

#### Acceptance Criteria

1. WHEN a node in the [DWF] section has a timeseries pattern named "DUMMY", THE Parser SHALL identify it as a dynamic input
2. WHEN a DUMMY node inflow is discovered, THE Parser SHALL add it to the inputs array with object_type "NODE"
3. WHEN a DUMMY node inflow is discovered, THE Parser SHALL add it to the inputs array with property "LATFLOW"
4. WHEN a DUMMY node inflow is discovered, THE Parser SHALL assign it a sequential interface index
5. WHEN the [DWF] section is missing, THE Parser SHALL continue processing without error
6. WHEN a node has a pattern other than "DUMMY", THE Parser SHALL not include it as a dynamic input

### Requirement 4: Input Priority and Ordering

**User Story:** As a developer, I want inputs to be ordered consistently, so that the interface contract is predictable and maintainable.

#### Acceptance Criteria

1. THE Parser SHALL always assign elapsed time to interface index 0
2. WHEN assigning interface indices, THE Parser SHALL use the priority order: elapsed time, rain gages, pumps, orifices, weirs, nodes
3. WHEN multiple elements exist within the same priority level, THE Parser SHALL preserve the order from the .inp file
4. WHEN generating the mapping file, THE Parser SHALL sort inputs by interface index in ascending order
5. THE Parser SHALL assign sequential indices starting from 0 with no gaps

### Requirement 5: Bridge DLL Input Mapping

**User Story:** As a bridge developer, I want the DLL to correctly map input types to SWMM API calls, so that values are set on the correct elements with the correct properties.

#### Acceptance Criteria

1. WHEN an input has object_type "PUMP" and property "SETTING", THE Bridge_DLL SHALL call swmm_setValue with swmm_LINK_SETTING
2. WHEN an input has object_type "ORIFICE" and property "SETTING", THE Bridge_DLL SHALL call swmm_setValue with swmm_LINK_SETTING
3. WHEN an input has object_type "WEIR" and property "SETTING", THE Bridge_DLL SHALL call swmm_setValue with swmm_LINK_SETTING
4. WHEN an input has object_type "NODE" and property "LATFLOW", THE Bridge_DLL SHALL call swmm_setValue with swmm_NODE_LATFLOW
5. WHEN an input has object_type "GAGE" and property "RAINFALL", THE Bridge_DLL SHALL call swmm_setValue with swmm_GAGE_RAINFALL
6. WHEN an input has an unknown object_type or property combination, THE Bridge_DLL SHALL return an error with a descriptive message

### Requirement 6: Bridge DLL Element Resolution

**User Story:** As a bridge developer, I want the DLL to resolve element names to SWMM indices correctly, so that values are set on the intended elements.

#### Acceptance Criteria

1. WHEN resolving a PUMP input, THE Bridge_DLL SHALL use swmm_getIndex with swmm_LINK
2. WHEN resolving an ORIFICE input, THE Bridge_DLL SHALL use swmm_getIndex with swmm_LINK
3. WHEN resolving a WEIR input, THE Bridge_DLL SHALL use swmm_getIndex with swmm_LINK
4. WHEN resolving a NODE input, THE Bridge_DLL SHALL use swmm_getIndex with swmm_NODE
5. WHEN an element name cannot be resolved, THE Bridge_DLL SHALL return an error identifying the missing element
6. THE Bridge_DLL SHALL resolve all input elements during initialization before the first Calculate call

### Requirement 7: Backward Compatibility

**User Story:** As a user with existing models, I want the extended bridge to work with my current rain gage models, so that I don't need to modify working configurations.

#### Acceptance Criteria

1. WHEN a model contains only DUMMY rain gages and no other dynamic inputs, THE Parser SHALL generate a mapping file identical to the current implementation
2. WHEN a model contains no DUMMY elements, THE Parser SHALL generate a mapping file with only elapsed time as input
3. WHEN the Bridge_DLL processes a mapping file with only rain gage inputs, THE Bridge_DLL SHALL behave identically to the current implementation
4. WHEN a model mixes DUMMY rain gages with other DUMMY elements, THE Parser SHALL discover all dynamic inputs correctly
5. THE Mapping_File format SHALL remain compatible with existing JSON structure

### Requirement 8: Error Handling and Validation

**User Story:** As a modeler, I want clear error messages when my model configuration is invalid, so that I can quickly identify and fix issues.

#### Acceptance Criteria

1. WHEN a DUMMY element name is not found in the SWMM model, THE Bridge_DLL SHALL return an error identifying the missing element and its type
2. WHEN an unsupported object_type is encountered, THE Bridge_DLL SHALL return an error listing the supported types
3. WHEN an unsupported property is encountered, THE Bridge_DLL SHALL return an error listing valid properties for that object type
4. WHEN the mapping file contains invalid JSON, THE Bridge_DLL SHALL return a descriptive parsing error
5. WHEN swmm_setValue fails, THE Bridge_DLL SHALL log the failure and continue processing remaining inputs
6. THE Parser SHALL validate that DUMMY elements reference valid sections before adding them to the mapping

### Requirement 9: Documentation and Examples

**User Story:** As a new user, I want clear documentation and examples, so that I can understand how to mark elements as controllable from GoldSim.

#### Acceptance Criteria

1. THE Parser SHALL output a summary showing discovered dynamic inputs by type
2. WHEN the Parser completes successfully, THE Parser SHALL print the count of each input type discovered
3. THE documentation SHALL include examples of DUMMY pump configurations
4. THE documentation SHALL include examples of DUMMY valve configurations
5. THE documentation SHALL include examples of DUMMY node inflow configurations
6. THE documentation SHALL explain the DUMMY convention and how it applies to each element type
