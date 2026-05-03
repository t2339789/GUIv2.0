import sys
import os

def convert(dll_path, header_path):
    if not os.path.exists(dll_path):
        print(f"Error: {dll_path} not found.")
        return False
    
    with open(dll_path, "rb") as f:
        data = f.read()
    
    with open(header_path, "w") as f:
        f.write("#pragma once\n\n")
        f.write(f"unsigned char rawDll[] = {{\n")
        
        # Write bytes in hex format, 12 per line
        for i, b in enumerate(data):
            f.write(f"0x{b:02x}, ")
            if (i + 1) % 12 == 0:
                f.write("\n")
        
        f.write("\n};\n")
    
    print(f"Successfully converted {dll_path} to {header_path}")
    return True

if __name__ == "__main__":
    dll = "ESP.dll"
    header = "DllData.h"
    if len(sys.argv) > 1: dll = sys.argv[1]
    if len(sys.argv) > 2: header = sys.argv[2]
    convert(dll, header)
