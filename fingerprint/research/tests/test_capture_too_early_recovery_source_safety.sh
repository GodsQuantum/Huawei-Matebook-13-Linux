#!/usr/bin/env bash
set -euo pipefail
driver="$(cd "$(dirname "$0")/../.." && pwd)/driver/goodix51a0/goodix51a0.c"
grep -Fq '#define GX_CLEAR_WAIT_MS 15000' "$driver"
grep -Fq 'CALIBRATION_LIFT_REQUIRED' "$driver"
grep -Fq 'CALIBRATION_CLEAR_STABLE' "$driver"
grep -Fq 'gx_wait_sensor_clear_stable' "$driver"
python3 - "$driver" <<'PY2'
from pathlib import Path
import sys
s=Path(sys.argv[1]).read_text()
a=s.index("gx_wait_stable_sensor_clear (")
b=s.index("gx_diag_press_countdown (", a)
f=s[a:b]
assert "GX_CLEAR_WAIT_MS" in f
assert "gx_fdt_probe_ex (self, cur, &touchflag)" in f
assert "gx_fdt_clear_stable_sample" in f
assert "(touchflag & 0x3fu) != 0" in s
assert "GX_CALIB_RETOUCH_DWELL_US" in s
assert "calibration could not find a stable clean sensor" in f
assert "CALIBRATION_LIFT_REQUIRED" in f
a=s.index("gx_prepare_capture_context_once (")
b=s.index("gx_preprocess (", a)
f=s[a:b]
assert "gx_wait_sensor_clear (self, off_anchor_mean, &clear_mean)" in f
assert "FDT baseline contaminated by touch" in f
assert "gx_wait_sensor_clear_stable (self, off_anchor_mean, NULL," in f
assert "(touchflag & 0x3fu) == 0" in f
assert "mean >= GOODIX_FDT_ABS" in f
PY2
echo 'test_capture_too_early_recovery_source_safety: OK (stable no-finger gate)'
