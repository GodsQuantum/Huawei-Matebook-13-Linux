#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/../.." && pwd)"
driver="$root/driver/goodix51a0/goodix51a0.c"
pkg="$root/packaging/arch/PKGBUILD"

python3 - "$driver" "$pkg" <<'PY'
from pathlib import Path
import re, sys

drv = Path(sys.argv[1]).read_text()
pkg = Path(sys.argv[2]).read_text()

assert "pkgrel=71.33" in pkg
assert re.search(r"#define GX_MATCH_THRESHOLD\s+7\b", drv)

required = {
    "GX_PIXEL_RESCUE_CANDIDATE_BEST_MIN": "4",
    "GX_PIXEL_RESCUE_SCORE_MIN": "3",
    "GX_PIXEL_RESCUE_SCORE_MAX": "6",
    "GX_PIXEL_RESCUE_MIN_OVERLAP": "1500",
    "GX_PIXEL_RESCUE_MIN_ZNCC_MILLI": "750",
    "GX_PIXEL_RESCUE_MIN_AGREE_PERMILLE": "845",
}
for name, value in required.items():
    assert re.search(rf"#define {name}\s+{value}\b", drv), (name, value)

helper_start = drv.index("gx_pixel_rescue_view_pass")
start = drv.index("gx_score_probe_against_print", helper_start)
end = drv.index("/* One physical press", start)
helper = drv[helper_start:start]
score = drv[start:end]

assert "gx_sift_pixel_overlap_metrics" in helper
assert "score < GX_PIXEL_RESCUE_SCORE_MIN" in helper
assert "score > GX_PIXEL_RESCUE_SCORE_MAX" in helper
assert "overlap >= GX_PIXEL_RESCUE_MIN_OVERLAP" in helper
assert "zncc >= GX_PIXEL_RESCUE_MIN_ZNCC_MILLI" in helper
assert "agree >= GX_PIXEL_RESCUE_MIN_AGREE_PERMILLE" in helper

assert "GXFP51A0 PIXEL_RESCUE" in score
assert "gx_pixel_rescue_view_pass" in score
assert "best >= GX_PIXEL_RESCUE_CANDIDATE_BEST_MIN" in score
assert "best < GX_MATCH_THRESHOLD" in score
assert "GXFP51A0 PIXEL_NEARMISS" in score
assert "return GX_MATCH_THRESHOLD;" in score

# Security invariants: rescue is per-view, not fusion, and the same-pose policy
# remains untouched.
assert "MATCH_FUSION" not in score
assert "fusion_mask" not in score
same_press = drv[drv.index("gx_capture_auth_same_press"):drv.index("gx_capture_done")]
assert "decision=final-for-pose" in same_press
assert "first usable image is the only scored draw" in same_press
PY

echo 'test_strict_pixel_rescue_source_safety: OK'
