#!/usr/bin/env python3
"""
Root wrapper for update_tls_ca.py.
Delegates to products/control-lamp-through-voice/scripts/update_tls_ca.py.
"""
import subprocess
import sys
from pathlib import Path

root_dir = Path(__file__).resolve().parent.parent
target_script = root_dir / "products" / "control-lamp-through-voice" / "scripts" / "update_tls_ca.py"

if __name__ == "__main__":
    cmd = [sys.executable, str(target_script)] + sys.argv[1:]
    sys.exit(subprocess.run(cmd).returncode)
