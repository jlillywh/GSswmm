"""
Check if swmm5.dll has the LID API functions exported.
"""
import ctypes
import os
import sys

def check_lid_api():
    """Check if LID API functions are available in swmm5.dll"""
    dll_path = "swmm5.dll"
    
    if not os.path.exists(dll_path):
        print(f"ERROR: {dll_path} not found")
        return False
    
    print(f"Loading {dll_path}...")
    try:
        dll = ctypes.CDLL(dll_path)
        print("✓ DLL loaded successfully")
    except Exception as e:
        print(f"ERROR loading DLL: {e}")
        return False
    
    # Check for LID API functions
    functions = [
        "swmm_getLidUCount",
        "swmm_getLidUName",
        "swmm_getLidUStorageVolume"
    ]
    
    all_found = True
    for func_name in functions:
        try:
            func = getattr(dll, func_name)
            print(f"✓ Found: {func_name}")
        except AttributeError:
            print(f"✗ Missing: {func_name}")
            all_found = False
    
    return all_found

if __name__ == "__main__":
    os.chdir(os.path.dirname(os.path.abspath(__file__)))
    
    print("=" * 70)
    print("SWMM5 LID API Function Check")
    print("=" * 70)
    print()
    
    if check_lid_api():
        print()
        print("=" * 70)
        print("SUCCESS: All LID API functions are present!")
        print("=" * 70)
        sys.exit(0)
    else:
        print()
        print("=" * 70)
        print("FAILURE: Some LID API functions are missing!")
        print("=" * 70)
        print()
        print("The swmm5.dll needs to be rebuilt with the LID API extensions.")
        print("See tests/LID_API_SETUP.md for instructions.")
        sys.exit(1)
