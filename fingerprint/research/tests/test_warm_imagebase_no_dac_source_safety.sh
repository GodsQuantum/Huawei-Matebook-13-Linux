#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/../.." && pwd)"
recipe="$root/driver/goodix51a0/gx51_capture_recipe.c"
driver="$root/driver/goodix51a0/goodix51a0.c"

python3 - "$recipe" "$driver" <<'PY2'
from pathlib import Path
import re, sys
recipe=Path(sys.argv[1]).read_text()
driver=Path(sys.argv[2]).read_text()

def block(src, name):
    m=re.search(r"\b"+re.escape(name)+r"\s*\([^;{}]*\)\s*\n\{",src)
    assert m,name
    b=src.find("{",m.end()-1); depth=0
    for i in range(b,len(src)):
        if src[i]=="{": depth+=1
        elif src[i]=="}":
            depth-=1
            if depth==0: return src[m.start():i+1]
    raise AssertionError(name)

warm=block(recipe,"gxfp_build_warm_background_capture_recipe")
cold=block(recipe,"gxfp_build_background_capture_recipe")
validate=block(driver,"gx_warm_validate")

# Warm ImageBase update keeps local validated FDT/NAV/T0 path but cannot touch
# OTP-derived calibration registers.
assert "gxfp_build_nav" in warm
assert "gxfp_build_get_image" in warm
assert "FDT_BOOT" in warm and "FDT_BACKGROUND" in warm
for addr in ("0x0220", "0x0236", "0x0238", "0x023a"):
    assert addr not in warm
assert "gxfp_build_reg_write" not in warm
assert "gxfp_build_reg_read" not in warm

# Cold calibration still owns all four DAC writes.
for addr in ("0x0220", "0x0236", "0x0238", "0x023a"):
    assert addr in cold

# Warm validator must call the warm-only frame function, not the cold recipe.
assert "gx_capture_warm_background_frame" in validate
assert "gx_capture_frame (self, fresh_bg, TRUE)" not in validate
PY2

echo 'test_warm_imagebase_no_dac_source_safety: OK'
