"""
Test SWMM5 LID API with the actual GoldSim model file
"""
import ctypes
import os

# Load SWMM5 DLL from GoldSim directory
goldsim_dir = r"C:\Users\jason\OneDrive\GoldSim_Testing\GSswmm_examples\LID Treatment"
dll_path = os.path.join(goldsim_dir, 'swmm5.dll')
model_path = os.path.join(goldsim_dir, 'model.inp')

print(f"DLL: {dll_path}")
print(f"Model: {model_path}")
print(f"DLL exists: {os.path.exists(dll_path)}")
print(f"Model exists: {os.path.exists(model_path)}\n")

swmm = ctypes.CDLL(dll_path)

# Define function signatures
swmm.swmm_open.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p]
swmm.swmm_open.restype = ctypes.c_int

swmm.swmm_start.argtypes = [ctypes.c_int]
swmm.swmm_start.restype = ctypes.c_int

swmm.swmm_getCount.argtypes = [ctypes.c_int]
swmm.swmm_getCount.restype = ctypes.c_int

swmm.swmm_getIndex.argtypes = [ctypes.c_int, ctypes.c_char_p]
swmm.swmm_getIndex.restype = ctypes.c_int

swmm.swmm_getLidUCount.argtypes = [ctypes.c_int]
swmm.swmm_getLidUCount.restype = ctypes.c_int

swmm.swmm_getLidUName.argtypes = [ctypes.c_int, ctypes.c_int, ctypes.c_char_p, ctypes.c_int]
swmm.swmm_getLidUName.restype = None

swmm.swmm_end.argtypes = []
swmm.swmm_end.restype = ctypes.c_int

swmm.swmm_close.argtypes = []
swmm.swmm_close.restype = ctypes.c_int

# Open and start SWMM with GoldSim model
print("Opening model...")
err = swmm.swmm_open(model_path.encode(), b"test.rpt", b"test.out")
print(f"swmm_open: {err}")

if err == 0:
    # Check subcatchment count
    swmm_SUBCATCH = 1
    subcatch_count = swmm.swmm_getCount(swmm_SUBCATCH)
    print(f"Subcatchment count: {subcatch_count}\n")
    
    print("Starting simulation...")
    err = swmm.swmm_start(1)
    print(f"swmm_start: {err}\n")
    
    if err == 0:
        # Test each subcatchment
        for subcatch_name in [b"S1", b"S2", b"S3", b"S4", b"S5", b"S6", b"Swale3", b"Swale4", b"Swale6"]:
            idx = swmm.swmm_getIndex(swmm_SUBCATCH, subcatch_name)
            if idx >= 0:
                lid_count = swmm.swmm_getLidUCount(idx)
                print(f"Subcatchment '{subcatch_name.decode()}' (idx={idx}): {lid_count} LID units")
                
                if lid_count > 0:
                    for i in range(lid_count):
                        name_buf = ctypes.create_string_buffer(64)
                        swmm.swmm_getLidUName(idx, i, name_buf, 64)
                        print(f"  LID[{i}]: {name_buf.value.decode()}")
                elif lid_count < 0:
                    print(f"  ERROR: getLidUCount returned {lid_count}")
        
        swmm.swmm_end()
    
    swmm.swmm_close()

print("\nTest complete")
