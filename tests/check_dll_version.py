"""
Check which SWMM5 DLL version has LID API functions
"""
import ctypes
import os
import sys

def check_dll(dll_path):
    print(f"\nChecking: {dll_path}")
    print(f"Exists: {os.path.exists(dll_path)}")
    
    if not os.path.exists(dll_path):
        return
    
    try:
        dll = ctypes.CDLL(dll_path)
        
        # Check version
        try:
            dll.swmm_getVersion.restype = ctypes.c_int
            version = dll.swmm_getVersion()
            print(f"Version: {version} (v{version//10000}.{(version%10000)//1000}.{version%1000})")
        except:
            print("Version: Unable to get version")
        
        # Check for LID API functions
        has_lid_api = True
        for func_name in ['swmm_getLidUCount', 'swmm_getLidUName', 'swmm_getLidUStorageVolume']:
            try:
                getattr(dll, func_name)
                print(f"  ✓ {func_name} found")
            except AttributeError:
                print(f"  ✗ {func_name} NOT FOUND")
                has_lid_api = False
        
        if has_lid_api:
            print("  ✓ DLL has LID API functions")
        else:
            print("  ✗ DLL is missing LID API functions - NEEDS UPDATE")
            
    except Exception as e:
        print(f"Error loading DLL: {e}")

# Check multiple locations
locations = [
    r"C:\Users\jason\python\GSswmm\swmm5.dll",
    r"C:\Users\jason\python\GSswmm\tests\swmm5.dll",
    r"C:\Users\jason\OneDrive\GoldSim_Testing\GSswmm_examples\LID Treatment\swmm5.dll",
]

print("=" * 70)
print("SWMM5 DLL Version Check")
print("=" * 70)

for loc in locations:
    check_dll(loc)

print("\n" + "=" * 70)
print("RECOMMENDATION:")
print("=" * 70)
print("If the GoldSim model directory DLL is missing LID API functions,")
print("copy the DLL from:")
print("  C:\\Users\\jason\\python\\GSswmm\\swmm5.dll")
print("to:")
print("  C:\\Users\\jason\\OneDrive\\GoldSim_Testing\\GSswmm_examples\\LID Treatment\\swmm5.dll")
