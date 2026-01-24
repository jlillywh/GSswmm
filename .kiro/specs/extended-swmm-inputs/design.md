# Design Document: Extended SWMM Inputs

## Overview

This design extends the SWMM-GoldSim bridge to support dynamic control of pumps, valves (orifices and weirs), and node inflows in addition to the existing rain gage functionality. The extension uses the same "DUMMY" convention established for rain gages, applying it consistently across different SWMM element types.

The design maintains backward compatibility with existing models while enabling new capabilities through minimal changes to the parser (generate_mapping.py) and bridge DLL (SwmmGoldSimBridge.cpp). The mapping file format remains unchanged, leveraging the existing flexible structure that already supports arbitrary object types and properties.

### Key Design Principles

1. **Consistency**: Use the DUMMY convention uniformly across all element types
2. **Backward Compatibility**: Existing rain gage models work without modification
3. **Extensibility**: The design allows future addition of other controllable elements
4. **Minimal Changes**: Leverage existing architecture rather than redesigning
5. **Clear Errors**: Provide actionable error messages for configuration issues

## Architecture

The system consists of three main components that work together:

```
┌─────────────────┐
│  SWMM .inp File │  (User marks elements with DUMMY)
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│     Parser      │  (generate_mapping.py)
│  - Discovers    │  - Scans [PUMPS], [ORIFICES], [WEIRS], [DWF]
│    DUMMY        │  - Identifies DUMMY references
│    elements     │  - Assigns interface indices by priority
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│  Mapping File   │  (SwmmGoldSimBridge.json)
│  - inputs[]     │  - Lists all dynamic inputs with types
│  - outputs[]    │  - Unchanged from current design
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│   Bridge DLL    │  (SwmmGoldSimBridge.cpp)
│  - Resolves     │  - Maps object_type to swmm_Object
│    element      │  - Maps property to swmm_Property
│    names        │  - Calls swmm_setValue() appropriately
│  - Sets values  │
└─────────────────┘
```


## Components and Interfaces

### Parser Component (generate_mapping.py)

The parser is extended with new discovery functions for each element type. The existing `discover_inputs()` function is refactored to call specialized discovery functions in priority order.

#### Extended discover_inputs() Function

```python
def discover_inputs(sections: Dict[str, List[List[str]]]) -> List[InputElement]:
    """
    Discover all dynamic input elements from parsed SWMM sections.
    
    Priority order:
    1. Elapsed time (always index 0)
    2. Rain gages with TIMESERIES DUMMY
    3. Pumps with Pcurve DUMMY
    4. Orifices with control curve DUMMY  
    5. Weirs with control curve DUMMY
    6. Nodes with DWF pattern DUMMY
    """
    inputs = []
    next_index = 0
    
    # Priority 1: Elapsed time (always first)
    inputs.append(InputElement("ElapsedTime", "SYSTEM", next_index))
    next_index += 1
    
    # Priority 2: Rain gages
    inputs.extend(discover_rain_gages(sections, next_index))
    next_index = len(inputs)
    
    # Priority 3: Pumps
    inputs.extend(discover_pumps(sections, next_index))
    next_index = len(inputs)
    
    # Priority 4: Orifices
    inputs.extend(discover_orifices(sections, next_index))
    next_index = len(inputs)
    
    # Priority 5: Weirs
    inputs.extend(discover_weirs(sections, next_index))
    next_index = len(inputs)
    
    # Priority 6: Node inflows
    inputs.extend(discover_node_inflows(sections, next_index))
    
    return inputs
```

#### New Discovery Functions

**discover_pumps()**
```python
def discover_pumps(sections: Dict[str, List[List[str]]], start_index: int) -> List[InputElement]:
    """
    Discover pumps with DUMMY curve references.
    
    PUMPS section format:
    Name FromNode ToNode Pcurve Status Startup Shutoff
    
    Example:
    P1 WET_WELL OUTLET DUMMY ON 0 0
    """
    inputs = []
    pumps = sections.get('PUMPS', [])
    index = start_index
    
    for pump_line in pumps:
        if len(pump_line) < 4:
            continue
        
        pump_name = pump_line[0]
        pcurve = pump_line[3]  # Pump curve name
        
        if pcurve == "DUMMY":
            inputs.append(InputElement(pump_name, "PUMP", index))
            index += 1
    
    return inputs
```


**discover_orifices()**
```python
def discover_orifices(sections: Dict[str, List[List[str]]], start_index: int) -> List[InputElement]:
    """
    Discover orifices with DUMMY control curve.
    
    ORIFICES section format (SWMM 5.2):
    Name FromNode ToNode Type Offset Qcoeff Gated CloseTime [Shape] [Height] [Width] [Coeff] [Flap] [Orate]
    
    Note: Control curves are not directly in the ORIFICES section.
    Instead, we look for orifices that would be controlled by CONTROLS section
    with DUMMY references. For simplicity, we use a naming convention:
    An orifice named with suffix "_DUMMY" or having a comment marker.
    
    Alternative: Check [CONTROLS] section for rules referencing DUMMY curves.
    """
    inputs = []
    # Implementation depends on how DUMMY is marked for orifices
    # Option 1: Naming convention (e.g., OR1_DUMMY)
    # Option 2: Check CONTROLS section for DUMMY curve references
    # Option 3: Use a custom comment marker in the inp file
    
    # For this design, we'll use Option 2: Check CONTROLS section
    orifices = sections.get('ORIFICES', [])
    controls = sections.get('CONTROLS', [])
    
    # Parse controls to find which orifices reference DUMMY
    dummy_orifices = set()
    for control_line in controls:
        # CONTROLS format varies, but typically:
        # RULE rulename
        # IF condition
        # THEN action
        # Example: THEN ORIFICE OR1 SETTING = CURVE DUMMY
        if 'DUMMY' in control_line and 'ORIFICE' in control_line:
            # Extract orifice name (simplified parsing)
            for i, token in enumerate(control_line):
                if token == 'ORIFICE' and i + 1 < len(control_line):
                    dummy_orifices.add(control_line[i + 1])
    
    index = start_index
    for orifice_line in orifices:
        if len(orifice_line) < 1:
            continue
        
        orifice_name = orifice_line[0]
        if orifice_name in dummy_orifices:
            inputs.append(InputElement(orifice_name, "ORIFICE", index))
            index += 1
    
    return inputs
```

**discover_weirs()**
```python
def discover_weirs(sections: Dict[str, List[List[str]]], start_index: int) -> List[InputElement]:
    """
    Discover weirs with DUMMY control curve.
    
    Similar to orifices, weirs are controlled via CONTROLS section.
    """
    inputs = []
    weirs = sections.get('WEIRS', [])
    controls = sections.get('CONTROLS', [])
    
    # Parse controls to find which weirs reference DUMMY
    dummy_weirs = set()
    for control_line in controls:
        if 'DUMMY' in control_line and 'WEIR' in control_line:
            for i, token in enumerate(control_line):
                if token == 'WEIR' and i + 1 < len(control_line):
                    dummy_weirs.add(control_line[i + 1])
    
    index = start_index
    for weir_line in weirs:
        if len(weir_line) < 1:
            continue
        
        weir_name = weir_line[0]
        if weir_name in dummy_weirs:
            inputs.append(InputElement(weir_name, "WEIR", index))
            index += 1
    
    return inputs
```


**discover_node_inflows()**
```python
def discover_node_inflows(sections: Dict[str, List[List[str]]], start_index: int) -> List[InputElement]:
    """
    Discover nodes with DUMMY dry weather flow patterns.
    
    DWF section format:
    Node Constituent AvgValue Pattern1 Pattern2 Pattern3 Pattern4
    
    Example:
    J1 FLOW 1.0 DUMMY "" "" ""
    
    We look for nodes where any pattern is named "DUMMY".
    """
    inputs = []
    dwf_entries = sections.get('DWF', [])
    index = start_index
    
    # Track which nodes have DUMMY patterns (avoid duplicates)
    dummy_nodes = set()
    
    for dwf_line in dwf_entries:
        if len(dwf_line) < 4:
            continue
        
        node_name = dwf_line[0]
        # Patterns are in positions 3, 4, 5, 6 (if they exist)
        patterns = dwf_line[3:7] if len(dwf_line) >= 7 else dwf_line[3:]
        
        # Check if any pattern is DUMMY
        if "DUMMY" in patterns:
            if node_name not in dummy_nodes:
                inputs.append(InputElement(node_name, "NODE", index))
                dummy_nodes.add(node_name)
                index += 1
    
    return inputs
```

#### Updated generate_mapping_file()

The mapping file generation function needs to handle the new input types by mapping them to appropriate property names:

```python
def generate_mapping_file(inputs, outputs, content_hash, output_path):
    """Generate JSON mapping with extended input types."""
    mapping = {
        "version": "1.0",
        "inp_file_hash": content_hash,
        "input_count": len(inputs),
        "output_count": len(outputs),
        "inputs": [],
        "outputs": []
    }
    
    # Property mapping for each object type
    property_map = {
        "SYSTEM": "ELAPSEDTIME",
        "GAGE": "RAINFALL",
        "PUMP": "SETTING",
        "ORIFICE": "SETTING",
        "WEIR": "SETTING",
        "NODE": "LATFLOW"
    }
    
    for inp in inputs:
        property_name = property_map.get(inp.object_type, "UNKNOWN")
        input_entry = {
            "index": inp.index,
            "name": inp.name,
            "object_type": inp.object_type,
            "property": property_name
        }
        mapping["inputs"].append(input_entry)
    
    # Output handling unchanged
    for out in outputs:
        # ... existing output logic ...
    
    with open(output_path, 'w', encoding='utf-8') as f:
        json.dump(mapping, f, indent=2)
    
    return True
```


### Bridge DLL Component (SwmmGoldSimBridge.cpp)

The bridge DLL requires modifications to the `ValidateMapping()` and `Calculate()` methods to handle the new input types.

#### Extended ValidateMapping()

```cpp
bool SwmmSimulation::ValidateMapping(const MappingLoader& mapping, std::string& error) {
    Logger::Debug("=== ValidateMapping ===");
    
    input_indices_.clear();
    output_indices_.clear();
    
    const std::vector<MappingLoader::InputMapping>& inputs = mapping.GetInputs();
    
    for (size_t i = 0; i < inputs.size(); i++) {
        const MappingLoader::InputMapping& input = inputs[i];
        
        // Skip system properties
        if (input.object_type == "SYSTEM") {
            input_indices_.push_back(-1);
            continue;
        }
        
        // Determine SWMM object type for resolution
        int swmm_obj_type = -1;
        if (input.object_type == "GAGE") {
            swmm_obj_type = swmm_GAGE;
        } 
        else if (input.object_type == "PUMP" || 
                 input.object_type == "ORIFICE" || 
                 input.object_type == "WEIR") {
            swmm_obj_type = swmm_LINK;
        }
        else if (input.object_type == "NODE") {
            swmm_obj_type = swmm_NODE;
        }
        else {
            error = "Unknown input object type: " + input.object_type;
            return false;
        }
        
        // Resolve element name to SWMM index
        int swmm_index = swmm_getIndex(swmm_obj_type, input.name.c_str());
        if (swmm_index < 0) {
            error = "SWMM element not found: " + input.name + 
                    " (type: " + input.object_type + ")";
            return false;
        }
        
        Logger::Debug("Input %d: %s resolved to SWMM index %d", 
                     static_cast<int>(i), input.name.c_str(), swmm_index);
        input_indices_.push_back(swmm_index);
    }
    
    // Output validation unchanged
    // ...
    
    return true;
}
```


#### Extended Calculate() - Input Processing

```cpp
bool SwmmSimulation::Calculate(const double* inargs, double* outargs, 
                               const MappingLoader& mapping, std::string& error) {
    // ... existing elapsed time handling ...
    
    const std::vector<MappingLoader::InputMapping>& inputs = mapping.GetInputs();
    
    for (size_t i = 0; i < inputs.size(); i++) {
        const MappingLoader::InputMapping& input = inputs[i];
        
        // Skip system properties
        if (input.object_type == "SYSTEM") {
            continue;
        }
        
        // Get value from GoldSim
        double value = inargs[input.interface_index];
        
        // Map object_type and property to SWMM API constant
        int swmm_property = -1;
        
        if (input.object_type == "GAGE" && input.property == "RAINFALL") {
            swmm_property = swmm_GAGE_RAINFALL;
        }
        else if ((input.object_type == "PUMP" || 
                  input.object_type == "ORIFICE" || 
                  input.object_type == "WEIR") && 
                 input.property == "SETTING") {
            swmm_property = swmm_LINK_SETTING;
        }
        else if (input.object_type == "NODE" && input.property == "LATFLOW") {
            swmm_property = swmm_NODE_LATFLOW;
        }
        else {
            error = "Unknown property: " + input.object_type + "." + input.property;
            Logger::Debug("ERROR: %s", error.c_str());
            return false;
        }
        
        // Validate resolved index
        if (input_indices_[i] < 0) {
            error = "Invalid handle for input element: " + input.name;
            return false;
        }
        
        // Set value via SWMM API
        int swmm_index = input_indices_[i];
        swmm_setValue(swmm_property, swmm_index, value);
        
        Logger::Debug("Input %d: Set %s[%d].%s = %.6f", 
                     static_cast<int>(i), input.object_type.c_str(), 
                     swmm_index, input.property.c_str(), value);
    }
    
    // ... existing SWMM stepping and output retrieval ...
    
    return true;
}
```


## Data Models

### InputElement (Parser)

```python
@dataclass
class InputElement:
    """Represents a dynamic input element."""
    name: str           # Element name from SWMM (e.g., "P1", "OR1", "J1")
    object_type: str    # SWMM object type: "SYSTEM", "GAGE", "PUMP", "ORIFICE", "WEIR", "NODE"
    index: int          # Interface index in GoldSim inargs array (0-based)
```

### Mapping File JSON Schema

```json
{
  "version": "1.0",
  "inp_file_hash": "string (MD5 hash)",
  "input_count": "integer",
  "output_count": "integer",
  "inputs": [
    {
      "index": "integer (0-based interface position)",
      "name": "string (element name)",
      "object_type": "string (SYSTEM|GAGE|PUMP|ORIFICE|WEIR|NODE)",
      "property": "string (ELAPSEDTIME|RAINFALL|SETTING|LATFLOW)"
    }
  ],
  "outputs": [
    "... unchanged from current design ..."
  ]
}
```

### Example Mapping File

```json
{
  "version": "1.0",
  "inp_file_hash": "abc123...",
  "input_count": 5,
  "output_count": 2,
  "inputs": [
    {
      "index": 0,
      "name": "ElapsedTime",
      "object_type": "SYSTEM",
      "property": "ELAPSEDTIME"
    },
    {
      "index": 1,
      "name": "RG1",
      "object_type": "GAGE",
      "property": "RAINFALL"
    },
    {
      "index": 2,
      "name": "P1",
      "object_type": "PUMP",
      "property": "SETTING"
    },
    {
      "index": 3,
      "name": "OR1",
      "object_type": "ORIFICE",
      "property": "SETTING"
    },
    {
      "index": 4,
      "name": "J1",
      "object_type": "NODE",
      "property": "LATFLOW"
    }
  ],
  "outputs": [
    {
      "index": 0,
      "name": "OUT1",
      "object_type": "OUTFALL",
      "property": "FLOW",
      "swmm_index": 0
    },
    {
      "index": 1,
      "name": "POND1",
      "object_type": "STORAGE",
      "property": "VOLUME",
      "swmm_index": 0
    }
  ]
}
```


## Correctness Properties

A property is a characteristic or behavior that should hold true across all valid executions of a system—essentially, a formal statement about what the system should do. Properties serve as the bridge between human-readable specifications and machine-verifiable correctness guarantees.

### Property 1: DUMMY Element Discovery

*For any* SWMM .inp file containing elements (pumps, orifices, weirs, nodes) that reference "DUMMY" curves or patterns, the Parser should discover all and only those elements that have DUMMY references, regardless of how many non-DUMMY elements exist.

**Validates: Requirements 1.1, 1.6, 2.1, 2.2, 3.1, 3.6**

### Property 2: Correct Type and Property Assignment

*For any* discovered dynamic input element, the Parser should assign the correct object_type and property based on the element's section: PUMP→(PUMP, SETTING), ORIFICE→(ORIFICE, SETTING), WEIR→(WEIR, SETTING), NODE→(NODE, LATFLOW), GAGE→(GAGE, RAINFALL).

**Validates: Requirements 1.2, 1.3, 2.3, 2.4, 2.5, 3.2, 3.3**

### Property 3: Priority-Based Index Ordering

*For any* SWMM .inp file with multiple types of dynamic inputs, the Parser should assign interface indices following the strict priority order: elapsed time (0), then rain gages, then pumps, then orifices, then weirs, then nodes, with sequential numbering and no gaps.

**Validates: Requirements 4.1, 4.2, 4.5**

### Property 4: Intra-Priority Order Preservation

*For any* SWMM .inp file with multiple elements of the same type (e.g., multiple pumps), the Parser should preserve the order in which those elements appear in the .inp file when assigning interface indices.

**Validates: Requirements 4.3**

### Property 5: Mapping File Index Sorting

*For any* generated mapping file, the inputs array should be sorted by interface index in ascending order (0, 1, 2, ...).

**Validates: Requirements 4.4**


### Property 6: Backward Compatibility - Rain Gage Only Models

*For any* SWMM .inp file containing only DUMMY rain gages and no other DUMMY elements, the Parser should generate a mapping file with the same structure and content as the current implementation (elapsed time + rain gages only).

**Validates: Requirements 7.1, 7.3**

### Property 7: Mixed Element Discovery

*For any* SWMM .inp file containing a mix of DUMMY rain gages, pumps, valves, and nodes, the Parser should discover all DUMMY elements correctly without omissions or duplicates.

**Validates: Requirements 7.4**

### Property 8: Element Validation

*For any* discovered DUMMY element, the Parser should only include it in the mapping if it references a valid section in the .inp file (e.g., a DUMMY pump must exist in the [PUMPS] section).

**Validates: Requirements 8.6**

## Error Handling

### Error Categories

1. **Parser Errors**
   - Malformed .inp file sections
   - Invalid DUMMY references (element doesn't exist in section)
   - JSON generation failures

2. **Bridge DLL Errors**
   - Element name not found in SWMM model
   - Unknown object_type or property combination
   - Invalid JSON in mapping file
   - swmm_setValue failures

### Error Message Format

All error messages should follow this format:
```
Error: <brief description>
Context: <element name, type, or file location>
Suggestion: <actionable fix if applicable>
```

Example:
```
Error: SWMM element not found
Context: Pump "P1" (type: PUMP)
Suggestion: Verify that pump P1 exists in the [PUMPS] section of the .inp file
```

### Error Handling Strategy

1. **Parser**: Fail fast on structural errors (malformed .inp), continue on missing optional sections
2. **Bridge DLL**: Fail on initialization errors (missing elements), log and continue on runtime errors (swmm_setValue failures)
3. **Validation**: Validate mapping file structure and element existence during Initialize, not Calculate


## Testing Strategy

### Dual Testing Approach

This feature requires both unit tests and property-based tests to ensure comprehensive coverage:

- **Unit tests**: Verify specific examples, edge cases (missing sections), error conditions, and API mappings
- **Property tests**: Verify universal properties across all possible input combinations

Both testing approaches are complementary and necessary. Unit tests catch concrete bugs in specific scenarios, while property tests verify general correctness across the input space.

### Property-Based Testing

We will use **Hypothesis** (Python) for parser tests and **GoogleTest with custom generators** (C++) for bridge DLL tests.

**Configuration**:
- Minimum 100 iterations per property test
- Each test tagged with: `Feature: extended-swmm-inputs, Property N: <property text>`
- Tests should generate random .inp file structures with varying combinations of DUMMY elements

**Property Test Examples**:

1. **DUMMY Discovery Property** (Property 1)
   - Generate: Random .inp files with 0-10 elements of each type, some with DUMMY, some without
   - Verify: Only DUMMY elements are discovered

2. **Index Ordering Property** (Property 3)
   - Generate: Random combinations of all element types
   - Verify: Indices follow priority order and are sequential

3. **Backward Compatibility Property** (Property 6)
   - Generate: .inp files with only rain gages
   - Verify: Output matches current implementation format

### Unit Testing

**Parser Unit Tests** (Python):
- Test each discovery function independently
- Test missing section handling (edge cases)
- Test malformed .inp file handling
- Test JSON generation with various input combinations

**Bridge DLL Unit Tests** (C++):
- Test API constant mapping for each object_type/property pair
- Test element resolution for each object type
- Test error messages for missing elements
- Test error messages for unknown types
- Test Initialize/Calculate lifecycle with new input types

### Integration Testing

**End-to-End Tests**:
1. Create test .inp files with all element types
2. Run parser to generate mapping file
3. Run bridge DLL with mapping file and mock SWMM API
4. Verify correct swmm_setValue calls with correct parameters

**Backward Compatibility Tests**:
1. Run existing test models through new parser
2. Verify output is identical to current implementation
3. Run existing test models through new bridge DLL
4. Verify behavior is identical to current implementation

### Test Data

Create test .inp files covering:
- Rain gages only (backward compatibility)
- Pumps only
- Valves only (orifices and weirs)
- Nodes only
- Mixed combinations
- Empty model (no DUMMY elements)
- Maximum complexity (all types present)

