import os
import psutil
from pyinjector import inject as py_inject

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

def main():
    pid = find_minecraft_pid()
    if not pid:
        print("Minecraft not found.")
        return

    script_dir = os.path.dirname(os.path.abspath(__file__))
    dll_path = os.path.join(script_dir, "Position_v13.dll")
    
    print(f"Injecting {dll_path} into PID {pid} using pyinjector...")
    try:
        py_inject(pid, dll_path)
        print("Injection successful (or at least no error raised).")
    except Exception as e:
        print(f"Injection failed: {e}")

if __name__ == "__main__":
    main()
