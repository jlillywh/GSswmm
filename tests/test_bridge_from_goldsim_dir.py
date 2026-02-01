"""
Test loading the bridge DLL from GoldSim directory
This mimics what GoldSim does
"""
import ctypes
import os
import sys

# Change to GoldSim directory (this is what GoldSim does)
goldsim_dir = r"C:\Users\jason\OneDrive\GoldSim_Testing\GSswmm_examples\LID Treatment"
os.chdir(goldsim_dir)

print(f"Working directory: {os.getcwd()}")
print(f"Files in directory:")
for f in ['GSswmm.dll', 'swmm5.dll', 'model.inp', 'SwmmGoldSimBridge.json']:
    exists = os.path.exists(f)
    if exists:
        size = os.path.getsize(f)
        print(f"  {f}: {size} bytes")
    else:
        print(f"  {f}: NOT FOUND")

print("\nLoading bridge DLL...")
try:
    bridge = ctypes.CDLL("GSswmm.dll")
    print("Bridge DLL loaded successfully")
    
    # Get the bridge function
    bridge_func = bridge.SwmmGoldSimBridge
    bridge_func.argtypes = [ctypes.c_int, ctypes.POINTER(ctypes.c_int), 
                            ctypes.POINTER(ctypes.c_double), ctypes.POINTER(ctypes.c_double)]
    bridge_func.restype = None
    
    print("\nTesting bridge...")
    
    # Test 1: Get version
    status = ctypes.c_int(0)
    inargs = (ctypes.c_double * 10)()
    outargs = (ctypes.c_double * 20)()
    
    bridge_func(2, ctypes.byref(status), inargs, outargs)
    print(f"Version: {outargs[0]}, status={status.value}")
    
    # Test 2: Get arguments
    bridge_func(3, ctypes.byref(status), inargs, outargs)
    print(f"Arguments: {int(outargs[0])} inputs, {int(outargs[1])} outputs, status={status.value}")
    
    # Test 3: Initialize
    print("\nInitializing bridge...")
    bridge_func(0, ctypes.byref(status), inargs, outargs)
    print(f"Initialize status: {status.value}")
    
    if status.value != 0:
        print("Initialization failed!")
        if status.value == -1:
            # Get error message
            err_ptr = ctypes.cast(int(outargs[0]), ctypes.c_char_p)
            print(f"Error: {err_ptr.value.decode()}")
    else:
        print("Initialization succeeded!")
        
        # Test 4: Calculate
        print("\nCalling calculate...")
        inargs[0] = 0.0
        bridge_func(1, ctypes.byref(status), inargs, outargs)
        print(f"Calculate status: {status.value}")
        
        if status.value == 0:
            print("Output values:")
            for i in range(18):
                print(f"  Output[{i}]: {outargs[i]:.6f}")
        
        # Cleanup
        bridge_func(99, ctypes.byref(status), inargs, outargs)
        print(f"\nCleanup status: {status.value}")
    
    print("\nCheck bridge_debug.log for details")
    
except Exception as e:
    print(f"Error: {e}")
    import traceback
    traceback.print_exc()
