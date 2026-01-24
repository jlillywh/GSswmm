#!/usr/bin/env python3
"""
SWMM Model Diagram Generator

This script parses SWMM .inp files and generates a Mermaid.js diagram
showing the model structure (nodes, links, subcatchments).

Usage:
    python generate_diagram.py <path_to_swmm_inp_file>

Example:
    python generate_diagram.py tests/storage_model.inp

The script will generate a .mmd file that can be viewed in:
- GitHub/GitLab (automatic rendering)
- VS Code with Mermaid extension
- Online at https://mermaid.live
"""

import sys
import os
import argparse
from typing import Dict, List, Set, Tuple


def parse_inp_file(inp_path: str) -> Dict[str, List[List[str]]]:
    """
    Parse SWMM .inp file and extract sections.
    
    Args:
        inp_path: Path to the SWMM .inp file
        
    Returns:
        Dictionary mapping section names to lists of data lines
    """
    sections = {}
    current_section = None
    
    try:
        with open(inp_path, 'r', encoding='utf-8') as f:
            for line in f:
                line = line.strip()
                
                # Skip empty lines and comments
                if not line or line.startswith(';'):
                    continue
                
                # Check for section header
                if line.startswith('[') and line.endswith(']'):
                    current_section = line[1:-1].strip()
                    if current_section not in sections:
                        sections[current_section] = []
                    continue
                
                # Add data line to current section
                if current_section is not None:
                    fields = line.split()
                    if fields:
                        sections[current_section].append(fields)
        
        return sections
        
    except Exception as e:
        print(f"Error parsing file: {e}", file=sys.stderr)
        return {}


def sanitize_id(name: str) -> str:
    """Sanitize node/link names for Mermaid IDs."""
    # Replace special characters with underscores
    return name.replace('-', '_').replace('.', '_').replace(' ', '_')


def get_node_shape(node_name: str, sections: Dict[str, List[List[str]]]) -> Tuple[str, str]:
    """
    Determine the shape and type for a node in the diagram.
    
    Returns:
        Tuple of (shape_start, shape_end) for Mermaid syntax
    """
    # Check if it's a junction
    junctions = {line[0] for line in sections.get('JUNCTIONS', []) if len(line) >= 1}
    if node_name in junctions:
        return '([', '])'  # Stadium shape for junctions
    
    # Check if it's storage
    storage = {line[0] for line in sections.get('STORAGE', []) if len(line) >= 1}
    if node_name in storage:
        return '[(', ')]'  # Cylinder shape for storage
    
    # Check if it's an outfall
    outfalls = {line[0] for line in sections.get('OUTFALLS', []) if len(line) >= 1}
    if node_name in outfalls:
        return '[[', ']]'  # Hexagon shape for outfalls
    
    # Default to circle
    return '((', '))'


def generate_mermaid_diagram(sections: Dict[str, List[List[str]]]) -> str:
    """
    Generate Mermaid.js diagram from parsed SWMM sections.
    
    Args:
        sections: Dictionary of parsed SWMM sections
        
    Returns:
        Mermaid diagram as a string
    """
    lines = []
    lines.append("```mermaid")
    lines.append("graph TD")
    lines.append("    %% SWMM Model Structure Diagram")
    lines.append("")
    
    # Track all nodes
    all_nodes = set()
    
    # Add subcatchments
    subcatchments = sections.get('SUBCATCHMENTS', [])
    if subcatchments:
        lines.append("    %% Subcatchments")
        for subcatch_line in subcatchments:
            if len(subcatch_line) >= 3:
                subcatch_name = subcatch_line[0]
                rain_gage = subcatch_line[1]
                outlet = subcatch_line[2]
                
                subcatch_id = sanitize_id(subcatch_name)
                outlet_id = sanitize_id(outlet)
                
                # Subcatchment as a rounded rectangle
                lines.append(f"    {subcatch_id}[/{subcatch_name}\\]")
                lines.append(f"    {subcatch_id} -->|runoff| {outlet_id}")
                
                all_nodes.add(outlet)
        lines.append("")
    
    # Add nodes with appropriate shapes
    lines.append("    %% Nodes")
    
    # Junctions
    junctions = sections.get('JUNCTIONS', [])
    for junction_line in junctions:
        if len(junction_line) >= 1:
            node_name = junction_line[0]
            node_id = sanitize_id(node_name)
            lines.append(f"    {node_id}([{node_name}])")
            all_nodes.add(node_name)
    
    # Storage nodes
    storage_nodes = sections.get('STORAGE', [])
    for storage_line in storage_nodes:
        if len(storage_line) >= 1:
            node_name = storage_line[0]
            node_id = sanitize_id(node_name)
            lines.append(f"    {node_id}[({node_name})]")
            all_nodes.add(node_name)
    
    # Outfalls
    outfalls = sections.get('OUTFALLS', [])
    for outfall_line in outfalls:
        if len(outfall_line) >= 1:
            node_name = outfall_line[0]
            node_id = sanitize_id(node_name)
            lines.append(f"    {node_id}[[\"{node_name}\"]]")
            all_nodes.add(node_name)
    
    lines.append("")
    
    # Add conduits
    conduits = sections.get('CONDUITS', [])
    if conduits:
        lines.append("    %% Conduits")
        for conduit_line in conduits:
            if len(conduit_line) >= 3:
                link_name = conduit_line[0]
                from_node = conduit_line[1]
                to_node = conduit_line[2]
                
                from_id = sanitize_id(from_node)
                to_id = sanitize_id(to_node)
                
                lines.append(f"    {from_id} -->|{link_name}| {to_id}")
                all_nodes.add(from_node)
                all_nodes.add(to_node)
        lines.append("")
    
    # Add orifices
    orifices = sections.get('ORIFICES', [])
    if orifices:
        lines.append("    %% Orifices")
        for orifice_line in orifices:
            if len(orifice_line) >= 3:
                link_name = orifice_line[0]
                from_node = orifice_line[1]
                to_node = orifice_line[2]
                
                from_id = sanitize_id(from_node)
                to_id = sanitize_id(to_node)
                
                lines.append(f"    {from_id} -.->|{link_name}<br/>orifice| {to_id}")
                all_nodes.add(from_node)
                all_nodes.add(to_node)
        lines.append("")
    
    # Add weirs
    weirs = sections.get('WEIRS', [])
    if weirs:
        lines.append("    %% Weirs")
        for weir_line in weirs:
            if len(weir_line) >= 3:
                link_name = weir_line[0]
                from_node = weir_line[1]
                to_node = weir_line[2]
                
                from_id = sanitize_id(from_node)
                to_id = sanitize_id(to_node)
                
                lines.append(f"    {from_id} -.->|{link_name}<br/>weir| {to_id}")
                all_nodes.add(from_node)
                all_nodes.add(to_node)
        lines.append("")
    
    # Add pumps
    pumps = sections.get('PUMPS', [])
    if pumps:
        lines.append("    %% Pumps")
        for pump_line in pumps:
            if len(pump_line) >= 3:
                link_name = pump_line[0]
                from_node = pump_line[1]
                to_node = pump_line[2]
                
                from_id = sanitize_id(from_node)
                to_id = sanitize_id(to_node)
                
                lines.append(f"    {from_id} ==>|{link_name}<br/>pump| {to_id}")
                all_nodes.add(from_node)
                all_nodes.add(to_node)
        lines.append("")
    
    # Add styling
    lines.append("    %% Styling")
    lines.append("    classDef subcatchment fill:#e1f5e1,stroke:#4caf50,stroke-width:2px")
    lines.append("    classDef storage fill:#fff3e0,stroke:#ff9800,stroke-width:2px")
    lines.append("    classDef outfall fill:#e3f2fd,stroke:#2196f3,stroke-width:2px")
    lines.append("    classDef junction fill:#f5f5f5,stroke:#757575,stroke-width:2px")
    lines.append("")
    
    # Apply classes
    subcatch_ids = [sanitize_id(line[0]) for line in subcatchments if len(line) >= 1]
    if subcatch_ids:
        lines.append(f"    class {','.join(subcatch_ids)} subcatchment")
    
    storage_ids = [sanitize_id(line[0]) for line in storage_nodes if len(line) >= 1]
    if storage_ids:
        lines.append(f"    class {','.join(storage_ids)} storage")
    
    outfall_ids = [sanitize_id(line[0]) for line in outfalls if len(line) >= 1]
    if outfall_ids:
        lines.append(f"    class {','.join(outfall_ids)} outfall")
    
    junction_ids = [sanitize_id(line[0]) for line in junctions if len(line) >= 1]
    if junction_ids:
        lines.append(f"    class {','.join(junction_ids)} junction")
    
    lines.append("```")
    
    return '\n'.join(lines)


def main():
    """Main entry point."""
    parser = argparse.ArgumentParser(
        description='Generate Mermaid.js diagram from SWMM .inp file',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python generate_diagram.py tests/storage_model.inp
  python generate_diagram.py model.inp

The script will generate a .mmd file with the same base name as the input file.
        """
    )
    
    parser.add_argument(
        'inp_file',
        nargs='?',
        help='Path to SWMM .inp file'
    )
    
    args = parser.parse_args()
    
    if args.inp_file is None:
        print(__doc__)
        return 1
    
    if not os.path.exists(args.inp_file):
        print(f"Error: File not found: {args.inp_file}", file=sys.stderr)
        return 1
    
    print(f"Processing: {args.inp_file}")
    
    # Parse the .inp file
    sections = parse_inp_file(args.inp_file)
    
    if not sections:
        print("Error: Failed to parse .inp file", file=sys.stderr)
        return 1
    
    # Generate Mermaid diagram
    diagram = generate_mermaid_diagram(sections)
    
    # Determine output filename
    base_name = os.path.splitext(args.inp_file)[0]
    output_file = f"{base_name}.mmd"
    
    # Write diagram to file
    try:
        with open(output_file, 'w', encoding='utf-8') as f:
            f.write(diagram)
        
        print(f"\n✓ Successfully generated: {output_file}")
        print(f"\nView the diagram:")
        print(f"  - In VS Code with Mermaid extension")
        print(f"  - On GitHub/GitLab (automatic rendering)")
        print(f"  - Online at https://mermaid.live")
        
        return 0
        
    except Exception as e:
        print(f"Error writing output file: {e}", file=sys.stderr)
        return 1


if __name__ == '__main__':
    sys.exit(main())
