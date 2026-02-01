"""
Direct test of SWMM5 LID API functions
"""
import ctypes
import os

# Load SWMM5 DLL
dll_path = os.path.join(os.path.dirname(__file__), 'swmm5.dll')
swmm = ctypes.CDLL(dll_path)

# Define function signatures
swmm.swmm_open.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p]
swmm.swmm_open.restype = ctypes.c_int

swmm.swmm_start.argtypes = [ctypes.c_int]
swmm.swmm_start.restype = ctypes.c_int

swmm.swmm_getIndex.argtypes = [ctypes.c_int, ctypes.c_char_p]
swmm.swmm_getIndex.restype = ctypes.c_int

swmm.swmm_getLidUCount.argtypes = [ctypes.c_int]
swmm.swmm_getLidUCount.restype = ctypes.c_int

swmm.swmm_getLidUName.argtypes = [ctypes.c_int, ctypes.c_int, ctypes.c_char_p, ctypes.c_int]
swmm.swmm_getLidUName.restype = None

swmm.swmm_getLidUStorageVolume.argtypes = [ctypes.c_int, ctypes.c_int]
swmm.swmm_getLidUStorageVolume.restype = ctypes.c_double

swmm.swmm_end.argtypes = []
swmm.swmm_end.restype = ctypes.c_int

swmm.swmm_close.argtypes = []
swmm.swmm_close.restype = ctypes.c_int

# Test with LID model
model_path = os.path.join(os.path.dirname(__file__), '..', 'examples', 'LID Treatment', 'LID_Model.inp')
print(f"Testing with model: {model_path}")
print(f"Model exists: {os.path.exists(model_path)}")

# Open and start SWMM
err = swmm.swmm_open(model_path.encode(), b"test.rpt", b"test.out")
print(f"swmm_open: {err}")

if err == 0:
    err = swmm.swmm_start(1)
    print(f"swmm_start: {err}")
    
    if err == 0:
        # Get subcatchment index for S1
        swmm_SUBCATCH = 1
        s1_idx = swmm.swmm_getIndex(swmm_SUBCATCH, b"S1")
        print(f"\nSubcatchment 'S1' index: {s1_idx}")
        
        if s1_idx >= 0:
            # Get LID count
            lid_count = swmm.swmm_getLidUCount(s1_idx)
            print(f"LID count for S1: {lid_count}")
            
            if lid_count > 0:
                # Get LID names
                for i in range(lid_count):
                    name_buf = ctypes.create_string_buffer(64)
                    swmm.swmm_getLidUName(s1_idx, i, name_buf, 64)
                    print(f"  LID[{i}]: {name_buf.value.decode()}")
                    
                    # Get storage volume
                    volume = swmm.swmm_getLidUStorageVolume(s1_idx, i)
                    print(f"    Storage volume: {volume:.6f}")
            elif lid_count == 0:
                print("  No LID units found (lidCount = 0)")
            else:
                print(f"  ERROR: getLidUCount returned {lid_count}")
        
        # Test S4
        s4_idx = swmm.swmm_getIndex(swmm_SUBCATCH, b"S4")
        print(f"\nSubcatchment 'S4' index: {s4_idx}")
        
        if s4_idx >= 0:
            lid_count = swmm.swmm_getLidUCount(s4_idx)
            print(f"LID count for S4: {lid_count}")
            
            if lid_count > 0:
                for i in range(lid_count):
                    name_buf = ctypes.create_string_buffer(64)
                    swmm.swmm_getLidUName(s4_idx, i, name_buf, 64)
                    print(f"  LID[{i}]: {name_buf.value.decode()}")
        
        swmm.swmm_end()
    
    swmm.swmm_close()

print("\nTest complete")
