#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/../.." && pwd)"
driver="$root/driver/goodix51a0/goodix51a0.c"

python3 - "$driver" <<'PY'
from pathlib import Path
import re, sys
s=Path(sys.argv[1]).read_text()

required = {
    "GX_ENROLL_ANCHOR_STAGES": "5",
    "GX_ENROLL_ANCHOR_MIN_SCORE": "3",
    "GX_ENROLL_EXTENSION_MIN_SCORE": "3",
    "GX_ENROLL_EXTENSION_WEAK_SCORE": "2",
    "GX_ENROLL_EXTENSION_WEAK_LINKS": "2",
}
for name, value in required.items():
    assert re.search(rf"#define {name}\s+{value}\b", s), (name, value)

assert "gx_enroll_view_connected" in s
start=s.index("gx_enroll_view_connected")
end=s.index("static void\ngx_capture_done", start)
helper=s[start:end]
assert "accepted->len == 0" in helper
assert "stage < GX_ENROLL_ANCHOR_STAGES" in helper
assert "best >= GX_ENROLL_ANCHOR_MIN_SCORE" in helper
assert "best >= GX_ENROLL_EXTENSION_MIN_SCORE" in helper
assert "weak_links >= GX_ENROLL_EXTENSION_WEAK_LINKS" in helper
assert "score >= GX_ENROLL_EXTENSION_WEAK_SCORE" in helper

cap=s[s.index("static void\ngx_capture_done"):]
assert "enroll: rejected disconnected view" in cap
assert "fpi_device_retry_new (FP_DEVICE_RETRY_CENTER_FINGER)" in cap
assert "g_ptr_array_add (t->views, f)" in cap
assert "t->stage++" in cap
assert cap.index("gx_enroll_view_connected") < cap.index("g_ptr_array_add (t->views, f)")
print("ENROLL_CONNECTIVITY_GATE_SOURCE_TEST=PASS")
PY
