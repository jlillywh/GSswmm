#!/usr/bin/env python3
"""
Unit tests for mapping file generation with new input types.

Tests the generate_mapping_file() function to ensure correct property
mapping for PUMP, ORIFICE, WEIR, and NODE input types.
"""

import unittest
import tempfile
import os
import sys
import json

# Add parent directory to path to import generate_mapping
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from generate_mapping import (
    InputElement, 
    OutputElement, 
    generate_mapping_file
)


class TestMappingFileGeneration(unittest.TestCase):
    """Test cases for mapping file generation with new input types."""
    
    def test_property_mapping_for_all_input_types(self):
        """Test that all input types are mapped to correct properties."""
        # Create inputs with all supported types
        inputs = [
            InputElement("ElapsedTime", "SYSTEM", 0),
            InputElement("RG1", "GAGE", 1),
            InputElement("P1", "PUMP", 2),
            InputElement("OR1", "ORIFICE", 3),
            InputElement("W1", "WEIR", 4),
            InputElement("J1", "NODE", 5)
        ]
        
        outputs = []
        content_hash = "test_hash_123"
        
        # Generate mapping file
        with tempfile.NamedTemporaryFile(mode='w', suffix='.json', delete=False) as f:
            temp_path = f.name
        
        try:
            result = generate_mapping_file(inputs, outputs, content_hash, temp_path)
            self.assertTrue(result)
            
            # Read and verify the generated JSON
            with open(temp_path, 'r', encoding='utf-8') as f:
                mapping = json.load(f)
            
            # Verify input count
            self.assertEqual(mapping['input_count'], 6)
            
            # Verify property mappings (Requirements 1.3, 2.5, 3.3)
            inputs_array = mapping['inputs']
            self.assertEqual(len(inputs_array), 6)
            
            # Check SYSTEM → ELAPSEDTIME
            self.assertEqual(inputs_array[0]['object_type'], 'SYSTEM')
            self.assertEqual(inputs_array[0]['property'], 'ELAPSEDTIME')
            
            # Check GAGE → RAINFALL
            self.assertEqual(inputs_array[1]['object_type'], 'GAGE')
            self.assertEqual(inputs_array[1]['property'], 'RAINFALL')
            
            # Check PUMP → SETTING (Requirement 1.3)
            self.assertEqual(inputs_array[2]['object_type'], 'PUMP')
            self.assertEqual(inputs_array[2]['property'], 'SETTING')
            
            # Check ORIFICE → SETTING (Requirement 2.5)
            self.assertEqual(inputs_array[3]['object_type'], 'ORIFICE')
            self.assertEqual(inputs_array[3]['property'], 'SETTING')
            
            # Check WEIR → SETTING (Requirement 2.5)
            self.assertEqual(inputs_array[4]['object_type'], 'WEIR')
            self.assertEqual(inputs_array[4]['property'], 'SETTING')
            
            # Check NODE → LATFLOW (Requirement 3.3)
            self.assertEqual(inputs_array[5]['object_type'], 'NODE')
            self.assertEqual(inputs_array[5]['property'], 'LATFLOW')
            
        finally:
            os.unlink(temp_path)
    
    def test_pump_property_mapping(self):
        """Test that PUMP inputs are mapped to SETTING property."""
        inputs = [
            InputElement("ElapsedTime", "SYSTEM", 0),
            InputElement("P1", "PUMP", 1),
            InputElement("P2", "PUMP", 2)
        ]
        
        outputs = []
        content_hash = "test_hash"
        
        with tempfile.NamedTemporaryFile(mode='w', suffix='.json', delete=False) as f:
            temp_path = f.name
        
        try:
            result = generate_mapping_file(inputs, outputs, content_hash, temp_path)
            self.assertTrue(result)
            
            with open(temp_path, 'r', encoding='utf-8') as f:
                mapping = json.load(f)
            
            # Verify both pumps have SETTING property
            self.assertEqual(mapping['inputs'][1]['name'], 'P1')
            self.assertEqual(mapping['inputs'][1]['property'], 'SETTING')
            self.assertEqual(mapping['inputs'][2]['name'], 'P2')
            self.assertEqual(mapping['inputs'][2]['property'], 'SETTING')
            
        finally:
            os.unlink(temp_path)
    
    def test_valve_property_mapping(self):
        """Test that ORIFICE and WEIR inputs are mapped to SETTING property."""
        inputs = [
            InputElement("ElapsedTime", "SYSTEM", 0),
            InputElement("OR1", "ORIFICE", 1),
            InputElement("W1", "WEIR", 2)
        ]
        
        outputs = []
        content_hash = "test_hash"
        
        with tempfile.NamedTemporaryFile(mode='w', suffix='.json', delete=False) as f:
            temp_path = f.name
        
        try:
            result = generate_mapping_file(inputs, outputs, content_hash, temp_path)
            self.assertTrue(result)
            
            with open(temp_path, 'r', encoding='utf-8') as f:
                mapping = json.load(f)
            
            # Verify orifice has SETTING property
            self.assertEqual(mapping['inputs'][1]['name'], 'OR1')
            self.assertEqual(mapping['inputs'][1]['property'], 'SETTING')
            
            # Verify weir has SETTING property
            self.assertEqual(mapping['inputs'][2]['name'], 'W1')
            self.assertEqual(mapping['inputs'][2]['property'], 'SETTING')
            
        finally:
            os.unlink(temp_path)
    
    def test_node_property_mapping(self):
        """Test that NODE inputs are mapped to LATFLOW property."""
        inputs = [
            InputElement("ElapsedTime", "SYSTEM", 0),
            InputElement("J1", "NODE", 1),
            InputElement("J2", "NODE", 2)
        ]
        
        outputs = []
        content_hash = "test_hash"
        
        with tempfile.NamedTemporaryFile(mode='w', suffix='.json', delete=False) as f:
            temp_path = f.name
        
        try:
            result = generate_mapping_file(inputs, outputs, content_hash, temp_path)
            self.assertTrue(result)
            
            with open(temp_path, 'r', encoding='utf-8') as f:
                mapping = json.load(f)
            
            # Verify both nodes have LATFLOW property
            self.assertEqual(mapping['inputs'][1]['name'], 'J1')
            self.assertEqual(mapping['inputs'][1]['property'], 'LATFLOW')
            self.assertEqual(mapping['inputs'][2]['name'], 'J2')
            self.assertEqual(mapping['inputs'][2]['property'], 'LATFLOW')
            
        finally:
            os.unlink(temp_path)
    
    def test_backward_compatibility_gage_only(self):
        """Test that rain gage only models maintain existing property mapping."""
        inputs = [
            InputElement("ElapsedTime", "SYSTEM", 0),
            InputElement("RG1", "GAGE", 1),
            InputElement("RG2", "GAGE", 2)
        ]
        
        outputs = []
        content_hash = "test_hash"
        
        with tempfile.NamedTemporaryFile(mode='w', suffix='.json', delete=False) as f:
            temp_path = f.name
        
        try:
            result = generate_mapping_file(inputs, outputs, content_hash, temp_path)
            self.assertTrue(result)
            
            with open(temp_path, 'r', encoding='utf-8') as f:
                mapping = json.load(f)
            
            # Verify rain gages have RAINFALL property
            self.assertEqual(mapping['inputs'][1]['name'], 'RG1')
            self.assertEqual(mapping['inputs'][1]['property'], 'RAINFALL')
            self.assertEqual(mapping['inputs'][2]['name'], 'RG2')
            self.assertEqual(mapping['inputs'][2]['property'], 'RAINFALL')
            
        finally:
            os.unlink(temp_path)
    
    def test_unknown_object_type_handling(self):
        """Test that unknown object types are mapped to UNKNOWN property."""
        inputs = [
            InputElement("ElapsedTime", "SYSTEM", 0),
            InputElement("UNKNOWN_ELEM", "UNKNOWN_TYPE", 1)
        ]
        
        outputs = []
        content_hash = "test_hash"
        
        with tempfile.NamedTemporaryFile(mode='w', suffix='.json', delete=False) as f:
            temp_path = f.name
        
        try:
            result = generate_mapping_file(inputs, outputs, content_hash, temp_path)
            self.assertTrue(result)
            
            with open(temp_path, 'r', encoding='utf-8') as f:
                mapping = json.load(f)
            
            # Verify unknown type gets UNKNOWN property
            self.assertEqual(mapping['inputs'][1]['object_type'], 'UNKNOWN_TYPE')
            self.assertEqual(mapping['inputs'][1]['property'], 'UNKNOWN')
            
        finally:
            os.unlink(temp_path)


if __name__ == '__main__':
    unittest.main()
