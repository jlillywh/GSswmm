#!/usr/bin/env python3
"""
Unit tests for the SWMM .inp file parser.

Tests the section parser functionality including:
- Section header parsing
- Comment line skipping
- Data line extraction
- Missing section handling
- Input discovery (elapsed time and DUMMY rain gages)
"""

import unittest
import tempfile
import os
import sys

# Add parent directory to path to import generate_mapping
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from generate_mapping import parse_inp_file, discover_inputs


class TestSectionParser(unittest.TestCase):
    """Test cases for the section parser."""
    
    def test_parse_section_headers(self):
        """Test that section headers in square brackets are correctly identified."""
        content = """
[SECTION1]
data1 value1
data2 value2

[SECTION2]
data3 value3
"""
        with tempfile.NamedTemporaryFile(mode='w', suffix='.inp', delete=False) as f:
            f.write(content)
            temp_path = f.name
        
        try:
            sections, error = parse_inp_file(temp_path)
            self.assertIsNone(error)
            self.assertIn('SECTION1', sections)
            self.assertIn('SECTION2', sections)
            self.assertEqual(len(sections['SECTION1']), 2)
            self.assertEqual(len(sections['SECTION2']), 1)
        finally:
            os.unlink(temp_path)
    
    def test_skip_comment_lines(self):
        """Test that comment lines starting with semicolons are skipped."""
        content = """
[TEST]
;; This is a comment
data1 value1
; Another comment
data2 value2
;Comment without space
data3 value3
"""
        with tempfile.NamedTemporaryFile(mode='w', suffix='.inp', delete=False) as f:
            f.write(content)
            temp_path = f.name
        
        try:
            sections, error = parse_inp_file(temp_path)
            self.assertIsNone(error)
            self.assertIn('TEST', sections)
            # Should have 3 data lines, no comments
            self.assertEqual(len(sections['TEST']), 3)
            # Verify the data lines
            self.assertEqual(sections['TEST'][0], ['data1', 'value1'])
            self.assertEqual(sections['TEST'][1], ['data2', 'value2'])
            self.assertEqual(sections['TEST'][2], ['data3', 'value3'])
        finally:
            os.unlink(temp_path)
    
    def test_whitespace_separation(self):
        """Test that data lines are split by whitespace."""
        content = """
[TEST]
field1 field2 field3
tab1\ttab2\ttab3
mixed  spaces\tand\ttabs
"""
        with tempfile.NamedTemporaryFile(mode='w', suffix='.inp', delete=False) as f:
            f.write(content)
            temp_path = f.name
        
        try:
            sections, error = parse_inp_file(temp_path)
            self.assertIsNone(error)
            self.assertIn('TEST', sections)
            self.assertEqual(len(sections['TEST']), 3)
            # Check that all fields are correctly separated
            self.assertEqual(sections['TEST'][0], ['field1', 'field2', 'field3'])
            self.assertEqual(sections['TEST'][1], ['tab1', 'tab2', 'tab3'])
            self.assertEqual(sections['TEST'][2], ['mixed', 'spaces', 'and', 'tabs'])
        finally:
            os.unlink(temp_path)
    
    def test_empty_lines_ignored(self):
        """Test that empty lines are ignored."""
        content = """
[TEST]

data1 value1

data2 value2


data3 value3
"""
        with tempfile.NamedTemporaryFile(mode='w', suffix='.inp', delete=False) as f:
            f.write(content)
            temp_path = f.name
        
        try:
            sections, error = parse_inp_file(temp_path)
            self.assertIsNone(error)
            self.assertIn('TEST', sections)
            # Should have 3 data lines, empty lines ignored
            self.assertEqual(len(sections['TEST']), 3)
        finally:
            os.unlink(temp_path)
    
    def test_missing_sections_handled_gracefully(self):
        """Test that missing sections result in empty dict entries (not present)."""
        content = """
[SECTION1]
data1 value1
"""
        with tempfile.NamedTemporaryFile(mode='w', suffix='.inp', delete=False) as f:
            f.write(content)
            temp_path = f.name
        
        try:
            sections, error = parse_inp_file(temp_path)
            self.assertIsNone(error)
            # Only SECTION1 should be present
            self.assertIn('SECTION1', sections)
            self.assertNotIn('SECTION2', sections)
            # Can safely check for missing sections
            self.assertEqual(sections.get('SECTION2', []), [])
        finally:
            os.unlink(temp_path)
    
    def test_data_before_section_ignored(self):
        """Test that data lines before any section header are ignored."""
        content = """
orphan1 data1
orphan2 data2

[SECTION1]
data1 value1
"""
        with tempfile.NamedTemporaryFile(mode='w', suffix='.inp', delete=False) as f:
            f.write(content)
            temp_path = f.name
        
        try:
            sections, error = parse_inp_file(temp_path)
            self.assertIsNone(error)
            self.assertIn('SECTION1', sections)
            # Only data after section header should be captured
            self.assertEqual(len(sections['SECTION1']), 1)
        finally:
            os.unlink(temp_path)
    
    def test_section_with_spaces_in_name(self):
        """Test that section names with spaces are handled."""
        content = """
[SECTION WITH SPACES]
data1 value1
"""
        with tempfile.NamedTemporaryFile(mode='w', suffix='.inp', delete=False) as f:
            f.write(content)
            temp_path = f.name
        
        try:
            sections, error = parse_inp_file(temp_path)
            self.assertIsNone(error)
            self.assertIn('SECTION WITH SPACES', sections)
            self.assertEqual(len(sections['SECTION WITH SPACES']), 1)
        finally:
            os.unlink(temp_path)
    
    def test_empty_section(self):
        """Test that empty sections are handled correctly."""
        content = """
[EMPTY_SECTION]

[SECTION_WITH_DATA]
data1 value1
"""
        with tempfile.NamedTemporaryFile(mode='w', suffix='.inp', delete=False) as f:
            f.write(content)
            temp_path = f.name
        
        try:
            sections, error = parse_inp_file(temp_path)
            self.assertIsNone(error)
            self.assertIn('EMPTY_SECTION', sections)
            self.assertIn('SECTION_WITH_DATA', sections)
            # Empty section should have empty list
            self.assertEqual(len(sections['EMPTY_SECTION']), 0)
            self.assertEqual(len(sections['SECTION_WITH_DATA']), 1)
        finally:
            os.unlink(temp_path)
    
    def test_real_swmm_file(self):
        """Test parsing the actual test_model.inp file."""
        test_file = os.path.join(os.path.dirname(__file__), 'test_model.inp')
        
        if not os.path.exists(test_file):
            self.skipTest(f"Test file not found: {test_file}")
        
        sections, error = parse_inp_file(test_file)
        self.assertIsNone(error)
        
        # Verify expected sections are present
        self.assertIn('RAINGAGES', sections)
        self.assertIn('SUBCATCHMENTS', sections)
        self.assertIn('OUTFALLS', sections)
        
        # Verify RAINGAGES section has correct data
        self.assertEqual(len(sections['RAINGAGES']), 1)
        # First field should be the gage name
        self.assertEqual(sections['RAINGAGES'][0][0], 'RG1')
        
        # Verify SUBCATCHMENTS section
        self.assertEqual(len(sections['SUBCATCHMENTS']), 3)
        # Check subcatchment names
        subcatch_names = [line[0] for line in sections['SUBCATCHMENTS']]
        self.assertEqual(subcatch_names, ['S1', 'S2', 'S3'])
        
        # Verify OUTFALLS section
        self.assertEqual(len(sections['OUTFALLS']), 1)
        self.assertEqual(sections['OUTFALLS'][0][0], 'OUT1')
    
    def test_nonexistent_file(self):
        """Test that parsing a nonexistent file returns empty dict and error."""
        sections, error = parse_inp_file('/nonexistent/path/to/file.inp')
        self.assertEqual(sections, {})
        self.assertIsNotNone(error)
        self.assertIn("Error reading file", error)
    
    def test_malformed_section_header(self):
        """Test handling of malformed section headers - should return errors."""
        # Test unclosed bracket
        content = """
[VALID_SECTION]
data1 value1
[UNCLOSED_SECTION
data2 value2
"""
        with tempfile.NamedTemporaryFile(mode='w', suffix='.inp', delete=False) as f:
            f.write(content)
            temp_path = f.name
        
        try:
            sections, error = parse_inp_file(temp_path)
            # Should return an error for malformed section header
            self.assertIsNotNone(error)
            self.assertIn("Malformed section header", error)
            self.assertIn("missing closing bracket", error)
        finally:
            os.unlink(temp_path)
    
    def test_empty_section_name(self):
        """Test that empty section names are detected as errors."""
        content = """
[VALID_SECTION]
data1 value1
[]
data2 value2
"""
        with tempfile.NamedTemporaryFile(mode='w', suffix='.inp', delete=False) as f:
            f.write(content)
            temp_path = f.name
        
        try:
            sections, error = parse_inp_file(temp_path)
            # Should return an error for empty section name
            self.assertIsNotNone(error)
            self.assertIn("Empty section name", error)
        finally:
            os.unlink(temp_path)
    
    def test_bracket_in_data_line(self):
        """Test that brackets in data lines are detected as errors."""
        content = """
[VALID_SECTION]
data1 value1
data2 [invalid] value2
"""
        with tempfile.NamedTemporaryFile(mode='w', suffix='.inp', delete=False) as f:
            f.write(content)
            temp_path = f.name
        
        try:
            sections, error = parse_inp_file(temp_path)
            # Should return an error for unexpected bracket
            self.assertIsNotNone(error)
            self.assertIn("Invalid syntax", error)
            self.assertIn("unexpected bracket", error)
        finally:
            os.unlink(temp_path)


if __name__ == '__main__':
    unittest.main()


class TestInputDiscovery(unittest.TestCase):
    """Test cases for input element discovery."""
    
    def test_elapsed_time_always_first(self):
        """Test that elapsed time is always the first input at index 0."""
        # Empty sections - should still have elapsed time
        sections = {}
        inputs = discover_inputs(sections)
        
        self.assertEqual(len(inputs), 1)
        self.assertEqual(inputs[0].name, "ElapsedTime")
        self.assertEqual(inputs[0].object_type, "SYSTEM")
        self.assertEqual(inputs[0].index, 0)
    
    def test_discover_dummy_rain_gages(self):
        """Test that rain gages with DUMMY timeseries are discovered."""
        sections = {
            'RAINGAGES': [
                ['RG1', 'INTENSITY', '0:01', '1.0', 'TIMESERIES', 'DUMMY'],
                ['RG2', 'INTENSITY', '0:01', '1.0', 'TIMESERIES', 'DUMMY']
            ]
        }
        inputs = discover_inputs(sections)
        
        # Should have elapsed time + 2 DUMMY gages
        self.assertEqual(len(inputs), 3)
        
        # Check elapsed time
        self.assertEqual(inputs[0].name, "ElapsedTime")
        self.assertEqual(inputs[0].index, 0)
        
        # Check first DUMMY gage
        self.assertEqual(inputs[1].name, "RG1")
        self.assertEqual(inputs[1].object_type, "GAGE")
        self.assertEqual(inputs[1].index, 1)
        
        # Check second DUMMY gage
        self.assertEqual(inputs[2].name, "RG2")
        self.assertEqual(inputs[2].object_type, "GAGE")
        self.assertEqual(inputs[2].index, 2)
    
    def test_exclude_non_dummy_rain_gages(self):
        """Test that rain gages with non-DUMMY timeseries are excluded."""
        sections = {
            'RAINGAGES': [
                ['RG1', 'INTENSITY', '0:01', '1.0', 'TIMESERIES', 'DUMMY'],
                ['RG2', 'INTENSITY', '0:01', '1.0', 'TIMESERIES', 'TS1'],
                ['RG3', 'INTENSITY', '0:01', '1.0', 'TIMESERIES', 'TS2']
            ]
        }
        inputs = discover_inputs(sections)
        
        # Should have elapsed time + 1 DUMMY gage (RG2 and RG3 excluded)
        self.assertEqual(len(inputs), 2)
        self.assertEqual(inputs[0].name, "ElapsedTime")
        self.assertEqual(inputs[1].name, "RG1")
    
    def test_missing_raingages_section(self):
        """Test that missing RAINGAGES section is handled gracefully."""
        sections = {
            'SUBCATCHMENTS': [
                ['S1', 'RG1', 'J1', '10', '50', '500', '0.5', '0']
            ]
        }
        inputs = discover_inputs(sections)
        
        # Should have only elapsed time
        self.assertEqual(len(inputs), 1)
        self.assertEqual(inputs[0].name, "ElapsedTime")
        self.assertEqual(inputs[0].index, 0)
    
    def test_sequential_index_assignment(self):
        """Test that DUMMY rain gages get sequential indices starting from 1."""
        sections = {
            'RAINGAGES': [
                ['RG1', 'INTENSITY', '0:01', '1.0', 'TIMESERIES', 'DUMMY'],
                ['RG2', 'INTENSITY', '0:01', '1.0', 'TIMESERIES', 'DUMMY'],
                ['RG3', 'INTENSITY', '0:01', '1.0', 'TIMESERIES', 'DUMMY']
            ]
        }
        inputs = discover_inputs(sections)
        
        # Should have elapsed time + 3 DUMMY gages
        self.assertEqual(len(inputs), 4)
        
        # Check indices are sequential
        for i, inp in enumerate(inputs):
            self.assertEqual(inp.index, i)
    
    def test_incomplete_raingage_lines_skipped(self):
        """Test that incomplete rain gage lines are skipped."""
        sections = {
            'RAINGAGES': [
                ['RG1', 'INTENSITY', '0:01', '1.0', 'TIMESERIES', 'DUMMY'],
                ['RG2', 'INTENSITY', '0:01'],  # Incomplete - missing source
                ['RG3', 'INTENSITY', '0:01', '1.0', 'TIMESERIES', 'DUMMY']
            ]
        }
        inputs = discover_inputs(sections)
        
        # Should have elapsed time + 2 DUMMY gages (RG2 skipped)
        self.assertEqual(len(inputs), 3)
        self.assertEqual(inputs[0].name, "ElapsedTime")
        self.assertEqual(inputs[1].name, "RG1")
        self.assertEqual(inputs[2].name, "RG3")
    
    def test_case_sensitive_dummy_matching(self):
        """Test that DUMMY matching is case-sensitive."""
        sections = {
            'RAINGAGES': [
                ['RG1', 'INTENSITY', '0:01', '1.0', 'TIMESERIES', 'DUMMY'],
                ['RG2', 'INTENSITY', '0:01', '1.0', 'TIMESERIES', 'dummy'],
                ['RG3', 'INTENSITY', '0:01', '1.0', 'TIMESERIES', 'Dummy']
            ]
        }
        inputs = discover_inputs(sections)
        
        # Should have elapsed time + 1 DUMMY gage (only RG1 matches)
        self.assertEqual(len(inputs), 2)
        self.assertEqual(inputs[0].name, "ElapsedTime")
        self.assertEqual(inputs[1].name, "RG1")
    
    def test_real_swmm_file_with_dummy(self):
        """Test input discovery on the test_model_dummy.inp file."""
        test_file = os.path.join(os.path.dirname(__file__), 'test_model_dummy.inp')
        
        if not os.path.exists(test_file):
            self.skipTest(f"Test file not found: {test_file}")
        
        sections, error = parse_inp_file(test_file)
        self.assertIsNone(error)
        
        inputs = discover_inputs(sections)
        
        # Should have elapsed time + 2 DUMMY gages (RG1, RG2)
        self.assertEqual(len(inputs), 3)
        self.assertEqual(inputs[0].name, "ElapsedTime")
        self.assertEqual(inputs[0].index, 0)
        self.assertEqual(inputs[1].name, "RG1")
        self.assertEqual(inputs[1].index, 1)
        self.assertEqual(inputs[2].name, "RG2")
        self.assertEqual(inputs[2].index, 2)
    
    def test_real_swmm_file_without_dummy(self):
        """Test input discovery on the test_model.inp file (no DUMMY)."""
        test_file = os.path.join(os.path.dirname(__file__), 'test_model.inp')
        
        if not os.path.exists(test_file):
            self.skipTest(f"Test file not found: {test_file}")
        
        sections, error = parse_inp_file(test_file)
        self.assertIsNone(error)
        
        inputs = discover_inputs(sections)
        
        # Should have only elapsed time (no DUMMY gages)
        self.assertEqual(len(inputs), 1)
        self.assertEqual(inputs[0].name, "ElapsedTime")
        self.assertEqual(inputs[0].index, 0)


class TestOutputDiscovery(unittest.TestCase):
    """Test cases for output element discovery."""
    
    def test_discover_storage_nodes(self):
        """Test that storage nodes are discovered with VOLUME value type."""
        sections = {
            'STORAGE': [
                ['POND1', '0', '10', '0', 'FUNCTIONAL', '1000', '0', '0'],
                ['POND2', '0', '15', '0', 'FUNCTIONAL', '2000', '0', '0']
            ]
        }
        from generate_mapping import discover_outputs
        outputs = discover_outputs(sections)
        
        # Should have 2 storage nodes
        self.assertEqual(len(outputs), 2)
        
        # Check first storage node
        self.assertEqual(outputs[0].name, "POND1")
        self.assertEqual(outputs[0].object_type, "STORAGE")
        self.assertEqual(outputs[0].value_type, "VOLUME")
        self.assertEqual(outputs[0].index, 0)
        
        # Check second storage node
        self.assertEqual(outputs[1].name, "POND2")
        self.assertEqual(outputs[1].object_type, "STORAGE")
        self.assertEqual(outputs[1].value_type, "VOLUME")
        self.assertEqual(outputs[1].index, 1)
    
    def test_discover_outfalls(self):
        """Test that outfalls are discovered with FLOW value type."""
        sections = {
            'OUTFALLS': [
                ['OUT1', '0', 'FREE', 'NO'],
                ['OUT2', '0', 'FREE', 'NO']
            ]
        }
        from generate_mapping import discover_outputs
        outputs = discover_outputs(sections)
        
        # Should have 2 outfalls
        self.assertEqual(len(outputs), 2)
        
        # Check first outfall
        self.assertEqual(outputs[0].name, "OUT1")
        self.assertEqual(outputs[0].object_type, "OUTFALL")
        self.assertEqual(outputs[0].value_type, "FLOW")
        self.assertEqual(outputs[0].index, 0)
        
        # Check second outfall
        self.assertEqual(outputs[1].name, "OUT2")
        self.assertEqual(outputs[1].object_type, "OUTFALL")
        self.assertEqual(outputs[1].value_type, "FLOW")
        self.assertEqual(outputs[1].index, 1)
    
    def test_discover_orifices(self):
        """Test that orifices are discovered with FLOW value type."""
        sections = {
            'ORIFICES': [
                ['OR1', 'POND1', 'OUT1', 'SIDE', '0', '0.65', 'NO', '0'],
                ['OR2', 'POND2', 'OUT2', 'SIDE', '0', '0.65', 'NO', '0']
            ]
        }
        from generate_mapping import discover_outputs
        outputs = discover_outputs(sections)
        
        # Should have 2 orifices
        self.assertEqual(len(outputs), 2)
        
        # Check first orifice
        self.assertEqual(outputs[0].name, "OR1")
        self.assertEqual(outputs[0].object_type, "ORIFICE")
        self.assertEqual(outputs[0].value_type, "FLOW")
        self.assertEqual(outputs[0].index, 0)
        
        # Check second orifice
        self.assertEqual(outputs[1].name, "OR2")
        self.assertEqual(outputs[1].object_type, "ORIFICE")
        self.assertEqual(outputs[1].value_type, "FLOW")
        self.assertEqual(outputs[1].index, 1)
    
    def test_discover_weirs(self):
        """Test that weirs are discovered with FLOW value type."""
        sections = {
            'WEIRS': [
                ['W1', 'POND1', 'OUT1', 'TRANSVERSE', '0', '3.33', 'NO', '0', '0', 'YES'],
                ['W2', 'POND2', 'OUT2', 'TRANSVERSE', '0', '3.33', 'NO', '0', '0', 'YES']
            ]
        }
        from generate_mapping import discover_outputs
        outputs = discover_outputs(sections)
        
        # Should have 2 weirs
        self.assertEqual(len(outputs), 2)
        
        # Check first weir
        self.assertEqual(outputs[0].name, "W1")
        self.assertEqual(outputs[0].object_type, "WEIR")
        self.assertEqual(outputs[0].value_type, "FLOW")
        self.assertEqual(outputs[0].index, 0)
        
        # Check second weir
        self.assertEqual(outputs[1].name, "W2")
        self.assertEqual(outputs[1].object_type, "WEIR")
        self.assertEqual(outputs[1].value_type, "FLOW")
        self.assertEqual(outputs[1].index, 1)
    
    def test_discover_subcatchments(self):
        """Test that subcatchments are discovered with RUNOFF value type."""
        sections = {
            'SUBCATCHMENTS': [
                ['S1', 'RG1', 'J1', '10', '50', '500', '0.5', '0'],
                ['S2', 'RG1', 'J1', '15', '60', '600', '0.6', '0'],
                ['S3', 'RG1', 'J1', '8', '40', '400', '0.4', '0']
            ]
        }
        from generate_mapping import discover_outputs
        outputs = discover_outputs(sections)
        
        # Should have 3 subcatchments
        self.assertEqual(len(outputs), 3)
        
        # Check first subcatchment
        self.assertEqual(outputs[0].name, "S1")
        self.assertEqual(outputs[0].object_type, "SUBCATCH")
        self.assertEqual(outputs[0].value_type, "RUNOFF")
        self.assertEqual(outputs[0].index, 0)
        
        # Check second subcatchment
        self.assertEqual(outputs[1].name, "S2")
        self.assertEqual(outputs[1].object_type, "SUBCATCH")
        self.assertEqual(outputs[1].value_type, "RUNOFF")
        self.assertEqual(outputs[1].index, 1)
        
        # Check third subcatchment
        self.assertEqual(outputs[2].name, "S3")
        self.assertEqual(outputs[2].object_type, "SUBCATCH")
        self.assertEqual(outputs[2].value_type, "RUNOFF")
        self.assertEqual(outputs[2].index, 2)
    
    def test_priority_ordering(self):
        """Test that outputs are ordered by priority: storage, outfalls, orifices, weirs, subcatchments."""
        sections = {
            'SUBCATCHMENTS': [
                ['S1', 'RG1', 'J1', '10', '50', '500', '0.5', '0']
            ],
            'WEIRS': [
                ['W1', 'POND1', 'OUT1', 'TRANSVERSE', '0', '3.33', 'NO', '0', '0', 'YES']
            ],
            'ORIFICES': [
                ['OR1', 'POND1', 'OUT1', 'SIDE', '0', '0.65', 'NO', '0']
            ],
            'OUTFALLS': [
                ['OUT1', '0', 'FREE', 'NO']
            ],
            'STORAGE': [
                ['POND1', '0', '10', '0', 'FUNCTIONAL', '1000', '0', '0']
            ]
        }
        from generate_mapping import discover_outputs
        outputs = discover_outputs(sections)
        
        # Should have 5 outputs in priority order
        self.assertEqual(len(outputs), 5)
        
        # Check priority order
        self.assertEqual(outputs[0].object_type, "STORAGE")
        self.assertEqual(outputs[0].index, 0)
        
        self.assertEqual(outputs[1].object_type, "OUTFALL")
        self.assertEqual(outputs[1].index, 1)
        
        self.assertEqual(outputs[2].object_type, "ORIFICE")
        self.assertEqual(outputs[2].index, 2)
        
        self.assertEqual(outputs[3].object_type, "WEIR")
        self.assertEqual(outputs[3].index, 3)
        
        self.assertEqual(outputs[4].object_type, "SUBCATCH")
        self.assertEqual(outputs[4].index, 4)
    
    def test_element_order_preserved_within_sections(self):
        """Test that element order within sections is preserved."""
        sections = {
            'STORAGE': [
                ['POND1', '0', '10', '0', 'FUNCTIONAL', '1000', '0', '0'],
                ['POND2', '0', '15', '0', 'FUNCTIONAL', '2000', '0', '0'],
                ['POND3', '0', '20', '0', 'FUNCTIONAL', '3000', '0', '0']
            ],
            'OUTFALLS': [
                ['OUT1', '0', 'FREE', 'NO'],
                ['OUT2', '0', 'FREE', 'NO']
            ]
        }
        from generate_mapping import discover_outputs
        outputs = discover_outputs(sections)
        
        # Should have 5 outputs
        self.assertEqual(len(outputs), 5)
        
        # Check storage nodes are in order
        self.assertEqual(outputs[0].name, "POND1")
        self.assertEqual(outputs[1].name, "POND2")
        self.assertEqual(outputs[2].name, "POND3")
        
        # Check outfalls are in order
        self.assertEqual(outputs[3].name, "OUT1")
        self.assertEqual(outputs[4].name, "OUT2")
    
    def test_missing_sections_handled_gracefully(self):
        """Test that missing output sections are handled gracefully."""
        sections = {
            'STORAGE': [
                ['POND1', '0', '10', '0', 'FUNCTIONAL', '1000', '0', '0']
            ]
        }
        from generate_mapping import discover_outputs
        outputs = discover_outputs(sections)
        
        # Should have only 1 storage node
        self.assertEqual(len(outputs), 1)
        self.assertEqual(outputs[0].name, "POND1")
        self.assertEqual(outputs[0].object_type, "STORAGE")
    
    def test_no_output_sections(self):
        """Test that no output sections results in empty list."""
        sections = {
            'RAINGAGES': [
                ['RG1', 'INTENSITY', '0:01', '1.0', 'TIMESERIES', 'DUMMY']
            ]
        }
        from generate_mapping import discover_outputs
        outputs = discover_outputs(sections)
        
        # Should have no outputs
        self.assertEqual(len(outputs), 0)
    
    def test_incomplete_lines_skipped(self):
        """Test that incomplete lines are skipped."""
        sections = {
            'STORAGE': [
                ['POND1', '0', '10', '0', 'FUNCTIONAL', '1000', '0', '0'],
                [],  # Empty line
                ['POND2', '0', '15', '0', 'FUNCTIONAL', '2000', '0', '0']
            ]
        }
        from generate_mapping import discover_outputs
        outputs = discover_outputs(sections)
        
        # Should have 2 storage nodes (empty line skipped)
        self.assertEqual(len(outputs), 2)
        self.assertEqual(outputs[0].name, "POND1")
        self.assertEqual(outputs[1].name, "POND2")
    
    def test_real_swmm_file_test_model(self):
        """Test output discovery on the test_model.inp file."""
        test_file = os.path.join(os.path.dirname(__file__), 'test_model.inp')
        
        if not os.path.exists(test_file):
            self.skipTest(f"Test file not found: {test_file}")
        
        sections, error = parse_inp_file(test_file)
        self.assertIsNone(error)
        
        from generate_mapping import discover_outputs
        outputs = discover_outputs(sections)
        
        # Should have 1 outfall + 3 subcatchments = 4 outputs
        self.assertEqual(len(outputs), 4)
        
        # Check outfall comes first
        self.assertEqual(outputs[0].name, "OUT1")
        self.assertEqual(outputs[0].object_type, "OUTFALL")
        self.assertEqual(outputs[0].value_type, "FLOW")
        self.assertEqual(outputs[0].index, 0)
        
        # Check subcatchments come after
        self.assertEqual(outputs[1].name, "S1")
        self.assertEqual(outputs[1].object_type, "SUBCATCH")
        self.assertEqual(outputs[1].value_type, "RUNOFF")
        self.assertEqual(outputs[1].index, 1)
        
        self.assertEqual(outputs[2].name, "S2")
        self.assertEqual(outputs[2].object_type, "SUBCATCH")
        self.assertEqual(outputs[2].value_type, "RUNOFF")
        self.assertEqual(outputs[2].index, 2)
        
        self.assertEqual(outputs[3].name, "S3")
        self.assertEqual(outputs[3].object_type, "SUBCATCH")
        self.assertEqual(outputs[3].value_type, "RUNOFF")
        self.assertEqual(outputs[3].index, 3)
    
    def test_real_swmm_file_test_model_dummy(self):
        """Test output discovery on the test_model_dummy.inp file."""
        test_file = os.path.join(os.path.dirname(__file__), 'test_model_dummy.inp')
        
        if not os.path.exists(test_file):
            self.skipTest(f"Test file not found: {test_file}")
        
        sections, error = parse_inp_file(test_file)
        self.assertIsNone(error)
        
        from generate_mapping import discover_outputs
        outputs = discover_outputs(sections)
        
        # Should have 1 storage + 2 outfalls + 1 orifice + 1 weir + 2 subcatchments = 7 outputs
        self.assertEqual(len(outputs), 7)
        
        # Check priority order
        self.assertEqual(outputs[0].name, "POND1")
        self.assertEqual(outputs[0].object_type, "STORAGE")
        self.assertEqual(outputs[0].value_type, "VOLUME")
        
        self.assertEqual(outputs[1].name, "OUT1")
        self.assertEqual(outputs[1].object_type, "OUTFALL")
        self.assertEqual(outputs[1].value_type, "FLOW")
        
        self.assertEqual(outputs[2].name, "OUT2")
        self.assertEqual(outputs[2].object_type, "OUTFALL")
        self.assertEqual(outputs[2].value_type, "FLOW")
        
        self.assertEqual(outputs[3].name, "OR1")
        self.assertEqual(outputs[3].object_type, "ORIFICE")
        self.assertEqual(outputs[3].value_type, "FLOW")
        
        self.assertEqual(outputs[4].name, "W1")
        self.assertEqual(outputs[4].object_type, "WEIR")
        self.assertEqual(outputs[4].value_type, "FLOW")
        
        self.assertEqual(outputs[5].name, "S1")
        self.assertEqual(outputs[5].object_type, "SUBCATCH")
        self.assertEqual(outputs[5].value_type, "RUNOFF")
        
        self.assertEqual(outputs[6].name, "S2")
        self.assertEqual(outputs[6].object_type, "SUBCATCH")
        self.assertEqual(outputs[6].value_type, "RUNOFF")
        
        # Verify indices are sequential
        for i, output in enumerate(outputs):
            self.assertEqual(output.index, i)


class TestPumpDiscovery(unittest.TestCase):
    """Test cases for pump input discovery."""
    
    def test_discover_dummy_pumps(self):
        """Test that pumps with DUMMY curve are discovered."""
        from generate_mapping import discover_pumps
        
        sections = {
            'PUMPS': [
                ['P1', 'WET_WELL', 'J1', 'DUMMY', 'ON', '0', '0'],
                ['P2', 'J1', 'OUTLET', 'DUMMY', 'ON', '0', '0']
            ]
        }
        pumps = discover_pumps(sections, start_index=1)
        
        # Should have 2 DUMMY pumps
        self.assertEqual(len(pumps), 2)
        
        # Check first DUMMY pump
        self.assertEqual(pumps[0].name, "P1")
        self.assertEqual(pumps[0].object_type, "PUMP")
        self.assertEqual(pumps[0].index, 1)
        
        # Check second DUMMY pump
        self.assertEqual(pumps[1].name, "P2")
        self.assertEqual(pumps[1].object_type, "PUMP")
        self.assertEqual(pumps[1].index, 2)
    
    def test_exclude_non_dummy_pumps(self):
        """Test that pumps with non-DUMMY curves are excluded."""
        from generate_mapping import discover_pumps
        
        sections = {
            'PUMPS': [
                ['P1', 'WET_WELL', 'J1', 'DUMMY', 'ON', '0', '0'],
                ['P2', 'J1', 'OUTLET', 'TYPE1', 'ON', '0', '0'],
                ['P3', 'WET_WELL', 'OUTLET', 'TYPE2', 'ON', '0', '0']
            ]
        }
        pumps = discover_pumps(sections, start_index=1)
        
        # Should have 1 DUMMY pump (P2 and P3 excluded)
        self.assertEqual(len(pumps), 1)
        self.assertEqual(pumps[0].name, "P1")
    
    def test_missing_pumps_section(self):
        """Test that missing PUMPS section is handled gracefully."""
        from generate_mapping import discover_pumps
        
        sections = {
            'JUNCTIONS': [
                ['J1', '0', '0', '0', '0']
            ]
        }
        pumps = discover_pumps(sections, start_index=1)
        
        # Should have no pumps
        self.assertEqual(len(pumps), 0)
    
    def test_incomplete_pump_lines_skipped(self):
        """Test that incomplete pump lines are skipped."""
        from generate_mapping import discover_pumps
        
        sections = {
            'PUMPS': [
                ['P1', 'WET_WELL', 'J1', 'DUMMY', 'ON', '0', '0'],
                ['P2', 'J1', 'OUTLET'],  # Incomplete - missing Pcurve
                ['P3', 'WET_WELL', 'OUTLET', 'DUMMY', 'ON', '0', '0']
            ]
        }
        pumps = discover_pumps(sections, start_index=1)
        
        # Should have 2 DUMMY pumps (P2 skipped)
        self.assertEqual(len(pumps), 2)
        self.assertEqual(pumps[0].name, "P1")
        self.assertEqual(pumps[1].name, "P3")
    
    def test_sequential_index_assignment(self):
        """Test that DUMMY pumps get sequential indices starting from start_index."""
        from generate_mapping import discover_pumps
        
        sections = {
            'PUMPS': [
                ['P1', 'WET_WELL', 'J1', 'DUMMY', 'ON', '0', '0'],
                ['P2', 'J1', 'OUTLET', 'DUMMY', 'ON', '0', '0'],
                ['P3', 'WET_WELL', 'OUTLET', 'DUMMY', 'ON', '0', '0']
            ]
        }
        pumps = discover_pumps(sections, start_index=5)
        
        # Should have 3 DUMMY pumps starting at index 5
        self.assertEqual(len(pumps), 3)
        self.assertEqual(pumps[0].index, 5)
        self.assertEqual(pumps[1].index, 6)
        self.assertEqual(pumps[2].index, 7)
    
    def test_case_sensitive_dummy_matching(self):
        """Test that DUMMY matching is case-sensitive."""
        from generate_mapping import discover_pumps
        
        sections = {
            'PUMPS': [
                ['P1', 'WET_WELL', 'J1', 'DUMMY', 'ON', '0', '0'],
                ['P2', 'J1', 'OUTLET', 'dummy', 'ON', '0', '0'],
                ['P3', 'WET_WELL', 'OUTLET', 'Dummy', 'ON', '0', '0']
            ]
        }
        pumps = discover_pumps(sections, start_index=1)
        
        # Should have 1 DUMMY pump (only P1 matches)
        self.assertEqual(len(pumps), 1)
        self.assertEqual(pumps[0].name, "P1")
    
    def test_real_swmm_file_with_dummy_pumps(self):
        """Test pump discovery on the test_model_pumps.inp file."""
        test_file = os.path.join(os.path.dirname(__file__), 'test_model_pumps.inp')
        
        if not os.path.exists(test_file):
            self.skipTest(f"Test file not found: {test_file}")
        
        sections, error = parse_inp_file(test_file)
        self.assertIsNone(error)
        
        from generate_mapping import discover_pumps
        pumps = discover_pumps(sections, start_index=1)
        
        # Should have 2 DUMMY pumps (P1, P2) - P3 has TYPE1 curve
        self.assertEqual(len(pumps), 2)
        self.assertEqual(pumps[0].name, "P1")
        self.assertEqual(pumps[0].index, 1)
        self.assertEqual(pumps[1].name, "P2")
        self.assertEqual(pumps[1].index, 2)



class TestNodeInflowDiscovery(unittest.TestCase):
    """Test cases for node inflow input discovery."""
    
    def test_discover_dummy_node_inflows(self):
        """Test that nodes with DUMMY DWF patterns are discovered."""
        from generate_mapping import discover_node_inflows
        
        sections = {
            'DWF': [
                ['J1', 'FLOW', '1.0', 'DUMMY', '""', '""', '""'],
                ['J2', 'FLOW', '2.0', 'DUMMY', '""', '""', '""']
            ]
        }
        nodes = discover_node_inflows(sections, start_index=1)
        
        # Should have 2 DUMMY nodes
        self.assertEqual(len(nodes), 2)
        
        # Check first DUMMY node
        self.assertEqual(nodes[0].name, "J1")
        self.assertEqual(nodes[0].object_type, "NODE")
        self.assertEqual(nodes[0].index, 1)
        
        # Check second DUMMY node
        self.assertEqual(nodes[1].name, "J2")
        self.assertEqual(nodes[1].object_type, "NODE")
        self.assertEqual(nodes[1].index, 2)
    
    def test_exclude_non_dummy_node_inflows(self):
        """Test that nodes with non-DUMMY patterns are excluded."""
        from generate_mapping import discover_node_inflows
        
        sections = {
            'DWF': [
                ['J1', 'FLOW', '1.0', 'DUMMY', '""', '""', '""'],
                ['J2', 'FLOW', '2.0', 'PATTERN1', '""', '""', '""'],
                ['J3', 'FLOW', '1.5', 'PATTERN2', '""', '""', '""']
            ]
        }
        nodes = discover_node_inflows(sections, start_index=1)
        
        # Should have 1 DUMMY node (J2 and J3 excluded)
        self.assertEqual(len(nodes), 1)
        self.assertEqual(nodes[0].name, "J1")
    
    def test_missing_dwf_section(self):
        """Test that missing DWF section is handled gracefully."""
        from generate_mapping import discover_node_inflows
        
        sections = {
            'JUNCTIONS': [
                ['J1', '0', '0', '0', '0']
            ]
        }
        nodes = discover_node_inflows(sections, start_index=1)
        
        # Should have no nodes
        self.assertEqual(len(nodes), 0)
    
    def test_duplicate_node_handling(self):
        """Test that duplicate nodes with DUMMY patterns are handled (only added once)."""
        from generate_mapping import discover_node_inflows
        
        sections = {
            'DWF': [
                ['J1', 'FLOW', '1.0', 'DUMMY', '""', '""', '""'],
                ['J1', 'FLOW', '0.5', 'DUMMY', '""', '""', '""'],  # Duplicate node
                ['J2', 'FLOW', '2.0', 'DUMMY', '""', '""', '""']
            ]
        }
        nodes = discover_node_inflows(sections, start_index=1)
        
        # Should have 2 unique nodes (J1 only added once)
        self.assertEqual(len(nodes), 2)
        self.assertEqual(nodes[0].name, "J1")
        self.assertEqual(nodes[1].name, "J2")
    
    def test_incomplete_dwf_lines_skipped(self):
        """Test that incomplete DWF lines are skipped."""
        from generate_mapping import discover_node_inflows
        
        sections = {
            'DWF': [
                ['J1', 'FLOW', '1.0', 'DUMMY', '""', '""', '""'],
                ['J2', 'FLOW', '2.0'],  # Incomplete - missing pattern
                ['J3', 'FLOW', '1.5', 'DUMMY', '""', '""', '""']
            ]
        }
        nodes = discover_node_inflows(sections, start_index=1)
        
        # Should have 2 DUMMY nodes (J2 skipped)
        self.assertEqual(len(nodes), 2)
        self.assertEqual(nodes[0].name, "J1")
        self.assertEqual(nodes[1].name, "J3")
    
    def test_sequential_index_assignment(self):
        """Test that DUMMY nodes get sequential indices starting from start_index."""
        from generate_mapping import discover_node_inflows
        
        sections = {
            'DWF': [
                ['J1', 'FLOW', '1.0', 'DUMMY', '""', '""', '""'],
                ['J2', 'FLOW', '2.0', 'DUMMY', '""', '""', '""'],
                ['J3', 'FLOW', '1.5', 'DUMMY', '""', '""', '""']
            ]
        }
        nodes = discover_node_inflows(sections, start_index=5)
        
        # Should have 3 DUMMY nodes starting at index 5
        self.assertEqual(len(nodes), 3)
        self.assertEqual(nodes[0].index, 5)
        self.assertEqual(nodes[1].index, 6)
        self.assertEqual(nodes[2].index, 7)
    
    def test_case_sensitive_dummy_matching(self):
        """Test that DUMMY matching is case-sensitive."""
        from generate_mapping import discover_node_inflows
        
        sections = {
            'DWF': [
                ['J1', 'FLOW', '1.0', 'DUMMY', '""', '""', '""'],
                ['J2', 'FLOW', '2.0', 'dummy', '""', '""', '""'],
                ['J3', 'FLOW', '1.5', 'Dummy', '""', '""', '""']
            ]
        }
        nodes = discover_node_inflows(sections, start_index=1)
        
        # Should have 1 DUMMY node (only J1 matches)
        self.assertEqual(len(nodes), 1)
        self.assertEqual(nodes[0].name, "J1")
    
    def test_dummy_in_any_pattern_position(self):
        """Test that DUMMY is detected in any pattern position (Pattern1-4)."""
        from generate_mapping import discover_node_inflows
        
        sections = {
            'DWF': [
                ['J1', 'FLOW', '1.0', 'DUMMY', '""', '""', '""'],  # Pattern1
                ['J2', 'FLOW', '2.0', 'PATTERN1', 'DUMMY', '""', '""'],  # Pattern2
                ['J3', 'FLOW', '1.5', 'PATTERN1', 'PATTERN2', 'DUMMY', '""'],  # Pattern3
                ['J4', 'FLOW', '1.2', 'PATTERN1', 'PATTERN2', 'PATTERN3', 'DUMMY']  # Pattern4
            ]
        }
        nodes = discover_node_inflows(sections, start_index=1)
        
        # Should have 4 DUMMY nodes (DUMMY in different pattern positions)
        self.assertEqual(len(nodes), 4)
        self.assertEqual(nodes[0].name, "J1")
        self.assertEqual(nodes[1].name, "J2")
        self.assertEqual(nodes[2].name, "J3")
        self.assertEqual(nodes[3].name, "J4")
    
    def test_real_swmm_file_with_dummy_nodes(self):
        """Test node inflow discovery on the test_model_nodes.inp file."""
        test_file = os.path.join(os.path.dirname(__file__), 'test_model_nodes.inp')
        
        if not os.path.exists(test_file):
            self.skipTest(f"Test file not found: {test_file}")
        
        sections, error = parse_inp_file(test_file)
        self.assertIsNone(error)
        
        from generate_mapping import discover_node_inflows
        nodes = discover_node_inflows(sections, start_index=1)
        
        # Should have 2 unique DUMMY nodes (J1 appears twice, J3 once, J2 has non-DUMMY pattern)
        self.assertEqual(len(nodes), 2)
        self.assertEqual(nodes[0].name, "J1")
        self.assertEqual(nodes[0].index, 1)
        self.assertEqual(nodes[1].name, "J3")
        self.assertEqual(nodes[1].index, 2)
