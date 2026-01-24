#!/usr/bin/env python3
"""
Unit tests for input discovery functionality.
Tests task 3.1: Implement elapsed time input (always index 0)
"""

import sys
import os

# Add parent directory to path to import generate_mapping
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from generate_mapping import InputElement, discover_inputs


def test_elapsed_time_always_first():
    """
    Test that elapsed time is always the first input at index 0.
    Validates Requirement 2.1.
    """
    # Test with empty sections
    inputs = discover_inputs({})
    assert len(inputs) == 1, f"Expected 1 input, got {len(inputs)}"
    assert inputs[0].name == "ElapsedTime", f"Expected 'ElapsedTime', got '{inputs[0].name}'"
    assert inputs[0].object_type == "SYSTEM", f"Expected 'SYSTEM', got '{inputs[0].object_type}'"
    assert inputs[0].index == 0, f"Expected index 0, got {inputs[0].index}"
    print("✓ Test 1: Elapsed time is first input with empty sections")
    
    # Test with sections that don't contain RAINGAGES
    sections = {
        "JUNCTIONS": [["J1", "0", "10", "0", "0", "0"]],
        "OUTFALLS": [["OUT1", "0", "FREE", "NO"]]
    }
    inputs = discover_inputs(sections)
    assert len(inputs) == 1, f"Expected 1 input, got {len(inputs)}"
    assert inputs[0].name == "ElapsedTime", f"Expected 'ElapsedTime', got '{inputs[0].name}'"
    assert inputs[0].object_type == "SYSTEM", f"Expected 'SYSTEM', got '{inputs[0].object_type}'"
    assert inputs[0].index == 0, f"Expected index 0, got {inputs[0].index}"
    print("✓ Test 2: Elapsed time is first input with non-RAINGAGES sections")
    
    # Test with RAINGAGES section (but no DUMMY timeseries yet - that's task 3.2)
    sections = {
        "RAINGAGES": [
            ["RG1", "INTENSITY", "0:01", "1.0", "TIMESERIES", "TS1"]
        ]
    }
    inputs = discover_inputs(sections)
    assert len(inputs) == 1, f"Expected 1 input (elapsed time only), got {len(inputs)}"
    assert inputs[0].name == "ElapsedTime", f"Expected 'ElapsedTime', got '{inputs[0].name}'"
    assert inputs[0].index == 0, f"Expected index 0, got {inputs[0].index}"
    print("✓ Test 3: Elapsed time is first input even with RAINGAGES section")


def test_input_element_structure():
    """
    Test that InputElement has the correct structure.
    """
    elem = InputElement(name="Test", object_type="GAGE", index=1)
    assert elem.name == "Test"
    assert elem.object_type == "GAGE"
    assert elem.index == 1
    print("✓ Test 4: InputElement structure is correct")


if __name__ == "__main__":
    print("Running input discovery tests for task 3.1...\n")
    
    try:
        test_input_element_structure()
        test_elapsed_time_always_first()
        
        print("\n" + "="*50)
        print("All tests PASSED! ✓")
        print("="*50)
        sys.exit(0)
        
    except AssertionError as e:
        print(f"\n❌ Test FAILED: {e}")
        sys.exit(1)
    except Exception as e:
        print(f"\n❌ Unexpected error: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)
