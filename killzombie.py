import os
import sys
from pathlib import Path

def kill_zone_identifiers():
    base_path = Path(__file__).parent.resolve()
    
    print(f"[*] Starting scan in: {base_path}")
    print("-" * 50)
    
    deleted_count = 0
    error_count = 0
    
    for target_file in base_path.rglob("*Zone.Identifier"):
        if target_file.is_file():
            # แปลง \uf03a กลับเป็น ':' และทำ Fallback ป้องกัน Print แครชบน Windows CMD
            safe_name = target_file.name.replace('\uf03a', ':')
            safe_name = safe_name.encode(sys.stdout.encoding or 'utf-8', errors='replace').decode(sys.stdout.encoding or 'utf-8')
            
            try:
                target_file.unlink()
                print(f"[SUCCESS] Deleted: {safe_name}")
                deleted_count += 1
            except Exception as e:
                # ทำ Safe String สำหรับ Error Message เช่นกัน
                safe_error = str(e).encode(sys.stdout.encoding or 'utf-8', errors='replace').decode(sys.stdout.encoding or 'utf-8')
                print(f"[FAILED] Could not delete {safe_name} | Error: {safe_error}")
                error_count += 1

    print("-" * 50)
    print("[*] Operation completed.")
    print(f"    - Total deleted: {deleted_count} files")
    if error_count > 0:
        print(f"    - Failed: {error_count} files")

if __name__ == "__main__":
    # บังคับให้ Standard Output ใช้ UTF-8 ถ้าทำได้ (รองรับ Python 3.7+)
    if sys.stdout and hasattr(sys.stdout, 'reconfigure'):
        try:
            sys.stdout.reconfigure(encoding='utf-8')
        except Exception:
            pass
            
    kill_zone_identifiers()