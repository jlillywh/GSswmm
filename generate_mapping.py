#!/usr/bin/env python3
"""
SWMM-GoldSim Bridge Mapping Generator

This script parses SWMM .inp files and generates a JSON mapping file
that defines the interface between GoldSim and the SWMM Bridge DLL.

Usage:
    python generate_mapping.py <path_to_swmm_inp_file>

Example:
    python generate_mapping.py tests/test_model.inp

The script will generate SwmmGoldSimBridge.json in the current directory.
"""

import sys
import os
import argparse
import hashlib
import json
from dataclasses import dataclass
from typing import List, Dict, Tuple, Optional


@dataclass
class InputElement:
    """
    Represents an input element in the GoldSim-SWMM interface.
    
    Attributes:
        name: Element name from SWMM (or "ElapsedTime" for the time input)
        object_type: SWMM object type ("SYSTEM" for elapsed time, "GAGE" for rain gages)
        index: Interface index in the GoldSim inargs array (0-based)
    """
    name: str
    object_type: str
    index: int


@dataclass
class OutputElement:
    """
    Represents an output element in the GoldSim-SWMM interface.
    
    Attributes:
        name: Element name from SWMM
        object_type: SWMM object type ("STORAGE", "OUTFALL", "ORIFICE", "WEIR", "SUBCATCH")
        value_type: Type of value being reported ("VOLUME", "FLOW", "RUNOFF")
        index: Interface index in the GoldSim outargs array (0-based)
    """
    name: str
    object_type: str
    value_type: str
    index: int


def validate_file_exists(filepath):
    """
    Validate that the specified file exists.
    
    Args:
        filepath: Path to the file to validate
        
    Returns:
        True if file exists, False otherwise
    """
    if not os.path.exists(filepath):
        print(f"Error: File not found: {filepath}", file=sys.stderr)
        return False
    
    if not os.path.isfile(filepath):
        print(f"Error: Path is not a file: {filepath}", file=sys.stderr)
        return False
    
    return True


def print_usage():
    """
    Print usage instructions when no arguments are provided.
    """
    print(__doc__)


def parse_arguments():
    """
    Parse command-line arguments.
    
    Returns:
        Parsed arguments namespace, or None if no arguments provided
    """
    parser = argparse.ArgumentParser(
        description='Generate GoldSim-SWMM Bridge mapping file from SWMM .inp file',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python generate_mapping.py tests/test_model.inp
  python generate_mapping.py model.inp

The script will generate SwmmGoldSimBridge.json in the current directory.
        """
    )
    
    parser.add_argument(
        'inp_file',
        nargs='?',
        help='Path to SWMM .inp file'
    )
    
    args = parser.parse_args()
    
    # If no arguments provided, print usage and return None
    if args.inp_file is None:
        print_usage()
        return None
    
    return args


def parse_inp_file(inp_path):
    """
    Parse SWMM .inp file and extract sections.
    
    Args:
        inp_path: Path to the SWMM .inp file
        
    Returns:
        Tuple of (sections_dict, error_message).
        sections_dict: Dictionary mapping section names to lists of data lines.
                      Each data line is a list of whitespace-separated fields.
        error_message: None if successful, error string if parsing failed.
    """
    sections = {}
    current_section = None
    
    try:
        with open(inp_path, 'r', encoding='utf-8') as f:
            for line_num, line in enumerate(f, 1):
                # Strip whitespace from both ends
                original_line = line
                line = line.strip()
                
                # Skip empty lines
                if not line:
                    continue
                
                # Skip comment lines (starting with semicolon)
                if line.startswith(';'):
                    continue
                
                # Check for potential section header (starts with '[')
                if line.startswith('['):
                    # Validate that it's a properly formed section header
                    if not line.endswith(']'):
                        error_msg = f"Malformed section header at line {line_num}: '{line}' - missing closing bracket ']'"
                        print(f"Error: {error_msg}", file=sys.stderr)
                        return {}, error_msg
                    
                    # Check for empty section name
                    section_name = line[1:-1].strip()
                    if not section_name:
                        error_msg = f"Empty section name at line {line_num}: '{line}'"
                        print(f"Error: {error_msg}", file=sys.stderr)
                        return {}, error_msg
                    
                    # Extract section name (remove brackets)
                    current_section = section_name
                    # Initialize section with empty list if not exists
                    if current_section not in sections:
                        sections[current_section] = []
                    continue
                
                # Check for lines that look like they might be malformed section headers
                # (contain '[' or ']' but not in the right format)
                if '[' in line or ']' in line:
                    # If we see brackets in the middle of a line, it might be malformed
                    if not (line.startswith('[') and line.endswith(']')):
                        error_msg = f"Invalid syntax at line {line_num}: '{line}' - unexpected bracket character"
                        print(f"Error: {error_msg}", file=sys.stderr)
                        return {}, error_msg
                
                # If we're in a section, parse the data line
                if current_section is not None:
                    # Split by whitespace and filter out empty strings
                    fields = line.split()
                    if fields:  # Only add non-empty lines
                        sections[current_section].append(fields)
        
        return sections, None
        
    except IOError as e:
        error_msg = f"Error reading file {inp_path}: {e}"
        print(error_msg, file=sys.stderr)
        return {}, error_msg
    except Exception as e:
        error_msg = f"Error parsing file {inp_path}: {e}"
        print(error_msg, file=sys.stderr)
        return {}, error_msg


def validate_xsections(sections: Dict[str, List[List[str]]]) -> List[str]:
    """
    Validate XSECTIONS entries for common formatting errors.
    
    Returns:
        List of warning/error messages (empty if no issues found)
    """
    issues = []
    xsections = sections.get('XSECTIONS', [])
    
    # Build sets of valid link names from different sections
    conduits = {line[0] for line in sections.get('CONDUITS', []) if len(line) >= 1}
    orifices = {line[0] for line in sections.get('ORIFICES', []) if len(line) >= 1}
    weirs = {line[0] for line in sections.get('WEIRS', []) if len(line) >= 1}
    pumps = {line[0] for line in sections.get('PUMPS', []) if len(line) >= 1}
    
    all_links = conduits | orifices | weirs | pumps
    
    for xsect_line in xsections:
        if len(xsect_line) < 2:
            continue
        
        link_name = xsect_line[0]
        shape = xsect_line[1]
        
        # Check if link exists
        if link_name not in all_links:
            issues.append(f"WARNING: XSECTION for '{link_name}' but link not found in CONDUITS, ORIFICES, WEIRS, or PUMPS")
            continue
        
        # Validate parameter count based on shape and link type
        param_count = len(xsect_line) - 2  # Subtract link name and shape
        
        # Special validation for weirs with RECT_OPEN
        if link_name in weirs and shape == 'RECT_OPEN':
            if param_count < 2:
                issues.append(f"ERROR: Weir '{link_name}' with RECT_OPEN needs at least 2 parameters (height, width), found {param_count}")
                issues.append(f"  Current line: {' '.join(xsect_line)}")
                issues.append(f"  Expected format: {link_name} RECT_OPEN <height> <width> [side_slope_left] [side_slope_right]")
            elif param_count == 3:
                issues.append(f"ERROR: Weir '{link_name}' with RECT_OPEN has 3 parameters - SWMM expects 2 or 4")
                issues.append(f"  Current line: {' '.join(xsect_line)}")
                issues.append(f"  Fix: Either use 2 params (height width) or 4 params (height width slope_left slope_right)")
        
        # Validate orifices with CIRCULAR
        if link_name in orifices and shape == 'CIRCULAR':
            if param_count < 1:
                issues.append(f"ERROR: Orifice '{link_name}' with CIRCULAR needs at least 1 parameter (diameter), found {param_count}")
        
        # Validate conduits
        if link_name in conduits:
            if shape == 'CIRCULAR' and param_count < 1:
                issues.append(f"ERROR: Conduit '{link_name}' with CIRCULAR needs at least 1 parameter (diameter), found {param_count}")
            elif shape == 'RECT_OPEN' and param_count < 2:
                issues.append(f"ERROR: Conduit '{link_name}' with RECT_OPEN needs at least 2 parameters, found {param_count}")
    
    return issues


def validate_required_sections(sections: Dict[str, List[List[str]]]) -> List[str]:
    """
    Validate that required SWMM sections exist.
    
    Returns:
        List of warning/error messages (empty if no issues found)
    """
    issues = []
    
    # Check for essential sections
    required = ['OPTIONS', 'RAINGAGES', 'SUBCATCHMENTS', 'SUBAREAS', 'INFILTRATION']
    for section in required:
        if section not in sections or len(sections[section]) == 0:
            issues.append(f"WARNING: Missing or empty [{section}] section - model may not run")
    
    # Check for at least one outlet
    has_outlet = 'OUTFALLS' in sections and len(sections['OUTFALLS']) > 0
    if not has_outlet:
        issues.append("ERROR: No outfalls defined - SWMM requires at least one outlet node")
    
    return issues


def validate_node_references(sections: Dict[str, List[List[str]]]) -> List[str]:
    """
    Validate that links reference valid nodes.
    
    Returns:
        List of warning/error messages (empty if no issues found)
    """
    issues = []
    
    # Build set of all valid nodes
    valid_nodes = set()
    for section_name in ['JUNCTIONS', 'STORAGE', 'OUTFALLS']:
        nodes = sections.get(section_name, [])
        for node_line in nodes:
            if len(node_line) >= 1:
                valid_nodes.add(node_line[0])
    
    if not valid_nodes:
        issues.append("WARNING: No nodes defined in model")
        return issues
    
    # Check conduits
    for conduit_line in sections.get('CONDUITS', []):
        if len(conduit_line) >= 3:
            link_name = conduit_line[0]
            from_node = conduit_line[1]
            to_node = conduit_line[2]
            
            if from_node not in valid_nodes:
                issues.append(f"ERROR: Conduit '{link_name}' references non-existent from-node '{from_node}'")
            if to_node not in valid_nodes:
                issues.append(f"ERROR: Conduit '{link_name}' references non-existent to-node '{to_node}'")
    
    # Check orifices
    for orifice_line in sections.get('ORIFICES', []):
        if len(orifice_line) >= 3:
            link_name = orifice_line[0]
            from_node = orifice_line[1]
            to_node = orifice_line[2]
            
            if from_node not in valid_nodes:
                issues.append(f"ERROR: Orifice '{link_name}' references non-existent from-node '{from_node}'")
            if to_node not in valid_nodes:
                issues.append(f"ERROR: Orifice '{link_name}' references non-existent to-node '{to_node}'")
    
    # Check weirs
    for weir_line in sections.get('WEIRS', []):
        if len(weir_line) >= 3:
            link_name = weir_line[0]
            from_node = weir_line[1]
            to_node = weir_line[2]
            
            if from_node not in valid_nodes:
                issues.append(f"ERROR: Weir '{link_name}' references non-existent from-node '{from_node}'")
            if to_node not in valid_nodes:
                issues.append(f"ERROR: Weir '{link_name}' references non-existent to-node '{to_node}'")
    
    return issues


def validate_inp_file(sections: Dict[str, List[List[str]]]) -> Tuple[bool, List[str]]:
    """
    Run all validation checks on the parsed .inp file.
    
    Args:
        sections: Dictionary of parsed SWMM sections
        
    Returns:
        Tuple of (has_errors, messages)
        has_errors: True if any ERROR-level issues found
        messages: List of all warning and error messages
    """
    all_issues = []
    
    # Run validation checks
    all_issues.extend(validate_required_sections(sections))
    all_issues.extend(validate_node_references(sections))
    all_issues.extend(validate_xsections(sections))
    
    # Check if any are errors (vs warnings)
    has_errors = any('ERROR:' in msg for msg in all_issues)
    
    return has_errors, all_issues


def discover_pumps(sections: Dict[str, List[List[str]]], start_index: int) -> List[InputElement]:
    """
    Discover pumps with DUMMY curve references.
    
    PUMPS section format:
    Name FromNode ToNode Pcurve Status Startup Shutoff
    
    Example:
    P1 WET_WELL OUTLET DUMMY ON 0 0
    
    Args:
        sections: Dictionary of parsed SWMM sections
        start_index: Starting interface index for discovered pumps
        
    Returns:
        List of InputElement objects with object_type "PUMP" and property "SETTING"
    """
    inputs = []
    pumps = sections.get('PUMPS', [])
    index = start_index
    
    for pump_line in pumps:
        # PUMPS format requires at least 4 fields: Name FromNode ToNode Pcurve
        if len(pump_line) < 4:
            continue
        
        pump_name = pump_line[0]
        pcurve = pump_line[3]  # Pump curve name
        
        # Requirement 1.1, 1.6: Filter for pumps with Pcurve "DUMMY"
        if pcurve == "DUMMY":
            # Requirement 8.6: Validate that the pump exists in the [PUMPS] section
            # (it does by definition since we're iterating through the section)
            # Requirement 1.2, 1.3: Add with object_type "PUMP" and property "SETTING"
            inputs.append(InputElement(pump_name, "PUMP", index))
            index += 1
    
    # Requirement 1.5: Missing [PUMPS] section is handled gracefully by sections.get()
    return inputs


def discover_orifices(sections: Dict[str, List[List[str]]], start_index: int) -> List[InputElement]:
    """
    Discover orifices with DUMMY control curve references.
    
    Orifices are controlled via the [CONTROLS] section. We parse the CONTROLS
    section to find rules that reference DUMMY curves for orifice settings.
    
    CONTROLS section format (example):
    RULE R1
    IF NODE J1 DEPTH > 5
    THEN ORIFICE OR1 SETTING = CURVE DUMMY
    
    Args:
        sections: Dictionary of parsed SWMM sections
        start_index: Starting interface index for discovered orifices
        
    Returns:
        List of InputElement objects with object_type "ORIFICE" and property "SETTING"
    """
    inputs = []
    orifices = sections.get('ORIFICES', [])
    controls = sections.get('CONTROLS', [])
    
    # Build a set of valid orifice names for validation (Requirement 8.6)
    valid_orifices = set()
    for orifice_line in orifices:
        if len(orifice_line) >= 1:
            valid_orifices.add(orifice_line[0])
    
    # Parse controls to find which orifices reference DUMMY
    dummy_orifices = set()
    for control_line in controls:
        # Look for lines containing both "ORIFICE" and "DUMMY"
        # Example: THEN ORIFICE OR1 SETTING = CURVE DUMMY
        if 'DUMMY' in control_line and 'ORIFICE' in control_line:
            # Extract orifice name (it follows the ORIFICE keyword)
            for i, token in enumerate(control_line):
                if token == 'ORIFICE' and i + 1 < len(control_line):
                    orifice_name = control_line[i + 1]
                    # Requirement 8.6: Validate that the orifice exists in [ORIFICES] section
                    if orifice_name in valid_orifices:
                        dummy_orifices.add(orifice_name)
                    else:
                        print(f"Warning: Skipping DUMMY reference to orifice '{orifice_name}' - not found in [ORIFICES] section", 
                              file=sys.stderr)
                    break
    
    # Requirement 2.6: Missing sections handled gracefully by sections.get()
    index = start_index
    for orifice_line in orifices:
        # ORIFICES format requires at least the name (first field)
        if len(orifice_line) < 1:
            continue
        
        orifice_name = orifice_line[0]
        
        # Requirement 2.1: Filter for orifices with DUMMY control curves
        if orifice_name in dummy_orifices:
            # Requirement 2.3, 2.5: Add with object_type "ORIFICE" and property "SETTING"
            inputs.append(InputElement(orifice_name, "ORIFICE", index))
            index += 1
    
    return inputs


def discover_weirs(sections: Dict[str, List[List[str]]], start_index: int) -> List[InputElement]:
    """
    Discover weirs with DUMMY control curve references.
    
    Weirs are controlled via the [CONTROLS] section. We parse the CONTROLS
    section to find rules that reference DUMMY curves for weir settings.
    
    CONTROLS section format (example):
    RULE R2
    IF NODE J2 DEPTH > 3
    THEN WEIR W1 SETTING = CURVE DUMMY
    
    Args:
        sections: Dictionary of parsed SWMM sections
        start_index: Starting interface index for discovered weirs
        
    Returns:
        List of InputElement objects with object_type "WEIR" and property "SETTING"
    """
    inputs = []
    weirs = sections.get('WEIRS', [])
    controls = sections.get('CONTROLS', [])
    
    # Build a set of valid weir names for validation (Requirement 8.6)
    valid_weirs = set()
    for weir_line in weirs:
        if len(weir_line) >= 1:
            valid_weirs.add(weir_line[0])
    
    # Parse controls to find which weirs reference DUMMY
    dummy_weirs = set()
    for control_line in controls:
        # Look for lines containing both "WEIR" and "DUMMY"
        # Example: THEN WEIR W1 SETTING = CURVE DUMMY
        if 'DUMMY' in control_line and 'WEIR' in control_line:
            # Extract weir name (it follows the WEIR keyword)
            for i, token in enumerate(control_line):
                if token == 'WEIR' and i + 1 < len(control_line):
                    weir_name = control_line[i + 1]
                    # Requirement 8.6: Validate that the weir exists in [WEIRS] section
                    if weir_name in valid_weirs:
                        dummy_weirs.add(weir_name)
                    else:
                        print(f"Warning: Skipping DUMMY reference to weir '{weir_name}' - not found in [WEIRS] section", 
                              file=sys.stderr)
                    break
    
    # Requirement 2.6: Missing sections handled gracefully by sections.get()
    index = start_index
    for weir_line in weirs:
        # WEIRS format requires at least the name (first field)
        if len(weir_line) < 1:
            continue
        
        weir_name = weir_line[0]
        
        # Requirement 2.2: Filter for weirs with DUMMY control curves
        if weir_name in dummy_weirs:
            # Requirement 2.4, 2.5: Add with object_type "WEIR" and property "SETTING"
            inputs.append(InputElement(weir_name, "WEIR", index))
            index += 1
    
    return inputs


def discover_node_inflows(sections: Dict[str, List[List[str]]], start_index: int) -> List[InputElement]:
    """
    Discover nodes with DUMMY dry weather flow patterns.
    
    DWF section format:
    Node Constituent AvgValue Pattern1 Pattern2 Pattern3 Pattern4
    
    Example:
    J1 FLOW 1.0 DUMMY "" "" ""
    
    We look for nodes where any pattern is named "DUMMY".
    
    Args:
        sections: Dictionary of parsed SWMM sections
        start_index: Starting interface index for discovered nodes
        
    Returns:
        List of InputElement objects with object_type "NODE" and property "LATFLOW"
    """
    inputs = []
    dwf_entries = sections.get('DWF', [])
    index = start_index
    
    # Build a set of valid node names for validation (Requirement 8.6)
    # Nodes can be in JUNCTIONS, STORAGE, or OUTFALLS sections
    valid_nodes = set()
    for section_name in ['JUNCTIONS', 'STORAGE', 'OUTFALLS']:
        nodes = sections.get(section_name, [])
        for node_line in nodes:
            if len(node_line) >= 1:
                valid_nodes.add(node_line[0])
    
    # Requirement 3.5: Track which nodes have DUMMY patterns (avoid duplicates)
    dummy_nodes = set()
    
    for dwf_line in dwf_entries:
        # DWF format requires at least: Node Constituent AvgValue Pattern1
        # We need at least 4 fields to check for patterns
        if len(dwf_line) < 4:
            continue
        
        node_name = dwf_line[0]
        # Patterns are in positions 3, 4, 5, 6 (if they exist)
        # Pattern1 is at index 3, Pattern2-4 are at indices 4-6
        patterns = dwf_line[3:7] if len(dwf_line) >= 7 else dwf_line[3:]
        
        # Requirement 3.1, 3.6: Check if any pattern is DUMMY
        if "DUMMY" in patterns:
            # Requirement 8.6: Validate that the node exists in a valid section
            if node_name in valid_nodes:
                # Requirement 3.5: Avoid duplicate nodes if multiple DWF entries exist
                if node_name not in dummy_nodes:
                    # Requirement 3.2, 3.3: Add with object_type "NODE" and property "LATFLOW"
                    inputs.append(InputElement(node_name, "NODE", index))
                    dummy_nodes.add(node_name)
                    index += 1
            else:
                print(f"Warning: Skipping DUMMY reference to node '{node_name}' - not found in [JUNCTIONS], [STORAGE], or [OUTFALLS] sections", 
                      file=sys.stderr)
    
    # Requirement 3.5: Missing [DWF] section is handled gracefully by sections.get()
    return inputs


def discover_rain_gages(sections: Dict[str, List[List[str]]], start_index: int) -> List[InputElement]:
    """
    Discover rain gages with DUMMY timeseries references.
    
    RAINGAGES section format:
    Name Format Interval SCF Source_Type Source_Name
    
    Example:
    RG1 INTENSITY 1:00 1.0 TIMESERIES DUMMY
    
    Args:
        sections: Dictionary of parsed SWMM sections
        start_index: Starting interface index for discovered rain gages
        
    Returns:
        List of InputElement objects with object_type "GAGE" and property "RAINFALL"
    """
    inputs = []
    raingages = sections.get('RAINGAGES', [])
    index = start_index
    
    # Iterate through rain gages to find DUMMY timeseries
    for gage_line in raingages:
        # RAINGAGES format: Name Format Interval SCF Source_Type Source_Name
        # We need at least 6 fields to have a complete rain gage definition
        if len(gage_line) < 6:
            # Skip incomplete lines
            continue
        
        gage_name = gage_line[0]
        source_type = gage_line[4]
        source_name = gage_line[5]
        
        # Filter for TIMESERIES named DUMMY
        if source_type == "TIMESERIES" and source_name == "DUMMY":
            # Requirement 8.6: Rain gage exists by definition since we're iterating through [RAINGAGES]
            inputs.append(InputElement(gage_name, "GAGE", index))
            index += 1
    
    # Missing [RAINGAGES] section is handled gracefully by sections.get()
    return inputs


def discover_inputs(sections: Dict[str, List[List[str]]]) -> List[InputElement]:
    """
    Discover all dynamic input elements from parsed SWMM sections.
    
    Priority order (Requirements 4.1, 4.2, 4.5):
    1. Elapsed time (always index 0)
    2. Rain gages with TIMESERIES DUMMY
    3. Pumps with Pcurve DUMMY
    4. Orifices with control curve DUMMY  
    5. Weirs with control curve DUMMY
    6. Nodes with DWF pattern DUMMY
    
    Args:
        sections: Dictionary of parsed SWMM sections
        
    Returns:
        List of InputElement objects with sequential indices starting from 0
    """
    inputs = []
    next_index = 0
    
    # Priority 1: Elapsed time (always first at index 0)
    # Requirement 4.1: Maintain elapsed time at index 0
    inputs.append(InputElement("ElapsedTime", "SYSTEM", next_index))
    next_index += 1
    
    # Priority 2: Rain gages
    # Requirement 4.2: Call discovery functions in priority order
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
    
    # Requirement 4.5: Sequential indices starting from 0 with no gaps
    return inputs


def discover_outputs(sections: Dict[str, List[List[str]]]) -> List[OutputElement]:
    """
    Discover output elements from parsed SWMM sections.
    
    According to Requirement 3.6, outputs are assigned in priority order:
    1. Storage nodes (volume)
    2. Outfalls (flow)
    3. Orifices (flow)
    4. Weirs (flow)
    5. Subcatchments (runoff)
    
    Within each section, the order from the .inp file is preserved (Requirement 3.7).
    
    Args:
        sections: Dictionary of parsed SWMM sections
        
    Returns:
        List of OutputElement objects in priority order
    """
    outputs = []
    next_index = 0
    
    # Priority 1: Storage nodes - report volume (Requirement 3.1)
    storage_nodes = sections.get('STORAGE', [])
    for storage_line in storage_nodes:
        # STORAGE format: Name Elev MaxDepth InitDepth Shape ...
        # We need at least the name (first field)
        if len(storage_line) < 1:
            continue
        
        storage_name = storage_line[0]
        output = OutputElement(
            name=storage_name,
            object_type="STORAGE",
            value_type="VOLUME",
            index=next_index
        )
        outputs.append(output)
        next_index += 1
    
    # Priority 2: Outfalls - report discharge flow rate (Requirement 3.2)
    outfalls = sections.get('OUTFALLS', [])
    for outfall_line in outfalls:
        # OUTFALLS format: Name Elevation Type ...
        # We need at least the name (first field)
        if len(outfall_line) < 1:
            continue
        
        outfall_name = outfall_line[0]
        output = OutputElement(
            name=outfall_name,
            object_type="OUTFALL",
            value_type="FLOW",
            index=next_index
        )
        outputs.append(output)
        next_index += 1
    
    # Priority 3: Orifices - report current flow rate (Requirement 3.3)
    orifices = sections.get('ORIFICES', [])
    for orifice_line in orifices:
        # ORIFICES format: Name FromNode ToNode Type ...
        # We need at least the name (first field)
        if len(orifice_line) < 1:
            continue
        
        orifice_name = orifice_line[0]
        output = OutputElement(
            name=orifice_name,
            object_type="ORIFICE",
            value_type="FLOW",
            index=next_index
        )
        outputs.append(output)
        next_index += 1
    
    # Priority 4: Weirs - report current flow rate (Requirement 3.4)
    weirs = sections.get('WEIRS', [])
    for weir_line in weirs:
        # WEIRS format: Name FromNode ToNode Type ...
        # We need at least the name (first field)
        if len(weir_line) < 1:
            continue
        
        weir_name = weir_line[0]
        output = OutputElement(
            name=weir_name,
            object_type="WEIR",
            value_type="FLOW",
            index=next_index
        )
        outputs.append(output)
        next_index += 1
    
    # Priority 5: Subcatchments - report runoff flow rate (Requirement 3.5)
    subcatchments = sections.get('SUBCATCHMENTS', [])
    for subcatch_line in subcatchments:
        # SUBCATCHMENTS format: Name RainGage Outlet Area ...
        # We need at least the name (first field)
        if len(subcatch_line) < 1:
            continue
        
        subcatch_name = subcatch_line[0]
        output = OutputElement(
            name=subcatch_name,
            object_type="SUBCATCH",
            value_type="RUNOFF",
            index=next_index
        )
        outputs.append(output)
        next_index += 1
    
    return outputs


def compute_hash(inp_path: str) -> str:
    """
    Compute MD5 hash of .inp file contents, excluding comments and whitespace.
    
    According to Requirement 5.3, the hash should exclude comments (lines starting
    with semicolons) and normalize whitespace so that formatting changes don't
    affect the hash value.
    
    Args:
        inp_path: Path to the SWMM .inp file
        
    Returns:
        MD5 hash as a hexadecimal string
    """
    # Read file and extract meaningful content
    meaningful_lines = []
    
    try:
        with open(inp_path, 'r', encoding='utf-8') as f:
            for line in f:
                # Strip whitespace from both ends
                line = line.strip()
                
                # Skip empty lines
                if not line:
                    continue
                
                # Skip comment lines (starting with semicolon)
                if line.startswith(';'):
                    continue
                
                # Normalize whitespace: split and rejoin to collapse multiple spaces
                # This ensures that "A  B" and "A B" produce the same hash
                normalized = ' '.join(line.split())
                meaningful_lines.append(normalized)
        
        # Join all meaningful lines with newlines
        # This creates a canonical representation of the file content
        content = '\n'.join(meaningful_lines)
        
        # Compute MD5 hash
        hash_obj = hashlib.md5(content.encode('utf-8'))
        return hash_obj.hexdigest()
        
    except IOError as e:
        print(f"Error reading file for hash computation: {e}", file=sys.stderr)
        raise
    except Exception as e:
        print(f"Error computing hash: {e}", file=sys.stderr)
        raise


def generate_mapping_file(
    inputs: List[InputElement],
    outputs: List[OutputElement],
    content_hash: str,
    output_path: str = "SwmmGoldSimBridge.json"
) -> bool:
    """
    Generate JSON mapping file from discovered inputs and outputs.
    
    According to Requirements 4.1-4.8, the mapping file should:
    - Be in JSON format (4.1)
    - Be named "SwmmGoldSimBridge.json" (4.2)
    - Contain input and output counts (4.3, 4.4)
    - Contain ordered lists of input and output elements (4.5, 4.6)
    - Contain a version hash (4.7)
    - Overwrite existing files (4.8)
    
    Args:
        inputs: List of InputElement objects
        outputs: List of OutputElement objects
        content_hash: MD5 hash of the .inp file content
        output_path: Path where the JSON file should be written
        
    Returns:
        True if successful, False otherwise
    """
    try:
        # Build the mapping dictionary structure
        mapping = {
            "version": "1.0",
            "inp_file_hash": content_hash,
            "input_count": len(inputs),
            "output_count": len(outputs),
            "inputs": [],
            "outputs": []
        }
        
        # Property mapping for each object type (Requirements 1.3, 2.5, 3.3)
        property_map = {
            "SYSTEM": "ELAPSEDTIME",  # Elapsed time input
            "GAGE": "RAINFALL",       # Rain gage rainfall
            "PUMP": "SETTING",        # Pump setting (Requirement 1.3)
            "ORIFICE": "SETTING",     # Orifice setting (Requirement 2.5)
            "WEIR": "SETTING",        # Weir setting (Requirement 2.5)
            "NODE": "LATFLOW"         # Node lateral inflow (Requirement 3.3)
        }
        
        # Build inputs array with index, name, object_type, property
        for inp in inputs:
            # Determine the property based on object type
            property_name = property_map.get(inp.object_type, "UNKNOWN")
            
            input_entry = {
                "index": inp.index,
                "name": inp.name,
                "object_type": inp.object_type,
                "property": property_name
            }
            mapping["inputs"].append(input_entry)
        
        # Build outputs array with index, name, object_type, property, swmm_index
        for out in outputs:
            # Map value_type to property name
            if out.value_type == "VOLUME":
                property_name = "VOLUME"
            elif out.value_type == "FLOW":
                property_name = "FLOW"
            elif out.value_type == "RUNOFF":
                property_name = "RUNOFF"
            else:
                property_name = "UNKNOWN"
            
            output_entry = {
                "index": out.index,
                "name": out.name,
                "object_type": out.object_type,
                "property": property_name,
                "swmm_index": 0  # Will be resolved by DLL at runtime
            }
            mapping["outputs"].append(output_entry)
        
        # Write to JSON file (Requirement 4.8: overwrite if exists)
        with open(output_path, 'w', encoding='utf-8') as f:
            json.dump(mapping, f, indent=2)
        
        return True
        
    except IOError as e:
        print(f"Error writing mapping file: {e}", file=sys.stderr)
        return False
    except Exception as e:
        print(f"Error generating mapping: {e}", file=sys.stderr)
        return False


def main():
    """
    Main entry point for the script.
    
    Returns:
        Exit code (0 for success, non-zero for error)
    """
    # Parse command-line arguments
    args = parse_arguments()
    
    # If no arguments provided, show usage and exit with error
    if args is None:
        return 1
    
    # Validate that the input file exists
    if not validate_file_exists(args.inp_file):
        return 1
    
    # Parse the .inp file
    print(f"Processing: {args.inp_file}")
    sections, error = parse_inp_file(args.inp_file)
    
    if error:
        # Error message already printed by parse_inp_file
        return 1
    
    if not sections:
        print("Error: Failed to parse .inp file or file is empty", file=sys.stderr)
        return 1
    
    # Validate the .inp file for common issues
    print("\nValidating .inp file...")
    has_errors, validation_issues = validate_inp_file(sections)
    
    if validation_issues:
        print("\nValidation Issues Found:")
        for issue in validation_issues:
            print(f"  {issue}")
        
        if has_errors:
            print("\n❌ CRITICAL ERRORS FOUND - Model will likely fail to run in SWMM")
            print("Please fix the errors above before using this model with GoldSim")
            return 1
        else:
            print("\n⚠️  Warnings found - model may have issues but will attempt to continue")
    else:
        print("✓ No validation issues found")
    
    # Discover input elements
    inputs = discover_inputs(sections)
    
    # Discover output elements
    outputs = discover_outputs(sections)
    
    # Compute content hash (Requirement 5.1, 5.3)
    try:
        content_hash = compute_hash(args.inp_file)
    except Exception as e:
        print(f"Error: Failed to compute content hash: {e}", file=sys.stderr)
        return 1
    
    # Display parsing results
    print(f"Successfully parsed {len(sections)} sections")
    for section_name in sorted(sections.keys()):
        print(f"  [{section_name}]: {len(sections[section_name])} entries")
    
    print(f"\nContent hash: {content_hash}")
    
    print(f"\nDiscovered {len(inputs)} input(s):")
    for inp in inputs:
        print(f"  [{inp.index}] {inp.name} ({inp.object_type})")
    
    print(f"\nDiscovered {len(outputs)} output(s):")
    for out in outputs:
        print(f"  [{out.index}] {out.name} ({out.object_type} - {out.value_type})")
    
    # Generate mapping file (Requirements 4.1-4.8)
    output_path = "SwmmGoldSimBridge.json"
    print(f"\nGenerating mapping file: {output_path}")
    
    if not generate_mapping_file(inputs, outputs, content_hash, output_path):
        print("Error: Failed to generate mapping file", file=sys.stderr)
        return 1
    
    # Print success message with output file path (Requirement 9.3)
    print(f"Successfully generated: {output_path}")
    
    # Print discovered counts (Requirement 9.4)
    print(f"Input count: {len(inputs)}")
    print(f"Output count: {len(outputs)}")
    
    return 0


if __name__ == '__main__':
    sys.exit(main())
