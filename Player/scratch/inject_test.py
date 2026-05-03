import os
import psutil
from pyinjector import inject

def find_minecraft_pid():
    for proc in psutil.process_iter(['pid', 'name']):
        if proc.info['name'] in ['javaw.exe', 'java.exe']:
            try:
                cmdline = " ".join(proc.cmdline())
                if "minecraft" in cmdline.lower() or "badlion" in cmdline.lower():
                    return proc.info['pid']
            except:
                pass
    return None

pid = find_minecraft_pid()
if pid:
    dll_path = os.path.abspath("mapping_test.dll")
    print(f"Injecting {dll_path} into {pid}")
    inject(pid, dll_path)
    print("Done")
else:
    print("Minecraft not found")
