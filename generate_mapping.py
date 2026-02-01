#!/usr/bin/env python3
"""
Simple SWMM-GoldSim Bridge Mapping Generator

Generates a JSON mapping file without requiring DUMMY specifications.
You specify which inputs you want, and optionally which outputs.

Usage:
    python generate_mapping_simple.py model.inp
    python generate_mapping_simple.py model.inp --input R1
    python generate_mapping_simple.py model.inp --input R1 --input R2
    python generate_mapping_simple.py model.inp --input R1 --output SUB1 --output POND1
"""

import sys
import json
import argparse
import hashlib


def parse_inp_sections(inp_path):
    """Parse SWMM .inp file into sections."""
    sections = {}
    current_section = None
    
    with open(inp_path, 'r', encoding='utf-8') as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith(';'):
                continue
            
            if line.startswith('[') and line.endswith(']'):
                current_section = line[1:-1].strip()
                sections[current_section] = []
                continue
            
            if current_section:
                fields = line.split()
                if fields:
                    sections[current_section].append(fields)
    
    return sections


def compute_hash(inp_path):
    """Compute MD5 hash of .inp file (excluding comments)."""
    lines = []
    with open(inp_path, 'r', encoding='utf-8') as f:
        for line in f:
            line = line.strip()
            if line and not line.startswith(';'):
                lines.append(' '.join(line.split()))
    content = '\n'.join(lines)
    return hashlib.md5(content.encode('utf-8')).hexdigest()


def parse_input_spec(spec, sections):
    """Parse input specification - just a name, auto-detect type."""
    name = spec.strip()
    
    # Check what type of element this is
    for line in sections.get('RAINGAGES', []):
        if len(line) >= 1 and line[0] == name:
            return {'name': name, 'object_type': 'GAGE', 'property': 'RAINFALL'}
    
    for line in sections.get('PUMPS', []):
        if len(line) >= 1 and line[0] == name:
            return {'name': name, 'object_type': 'PUMP', 'property': 'SETTING'}
    
    for line in sections.get('ORIFICES', []):
        if len(line) >= 1 and line[0] == name:
            return {'name': name, 'object_type': 'ORIFICE', 'property': 'SETTING'}
    
    for line in sections.get('WEIRS', []):
        if len(line) >= 1 and line[0] == name:
            return {'name': name, 'object_type': 'WEIR', 'property': 'SETTING'}
    
    for line in sections.get('JUNCTIONS', []):
        if len(line) >= 1 and line[0] == name:
            return {'name': name, 'object_type': 'NODE', 'property': 'LATFLOW'}
    
    for line in sections.get('STORAGE', []):
        if len(line) >= 1 and line[0] == name:
            return {'name': name, 'object_type': 'NODE', 'property': 'LATFLOW'}
    
    raise ValueError(f"Input element '{name}' not found in model")


def parse_output_spec(spec, sections):
    """Parse output specification - just a name, auto-detect type and property."""
    name = spec.strip()
    
    # Check what type of element this is and assign default property
    for line in sections.get('SUBCATCHMENTS', []):
        if len(line) >= 1 and line[0] == name:
            return {'name': name, 'object_type': 'SUBCATCH', 'property': 'RUNOFF'}
    
    for line in sections.get('STORAGE', []):
        if len(line) >= 1 and line[0] == name:
            # Default to VOLUME for storage
            return {'name': name, 'object_type': 'STORAGE', 'property': 'VOLUME'}
    
    for line in sections.get('JUNCTIONS', []):
        if len(line) >= 1 and line[0] == name:
            return {'name': name, 'object_type': 'JUNCTION', 'property': 'INFLOW'}
    
    for line in sections.get('OUTFALLS', []):
        if len(line) >= 1 and line[0] == name:
            return {'name': name, 'object_type': 'OUTFALL', 'property': 'FLOW'}
    
    for line in sections.get('PUMPS', []):
        if len(line) >= 1 and line[0] == name:
            return {'name': name, 'object_type': 'PUMP', 'property': 'FLOW'}
    
    for line in sections.get('ORIFICES', []):
        if len(line) >= 1 and line[0] == name:
            return {'name': name, 'object_type': 'ORIFICE', 'property': 'FLOW'}
    
    for line in sections.get('WEIRS', []):
        if len(line) >= 1 and line[0] == name:
            return {'name': name, 'object_type': 'WEIR', 'property': 'FLOW'}
    
    for line in sections.get('CONDUITS', []):
        if len(line) >= 1 and line[0] == name:
            return {'name': name, 'object_type': 'CONDUIT', 'property': 'FLOW'}
    
    raise ValueError(f"Output element '{name}' not found in model")


def parse_lid_usage(sections):
    """Parse [LID_USAGE] section to identify LID deployments.
    
    The [LID_USAGE] section in SWMM INP files defines which LID controls
    are deployed in which subcatchments. Each line specifies:
    - Subcatchment name (column 0)
    - LID control name (column 1)
    - Number of units, area, width, etc. (remaining columns)
    
    Returns a list of dictionaries with 'subcatchment' and 'lid_control' keys.
    """
    lid_deployments = []
    
    for line in sections.get('LID_USAGE', []):
        # LID_USAGE format: Subcatchment LID_Process Number Area Width ...
        # We need at least subcatchment name (index 0) and LID control name (index 1)
        if len(line) >= 2:
            subcatch_name = line[0]
            lid_control_name = line[1]
            lid_deployments.append({
                'subcatchment': subcatch_name,
                'lid_control': lid_control_name
            })
    
    return lid_deployments


def parse_lid_controls(sections):
    """Parse [LID_CONTROLS] section to get list of defined LID control names.
    
    Returns a set of LID control names.
    """
    lid_controls = set()
    
    for line in sections.get('LID_CONTROLS', []):
        # LID_CONTROLS format: Name Type/Layer Parameters...
        # The first column is the LID control name
        if len(line) >= 1:
            lid_control_name = line[0]
            lid_controls.add(lid_control_name)
    
    return lid_controls


def validate_lid_deployments(sections):
    """Validate that LID controls in [LID_USAGE] exist in [LID_CONTROLS].
    
    Raises ValueError if validation fails.
    """
    lid_deployments = parse_lid_usage(sections)
    lid_controls = parse_lid_controls(sections)
    
    # Check each deployment references a valid control
    missing_controls = set()
    for deployment in lid_deployments:
        lid_control = deployment['lid_control']
        if lid_control not in lid_controls:
            missing_controls.add(lid_control)
    
    if missing_controls:
        raise ValueError(
            f"LID controls referenced in [LID_USAGE] but not defined in [LID_CONTROLS]: "
            f"{', '.join(sorted(missing_controls))}"
        )


def generate_lid_outputs(sections):
    """Generate LID output mappings with composite IDs.
    
    Creates output entries for each LID unit deployed in the model.
    Each output uses the composite ID format "SubcatchmentName/LIDControlName"
    to uniquely identify the LID unit.
    
    The bridge will parse these composite IDs at runtime to:
    1. Resolve the subcatchment index
    2. Enumerate LID units in that subcatchment
    3. Match the LID control name to get the LID unit index
    4. Retrieve storage volume using swmm_getLidUStorageVolume()
    
    Returns a list of output dictionaries for LID storage volumes.
    Each output uses composite ID format: "SubcatchmentName/LIDControlName"
    """
    # First validate that all LID deployments reference valid controls
    validate_lid_deployments(sections)
    
    lid_deployments = parse_lid_usage(sections)
    outputs = []
    
    for idx, deployment in enumerate(lid_deployments):
        # Create composite ID: "SubcatchmentName/LIDControlName"
        composite_id = f"{deployment['subcatchment']}/{deployment['lid_control']}"
        
        outputs.append({
            "index": idx,
            "name": composite_id,
            "object_type": "LID",
            "property": "STORAGE_VOLUME",
            "swmm_index": 0  # Resolved at runtime by bridge
        })
    
    return outputs


def generate_all_outputs(sections):
    """Generate outputs for all elements (original behavior)."""
    outputs = []
    idx = 0
    
    # Storage nodes
    for line in sections.get('STORAGE', []):
        if len(line) >= 1:
            outputs.append({
                "index": idx,
                "name": line[0],
                "object_type": "STORAGE",
                "property": "VOLUME",
                "swmm_index": 0
            })
            idx += 1
    
    # Outfalls
    for line in sections.get('OUTFALLS', []):
        if len(line) >= 1:
            outputs.append({
                "index": idx,
                "name": line[0],
                "object_type": "OUTFALL",
                "property": "FLOW",
                "swmm_index": 0
            })
            idx += 1
    
    # Orifices
    for line in sections.get('ORIFICES', []):
        if len(line) >= 1:
            outputs.append({
                "index": idx,
                "name": line[0],
                "object_type": "ORIFICE",
                "property": "FLOW",
                "swmm_index": 0
            })
            idx += 1
    
    # Weirs
    for line in sections.get('WEIRS', []):
        if len(line) >= 1:
            outputs.append({
                "index": idx,
                "name": line[0],
                "object_type": "WEIR",
                "property": "FLOW",
                "swmm_index": 0
            })
            idx += 1
    
    # Pumps
    for line in sections.get('PUMPS', []):
        if len(line) >= 1:
            outputs.append({
                "index": idx,
                "name": line[0],
                "object_type": "PUMP",
                "property": "FLOW",
                "swmm_index": 0
            })
            idx += 1
    
    # Subcatchments
    for line in sections.get('SUBCATCHMENTS', []):
        if len(line) >= 1:
            outputs.append({
                "index": idx,
                "name": line[0],
                "object_type": "SUBCATCH",
                "property": "RUNOFF",
                "swmm_index": 0
            })
            idx += 1
    
    return outputs


def generate_mapping(inp_path, input_specs=None, output_specs=None, include_lid_outputs=False):
    """Generate mapping JSON."""
    sections = parse_inp_sections(inp_path)
    content_hash = compute_hash(inp_path)
    
    # Build inputs (always include ElapsedTime)
    inputs = [
        {
            "index": 0,
            "name": "ElapsedTime",
            "object_type": "SYSTEM",
            "property": "ELAPSEDTIME"
        }
    ]
    
    if input_specs:
        for i, spec in enumerate(input_specs, start=1):
            inp = parse_input_spec(spec, sections)
            inp["index"] = i
            inputs.append(inp)
    
    # Build outputs
    if output_specs:
        # User specified outputs
        outputs = []
        for i, spec in enumerate(output_specs):
            out = parse_output_spec(spec, sections)
            out["index"] = i
            out["swmm_index"] = 0
            outputs.append(out)
    else:
        # Generate all outputs (original behavior)
        outputs = generate_all_outputs(sections)
    
    # Add LID outputs if requested
    if include_lid_outputs:
        lid_outputs = generate_lid_outputs(sections)
        # Re-index LID outputs to follow existing outputs
        start_idx = len(outputs)
        for lid_out in lid_outputs:
            lid_out["index"] = start_idx
            outputs.append(lid_out)
            start_idx += 1
    
    # Create mapping
    mapping = {
        "version": "1.0",
        "logging_level": "INFO",
        "inp_file_hash": content_hash,
        "input_count": len(inputs),
        "output_count": len(outputs),
        "inputs": inputs,
        "outputs": outputs
    }
    
    return mapping


def main():
    parser = argparse.ArgumentParser(
        description='Generate GoldSim-SWMM Bridge mapping (no DUMMY required)',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # No inputs, all outputs auto-generated
  python generate_mapping_simple.py model.inp
  
  # With rain gage inputs
  python generate_mapping_simple.py model.inp --input R1
  python generate_mapping_simple.py model.inp --input R1 --input R2
  
  # Multiple input types
  python generate_mapping_simple.py model.inp --input R1 --input P1 --input OR1
  
  # Specify both inputs and outputs
  python generate_mapping_simple.py model.inp --input R1 --output SUB1 --output POND1 --output OUT1
  
  # Node lateral flow input
  python generate_mapping_simple.py model.inp --input J2
  
  # Include LID storage volume outputs
  python generate_mapping_simple.py model.inp --lid-outputs
  python generate_mapping_simple.py model.inp --input R1 --lid-outputs --output OUT1

Notes:
  - Element names are auto-detected from the .inp file
  - If no outputs specified, all elements become outputs
  - Storage nodes default to VOLUME property
  - Use --output-file to specify different output filename
  - --lid-outputs generates composite ID outputs (SubcatchmentName/LIDControlName)
    for all LID units with STORAGE_VOLUME property
  - LID composite IDs reference specific LID units deployed in subcatchments
  - Example: "S1/InfilTrench" refers to InfilTrench LID control in subcatchment S1
  - LID outputs are validated against [LID_CONTROLS] section in the INP file
        """
    )
    
    parser.add_argument('inp_file', help='Path to SWMM .inp file')
    parser.add_argument('--input', '-i', action='append', dest='inputs',
                       help='Add input element by name (e.g., R1, P1, OR1)')
    parser.add_argument('--output', '-o', action='append', dest='outputs',
                       help='Add output element by name (e.g., SUB1, POND1, OUT1)')
    parser.add_argument('--lid-outputs', action='store_true',
                       help='Include LID storage volume outputs using composite IDs (SubcatchmentName/LIDControlName)')
    parser.add_argument('--output-file', '-f', default='SwmmGoldSimBridge.json',
                       help='Output JSON file (default: SwmmGoldSimBridge.json)')
    
    args = parser.parse_args()
    
    try:
        print(f"Processing: {args.inp_file}")
        mapping = generate_mapping(args.inp_file, args.inputs, args.outputs, args.lid_outputs)
        
        with open(args.output_file, 'w', encoding='utf-8') as f:
            json.dump(mapping, f, indent=2)
        
        print(f"\n✓ Generated: {args.output_file}")
        print(f"  Inputs:  {mapping['input_count']}")
        print(f"  Outputs: {mapping['output_count']}")
        
        print("\nInputs:")
        for inp in mapping['inputs']:
            print(f"  [{inp['index']}] {inp['name']} ({inp['object_type']}/{inp['property']})")
        
        print("\nOutputs:")
        for out in mapping['outputs']:
            print(f"  [{out['index']}] {out['name']} ({out['object_type']}/{out['property']})")
        
        return 0
    
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        return 1


if __name__ == '__main__':
    sys.exit(main())
