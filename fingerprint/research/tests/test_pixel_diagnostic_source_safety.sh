#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/../.." && pwd)"
driver="$root/driver/goodix51a0/goodix51a0.c"
matcher="$root/driver/goodix51a0/fastbrief/sigfm.c"

grep -Fq '#define GX_TEMPLATE_VERSION 4u' "$driver"
grep -Fq '#define SIGFM_VERSION 3u' "$matcher"
grep -Fq 'uint8_t *pix;' "$matcher"
grep -Fq 'sigfm_pixel_overlap_metrics' "$matcher"
grep -Fq 'pixel diagnostic: rank=%u view=%u baseline=%d' "$driver"
grep -Fq 'GXFP_MATCH_DIAGNOSTICS' "$driver"
grep -Fq 'GXFP51A0 PIXEL_RESCUE' "$driver"

python3 - "$driver" <<'PY'
from pathlib import Path
import sys
s=Path(sys.argv[1]).read_text()
a=s.index("gx_score_probe_against_print")
b=s.index("static void\ngx_capture_done",a)
f=s[a:b]

# Detailed per-view diagnostics remain opt-in. Production may use only the
# separately constrained rel71.28 rescue gate, never diagnostic logging as a
# decision input.
assert 'if (g_getenv ("GXFP_MATCH_DIAGNOSTICS"))' in f
assert "pixel diagnostic:" in f
assert "GXFP51A0 PIXEL_RESCUE" in f
assert "return GX_MATCH_THRESHOLD;" in f
assert f.rfind("return best;") > f.index("GXFP51A0 PIXEL_RESCUE")

# No release biometric dump path is introduced here.
assert "capture.pgm" not in f
assert "template.pgm" not in f
PY

echo 'test_pixel_diagnostic_source_safety: OK (diagnostics opt-in, strict rescue separate)'
