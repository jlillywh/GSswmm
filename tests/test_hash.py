#!/usr/bin/env python3
"""
Unit tests for the content hash computation.

Tests the hash computation functionality including:
- Deterministic hash computation
- Comment exclusion
- Whitespace normalization
- Content sensitivity
"""

import unittest
import tempfile
import os
import sys

# Add parent directory to path to import generate_mapping
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from generate_mapping import compute_hash


class TestHashComputation(unittest.TestCase):
    """Test cases for content hash computation."""
    
    def test_hash_is_deterministic(self):
        """Test that computing the hash multiple times produces the same result."""
        content = """
[SECTION1]
data1 value1
data2 value2
"""
        with tempfile.NamedTemporaryFile(mode='w', suffix='.inp', delete=False) as f:
            f.write(content)
            temp_path = f.name
        
        try:
            hash1 = compute_hash(temp_path)
            hash2 = compute_hash(temp_path)
            hash3 = compute_hash(temp_path)
            
            # All hashes should be identical
            self.assertEqual(hash1, hash2)
            self.assertEqual(hash2, hash3)
        finally:
            os.unlink(temp_path)
    
    def test_hash_excludes_comments(self):
        """Test that comments are excluded from hash computation."""
        content1 = """
[SECTION1]
; This is a comment
data1 value1
data2 value2
"""
        content2 = """
[SECTION1]
; This is a different comment
data1 value1
data2 value2
"""
        content3 = """
[SECTION1]
data1 value1
data2 value2
"""
        
        with tempfile.NamedTemporaryFile(mode='w', suffix='.inp', delete=False) as f:
            f.write(content1)
            temp_path1 = f.name
        
        with tempfile.NamedTemporaryFile(mode='w', suffix='.inp', delete=False) as f:
            f.write(content2)
            temp_path2 = f.name
        
        with tempfile.NamedTemporaryFile(mode='w', suffix='.inp', delete=False) as f:
            f.write(content3)
            temp_path3 = f.name
        
        try:
            hash1 = compute_hash(temp_path1)
            hash2 = compute_hash(temp_path2)
            hash3 = compute_hash(temp_path3)
            
            # All hashes should be identical (comments excluded)
            self.assertEqual(hash1, hash2)
            self.assertEqual(hash2, hash3)
        finally:
            os.unlink(temp_path1)
            os.unlink(temp_path2)
            os.unlink(temp_path3)
    
    def test_hash_normalizes_whitespace(self):
        """Test that whitespace is normalized in hash computation."""
        content1 = """
[SECTION1]
data1 value1
data2  value2
"""
        content2 = """
[SECTION1]
data1  value1
data2 value2
"""
        content3 = """
[SECTION1]
data1\tvalue1
data2\t\tvalue2
"""
        
        with tempfile.NamedTemporaryFile(mode='w', suffix='.inp', delete=False) as f:
            f.write(content1)
            temp_path1 = f.name
        
        with tempfile.NamedTemporaryFile(mode='w', suffix='.inp', delete=False) as f:
            f.write(content2)
            temp_path2 = f.name
        
        with tempfile.NamedTemporaryFile(mode='w', suffix='.inp', delete=False) as f:
            f.write(content3)
            temp_path3 = f.name
        
        try:
            hash1 = compute_hash(temp_path1)
            hash2 = compute_hash(temp_path2)
            hash3 = compute_hash(temp_path3)
            
            # All hashes should be identical (whitespace normalized)
            self.assertEqual(hash1, hash2)
            self.assertEqual(hash2, hash3)
        finally:
            os.unlink(temp_path1)
            os.unlink(temp_path2)
            os.unlink(temp_path3)
    
    def test_hash_sensitive_to_content_changes(self):
        """Test that hash changes when content changes."""
        content1 = """
[SECTION1]
data1 value1
data2 value2
"""
        content2 = """
[SECTION1]
data1 value1
data2 value3
"""
        content3 = """
[SECTION1]
data1 value1
data3 value2
"""
        
        with tempfile.NamedTemporaryFile(mode='w', suffix='.inp', delete=False) as f:
            f.write(content1)
            temp_path1 = f.name
        
        with tempfile.NamedTemporaryFile(mode='w', suffix='.inp', delete=False) as f:
            f.write(content2)
            temp_path2 = f.name
        
        with tempfile.NamedTemporaryFile(mode='w', suffix='.inp', delete=False) as f:
            f.write(content3)
            temp_path3 = f.name
        
        try:
            hash1 = compute_hash(temp_path1)
            hash2 = compute_hash(temp_path2)
            hash3 = compute_hash(temp_path3)
            
            # All hashes should be different
            self.assertNotEqual(hash1, hash2)
            self.assertNotEqual(hash2, hash3)
            self.assertNotEqual(hash1, hash3)
        finally:
            os.unlink(temp_path1)
            os.unlink(temp_path2)
            os.unlink(temp_path3)
    
    def test_hash_excludes_empty_lines(self):
        """Test that empty lines are excluded from hash computation."""
        content1 = """
[SECTION1]
data1 value1

data2 value2
"""
        content2 = """
[SECTION1]

data1 value1
data2 value2

"""
        content3 = """
[SECTION1]
data1 value1
data2 value2
"""
        
        with tempfile.NamedTemporaryFile(mode='w', suffix='.inp', delete=False) as f:
            f.write(content1)
            temp_path1 = f.name
        
        with tempfile.NamedTemporaryFile(mode='w', suffix='.inp', delete=False) as f:
            f.write(content2)
            temp_path2 = f.name
        
        with tempfile.NamedTemporaryFile(mode='w', suffix='.inp', delete=False) as f:
            f.write(content3)
            temp_path3 = f.name
        
        try:
            hash1 = compute_hash(temp_path1)
            hash2 = compute_hash(temp_path2)
            hash3 = compute_hash(temp_path3)
            
            # All hashes should be identical (empty lines excluded)
            self.assertEqual(hash1, hash2)
            self.assertEqual(hash2, hash3)
        finally:
            os.unlink(temp_path1)
            os.unlink(temp_path2)
            os.unlink(temp_path3)
    
    def test_hash_format(self):
        """Test that hash is a valid MD5 hex string."""
        content = """
[SECTION1]
data1 value1
"""
        with tempfile.NamedTemporaryFile(mode='w', suffix='.inp', delete=False) as f:
            f.write(content)
            temp_path = f.name
        
        try:
            hash_value = compute_hash(temp_path)
            
            # MD5 hash should be 32 hex characters
            self.assertEqual(len(hash_value), 32)
            # Should only contain hex characters
            self.assertTrue(all(c in '0123456789abcdef' for c in hash_value))
        finally:
            os.unlink(temp_path)
    
    def test_hash_real_swmm_file(self):
        """Test hash computation on the test_model.inp file."""
        test_file = os.path.join(os.path.dirname(__file__), 'test_model.inp')
        
        if not os.path.exists(test_file):
            self.skipTest(f"Test file not found: {test_file}")
        
        hash1 = compute_hash(test_file)
        hash2 = compute_hash(test_file)
        
        # Hash should be deterministic
        self.assertEqual(hash1, hash2)
        
        # Hash should be valid MD5 format
        self.assertEqual(len(hash1), 32)
        self.assertTrue(all(c in '0123456789abcdef' for c in hash1))


if __name__ == '__main__':
    unittest.main()
