#!/usr/bin/env python3
"""
Property-based test for round-trip consistency.

Property 26: Round-trip consistency
For any .inp file, generating a mapping and then using that mapping in the DLL
should result in the DLL reporting the same input/output counts that the parser discovered.

This test validates Requirements 2.1-2.5, 3.1-3.7, 6.4-6.5
"""

import unittest
import tempfile
import os
import sys
import subprocess
import json
import ctypes

# Add parent directory to path to import generate_mapping
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from generate_mapping import parse_inp_file, discover_inputs, discover_outputs, generate_mapping_file, compute_hash


class TestRoundTripConsistency(unittest.TestCase):
    """Property-based test for round-trip consistency."""
    
    def setUp(self):
        """Set up test environment."""
        # GoldSim method IDs
        self.XF_REP_ARGUMENTS = 3
        self.XF_CLEANUP = 99
        
        # GoldSim status codes
        self.XF_SUCCESS = 0
        self.XF_FAILURE = 1
        self.XF_FAILURE_WITH_MSG = -1
        
        # Find DLL path but don't load yet (load fresh for each test)
        self.dll_path = None
        # Prioritize correct build location, then test directory, then root (legacy)
        dll_paths = ['x64/Release/GSswmm.dll', 'tests/GSswmm.dll', '../x64/Release/GSswmm.dll', 'GSswmm.dll', '../GSswmm.dll']
        
        for dll_path in dll_paths:
            if os.path.exists(dll_path):
                self.dll_path = os.path.abspath(dll_path)
                break
        
        if self.dll_path is None:
            self.skipTest("DLL not found")
        
        self.dll_handle = None
    
    def tearDown(self):
        """Clean up after test."""
        # Unload DLL if loaded
        if self.dll_handle is not None:
            # Call cleanup first
            try:
                status = ctypes.c_int(0)
                inargs = (ctypes.c_double * 10)()
                outargs = (ctypes.c_double * 10)()
                self.dll_handle.SwmmGoldSimBridge(self.XF_CLEANUP, ctypes.byref(status), inargs, outargs)
            except:
                pass
            
            # Unload the DLL
            try:
                if sys.platform == 'win32':
                    ctypes.windll.kernel32.FreeLibrary.argtypes = [ctypes.c_void_p]
                    ctypes.windll.kernel32.FreeLibrary(self.dll_handle._handle)
            except:
                pass
            
            self.dll_handle = None
        
        # Clean up generated files
        if os.path.exists('SwmmGoldSimBridge.json'):
            os.remove('SwmmGoldSimBridge.json')
    
    def load_dll(self):
        """Load a fresh DLL instance for this test."""
        try:
            self.dll_handle = ctypes.CDLL(self.dll_path)
            
            # Define the bridge function signature
            self.dll_handle.SwmmGoldSimBridge.argtypes = [
                ctypes.c_int,
                ctypes.POINTER(ctypes.c_int),
                ctypes.POINTER(ctypes.c_double),
                ctypes.POINTER(ctypes.c_double)
            ]
            self.dll_handle.SwmmGoldSimBridge.restype = None
            
            return True
        except Exception as e:
            print(f"\n[ERROR] Failed to load DLL: {e}")
            return False
    
    def call_rep_arguments(self):
        """Call XF_REP_ARGUMENTS and return (status, input_count, output_count)."""
        if self.dll_handle is None:
            raise RuntimeError("DLL not loaded")
        
        status = ctypes.c_int(0)
        inargs = (ctypes.c_double * 10)()
        outargs = (ctypes.c_double * 10)()
        
        self.dll_handle.SwmmGoldSimBridge(
            self.XF_REP_ARGUMENTS,
            ctypes.byref(status),
            inargs,
            outargs
        )
        
        return status.value, int(outargs[0]), int(outargs[1])
    
    def test_roundtrip_test_model(self):
        """
        Property 26: Round-trip consistency for test_model.inp
        
        Test that parser counts match DLL reported counts.
        """
        test_file = os.path.join(os.path.dirname(__file__), 'test_model.inp')
        
        if not os.path.exists(test_file):
            self.skipTest(f"Test file not found: {test_file}")
        
        # Step 1: Parse the .inp file and discover elements
        sections, error = parse_inp_file(test_file)
        self.assertIsNone(error, f"Parser error: {error}")
        
        inputs = discover_inputs(sections)
        outputs = discover_outputs(sections)
        
        parser_input_count = len(inputs)
        parser_output_count = len(outputs)
        
        print(f"\n[Parser] Discovered {parser_input_count} inputs, {parser_output_count} outputs")
        
        # Step 2: Generate mapping file
        output_file = 'SwmmGoldSimBridge.json'
        content_hash = compute_hash(test_file)
        generate_mapping_file(inputs, outputs, content_hash, output_file)
        
        self.assertTrue(os.path.exists(output_file), "Mapping file was not generated")
        
        # Step 3: Load fresh DLL and call XF_REP_ARGUMENTS
        self.assertTrue(self.load_dll(), "Failed to load DLL")
        print(f"[INFO] Loaded fresh DLL from: {self.dll_path}")
        
        status, dll_input_count, dll_output_count = self.call_rep_arguments()
        
        self.assertEqual(status, self.XF_SUCCESS, 
                        f"XF_REP_ARGUMENTS failed with status {status}")
        
        print(f"[DLL] Reports {dll_input_count} inputs, {dll_output_count} outputs")
        
        # Step 4: Verify round-trip consistency
        self.assertEqual(parser_input_count, dll_input_count,
                        f"Input count mismatch: parser={parser_input_count}, dll={dll_input_count}")
        
        self.assertEqual(parser_output_count, dll_output_count,
                        f"Output count mismatch: parser={parser_output_count}, dll={dll_output_count}")
        
        print("[PASS] Round-trip consistency verified")
    
    def test_roundtrip_test_model_dummy(self):
        """
        Property 26: Round-trip consistency for test_model_dummy.inp
        
        Test with a model that has DUMMY rain gages.
        """
        test_file = os.path.join(os.path.dirname(__file__), 'test_model_dummy.inp')
        
        if not os.path.exists(test_file):
            self.skipTest(f"Test file not found: {test_file}")
        
        # Step 1: Parse and discover
        sections, error = parse_inp_file(test_file)
        self.assertIsNone(error, f"Parser error: {error}")
        
        inputs = discover_inputs(sections)
        outputs = discover_outputs(sections)
        
        parser_input_count = len(inputs)
        parser_output_count = len(outputs)
        
        print(f"\n[Parser] Discovered {parser_input_count} inputs, {parser_output_count} outputs")
        
        # Step 2: Generate mapping
        output_file = 'SwmmGoldSimBridge.json'
        content_hash = compute_hash(test_file)
        generate_mapping_file(inputs, outputs, content_hash, output_file)
        
        self.assertTrue(os.path.exists(output_file), "Mapping file was not generated")
        
        # Step 3: Load fresh DLL and call
        self.assertTrue(self.load_dll(), "Failed to load DLL")
        print(f"[INFO] Loaded fresh DLL from: {self.dll_path}")
        
        status, dll_input_count, dll_output_count = self.call_rep_arguments()
        
        self.assertEqual(status, self.XF_SUCCESS,
                        f"XF_REP_ARGUMENTS failed with status {status}")
        
        print(f"[DLL] Reports {dll_input_count} inputs, {dll_output_count} outputs")
        
        # Step 4: Verify consistency
        self.assertEqual(parser_input_count, dll_input_count,
                        f"Input count mismatch: parser={parser_input_count}, dll={dll_input_count}")
        
        self.assertEqual(parser_output_count, dll_output_count,
                        f"Output count mismatch: parser={parser_output_count}, dll={dll_output_count}")
        
        print("[PASS] Round-trip consistency verified")
    
    def test_roundtrip_no_raingages(self):
        """
        Property 26: Round-trip consistency for model with no rain gages.
        
        Test edge case where RAINGAGES section is missing.
        """
        test_file = os.path.join(os.path.dirname(__file__), 'test_model_no_raingages.inp')
        
        if not os.path.exists(test_file):
            self.skipTest(f"Test file not found: {test_file}")
        
        # Parse and discover
        sections, error = parse_inp_file(test_file)
        self.assertIsNone(error, f"Parser error: {error}")
        
        inputs = discover_inputs(sections)
        outputs = discover_outputs(sections)
        
        parser_input_count = len(inputs)
        parser_output_count = len(outputs)
        
        print(f"\n[Parser] Discovered {parser_input_count} inputs, {parser_output_count} outputs")
        
        # Should have only elapsed time as input
        self.assertEqual(parser_input_count, 1, "Should have only elapsed time input")
        
        # Generate mapping
        output_file = 'SwmmGoldSimBridge.json'
        content_hash = compute_hash(test_file)
        generate_mapping_file(inputs, outputs, content_hash, output_file)
        
        self.assertTrue(os.path.exists(output_file), "Mapping file was not generated")
        
        # Load fresh DLL and call
        self.assertTrue(self.load_dll(), "Failed to load DLL")
        print(f"[INFO] Loaded fresh DLL from: {self.dll_path}")
        
        status, dll_input_count, dll_output_count = self.call_rep_arguments()
        
        self.assertEqual(status, self.XF_SUCCESS,
                        f"XF_REP_ARGUMENTS failed with status {status}")
        
        print(f"[DLL] Reports {dll_input_count} inputs, {dll_output_count} outputs")
        
        # Verify consistency
        self.assertEqual(parser_input_count, dll_input_count,
                        f"Input count mismatch: parser={parser_input_count}, dll={dll_input_count}")
        
        self.assertEqual(parser_output_count, dll_output_count,
                        f"Output count mismatch: parser={parser_output_count}, dll={dll_output_count}")
        
        print("[PASS] Round-trip consistency verified")
    
    def test_roundtrip_mixed_gages(self):
        """
        Property 26: Round-trip consistency for model with mixed rain gages.
        
        Test with both DUMMY and non-DUMMY rain gages.
        """
        test_file = os.path.join(os.path.dirname(__file__), 'test_model_mixed_gages.inp')
        
        if not os.path.exists(test_file):
            self.skipTest(f"Test file not found: {test_file}")
        
        # Parse and discover
        sections, error = parse_inp_file(test_file)
        self.assertIsNone(error, f"Parser error: {error}")
        
        inputs = discover_inputs(sections)
        outputs = discover_outputs(sections)
        
        parser_input_count = len(inputs)
        parser_output_count = len(outputs)
        
        print(f"\n[Parser] Discovered {parser_input_count} inputs, {parser_output_count} outputs")
        
        # Generate mapping
        output_file = 'SwmmGoldSimBridge.json'
        content_hash = compute_hash(test_file)
        generate_mapping_file(inputs, outputs, content_hash, output_file)
        
        self.assertTrue(os.path.exists(output_file), "Mapping file was not generated")
        
        # Load fresh DLL and call
        self.assertTrue(self.load_dll(), "Failed to load DLL")
        print(f"[INFO] Loaded fresh DLL from: {self.dll_path}")
        
        status, dll_input_count, dll_output_count = self.call_rep_arguments()
        
        self.assertEqual(status, self.XF_SUCCESS,
                        f"XF_REP_ARGUMENTS failed with status {status}")
        
        print(f"[DLL] Reports {dll_input_count} inputs, {dll_output_count} outputs")
        
        # Verify consistency
        self.assertEqual(parser_input_count, dll_input_count,
                        f"Input count mismatch: parser={parser_input_count}, dll={dll_input_count}")
        
        self.assertEqual(parser_output_count, dll_output_count,
                        f"Output count mismatch: parser={parser_output_count}, dll={dll_output_count}")
        
        print("[PASS] Round-trip consistency verified")
    
    def test_roundtrip_idempotence(self):
        """
        Property 26 + Property 25: Round-trip consistency is idempotent.
        
        Running the parser multiple times should produce identical results,
        and the DLL should report the same counts each time.
        """
        test_file = os.path.join(os.path.dirname(__file__), 'test_model.inp')
        
        if not os.path.exists(test_file):
            self.skipTest(f"Test file not found: {test_file}")
        
        # Run parser twice
        sections1, error1 = parse_inp_file(test_file)
        self.assertIsNone(error1)
        inputs1 = discover_inputs(sections1)
        outputs1 = discover_outputs(sections1)
        
        sections2, error2 = parse_inp_file(test_file)
        self.assertIsNone(error2)
        inputs2 = discover_inputs(sections2)
        outputs2 = discover_outputs(sections2)
        
        # Verify parser is idempotent
        self.assertEqual(len(inputs1), len(inputs2), "Parser input count not idempotent")
        self.assertEqual(len(outputs1), len(outputs2), "Parser output count not idempotent")
        
        # Generate mapping twice
        output_file = 'SwmmGoldSimBridge.json'
        content_hash = compute_hash(test_file)
        generate_mapping_file(inputs1, outputs1, content_hash, output_file)
        
        with open(output_file, 'r') as f:
            json1 = f.read()
        
        generate_mapping_file(inputs2, outputs2, content_hash, output_file)
        
        with open(output_file, 'r') as f:
            json2 = f.read()
        
        # Verify mapping generation is idempotent
        self.assertEqual(json1, json2, "Mapping generation not idempotent")
        
        # Load fresh DLL and call twice
        self.assertTrue(self.load_dll(), "Failed to load DLL")
        print(f"\n[INFO] Loaded fresh DLL from: {self.dll_path}")
        
        status1, dll_inputs1, dll_outputs1 = self.call_rep_arguments()
        self.assertEqual(status1, self.XF_SUCCESS)
        
        status2, dll_inputs2, dll_outputs2 = self.call_rep_arguments()
        self.assertEqual(status2, self.XF_SUCCESS)
        
        # Verify DLL is idempotent
        self.assertEqual(dll_inputs1, dll_inputs2, "DLL input count not idempotent")
        self.assertEqual(dll_outputs1, dll_outputs2, "DLL output count not idempotent")
        
        # Verify round-trip consistency
        self.assertEqual(len(inputs1), dll_inputs1, "Round-trip input count mismatch")
        self.assertEqual(len(outputs1), dll_outputs1, "Round-trip output count mismatch")
        
        print(f"\n[PASS] Idempotent round-trip consistency verified")
        print(f"  Parser: {len(inputs1)} inputs, {len(outputs1)} outputs")
        print(f"  DLL: {dll_inputs1} inputs, {dll_outputs1} outputs")


if __name__ == '__main__':
    # Run with verbose output
    unittest.main(verbosity=2)
