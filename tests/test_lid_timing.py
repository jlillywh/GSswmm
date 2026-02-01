"""
Test when LID data becomes available in SWMM5
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

swmm.swmm_step.argtypes = [ctypes.POINTER(ctypes.c_double)]
swmm.swmm_step.restype = ctypes.c_int

swmm.swmm_getCount.argtypes = [ctypes.c_int]
swmm.swmm_getCount.restype = ctypes.c_int

swmm.swmm_getIndex.argtypes = [ctypes.c_int, ctypes.c_char_p]
swmm.swmm_getIndex.restype = ctypes.c_int

swmm.swmm_getLidUCount.argtypes = [ctypes.c_int]
swmm.swmm_getLidUCount.restype = ctypes.c_int

swmm.swmm_end.argtypes = []
swmm.swmm_end.restype = ctypes.c_int

swmm.swmm_close.argtypes = []
swmm.swmm_close.restype = ctypes.c_int

# Test with LID model
model_path = os.path.join(os.path.dirname(__file__), '..', 'examples', 'LID Treatment', 'LID_Model.inp')
print(f"Testing LID data availability timing")
print(f"Model: {model_path}\n")

# Open SWMM
err = swmm.swmm_open(model_path.encode(), b"test.rpt", b"test.out")
print(f"1. After swmm_open: err={err}")

if err == 0:
    # Check subcatchment count
    swmm_SUBCATCH = 1
    subcatch_count = swmm.swmm_getCount(swmm_SUBCATCH)
    print(f"   Subcatchment count: {subcatch_count}")
    
    # Try to get S1 index
    s1_idx = swmm.swmm_getIndex(swmm_SUBCATCH, b"S1")
    print(f"   S1 index: {s1_idx}")
    
    if s1_idx >= 0:
        # Try to get LID count BEFORE swmm_start
        lid_count = swmm.swmm_getLidUCount(s1_idx)
        print(f"   LID count for S1 (BEFORE start): {lid_count}")
    
    # Start SWMM
    print(f"\n2. Calling swmm_start...")
    err = swmm.swmm_start(1)
    print(f"   After swmm_start: err={err}")
    
    if err == 0:
        # Check again after start
        s1_idx = swmm.swmm_getIndex(swmm_SUBCATCH, b"S1")
        print(f"   S1 index: {s1_idx}")
        
        if s1_idx >= 0:
            lid_count = swmm.swmm_getLidUCount(s1_idx)
            print(f"   LID count for S1 (AFTER start): {lid_count}")
        
        # Try stepping once
        print(f"\n3. Calling swmm_step...")
        elapsed = ctypes.c_double()
        err = swmm.swmm_step(ctypes.byref(elapsed))
        print(f"   After swmm_step: err={err}, elapsed={elapsed.value}")
        
        if err == 0:
            # Check again after step
            s1_idx = swmm.swmm_getIndex(swmm_SUBCATCH, b"S1")
            print(f"   S1 index: {s1_idx}")
            
            if s1_idx >= 0:
                lid_count = swmm.swmm_getLidUCount(s1_idx)
                print(f"   LID count for S1 (AFTER step): {lid_count}")
                
                if lid_count > 0:
                    print(f"\n   SUCCESS! LID data available after first step")
                    for i in range(lid_count):
                        name_buf = ctypes.create_string_buffer(64)
                        swmm.swmm_getLidUName(s1_idx, i, name_buf, 64)
                        print(f"     LID[{i}]: {name_buf.value.decode()}")
        
        swmm.swmm_end()
    
    swmm.swmm_close()

print("\n" + "="*60)
print("CONCLUSION:")
print("="*60)
print("Check when LID count becomes > 0")
print("This will tell us when to call the LID API functions")
