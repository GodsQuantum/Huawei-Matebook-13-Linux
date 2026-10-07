#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/../.." && pwd)"
driver="$root/driver/goodix51a0/goodix51a0.c"

grep -Fq '#define GX_SAME_PRESS_CAPTURE_ATTEMPTS 3' "$driver"
grep -Fq 'Windows OnRetryCaptureIMG keeps the current physical press alive' "$driver"
grep -Fq 'gx_capture_retry_same_press_frame' "$driver"
grep -Fq 'gxfp_build_get_image (&packet)' "$driver"
grep -Fq 'rejected before scoring: contrast quality gate' "$driver"
grep -Fq 'rejected before scoring: keypoints=%u minimum=%u' "$driver"
grep -Fq 'decision=final-for-pose' "$driver"

python3 - "$driver" <<'PY2'
from pathlib import Path
import re, sys
s=Path(sys.argv[1]).read_text()
b=s.index("static void\ngx_capture_done")
a=s.rfind("static GxSiftFeatures *\ngx_capture_auth_same_press", 0, b)
assert a >= 0
auth=s[a:b]

# Same-press recapture is quality-only. Both quality gates happen before any
# biometric score is computed.
contrast=auth.index("probe = gx_features_from_pixels")
kp=auth.index("keypoints < GX_MIN_CAPTURE_KEYPOINTS")
score_v=auth.index("gx_score_probe_against_print (tmpl, probe")
score_i=auth.index("gx_score_probe_against_gallery (gallery, probe")
first_score=min(score_v, score_i)
assert contrast < kp < first_score

# The first usable/scored image is final for this physical pose.
decision=auth.index("decision=final-for-pose")
security=auth.index("Security boundary: the first usable image")
final_break=auth.index("break;", security)
assert first_score < decision < security < final_break
assert "GX_REPOSE_SCORE_CUTOFF" not in auth
assert "requesting reposition instead of same-press recapture" not in auth
assert not re.search(r"if\s*\([^)]*score[^)]*GX_MATCH_THRESHOLD[^)]*\)\s*break", auth)
assert "score +=" not in auth
assert "best_score +=" not in auth
assert "GX_MATCH_THRESHOLD +" not in auth
assert "GX_MATCH_THRESHOLD -" not in auth

# RetryCaptureIMG pacing remains available for quality failures only.
retry_a=s.index("gx_capture_retry_same_press_frame")
retry_b=s.index("#ifdef GXFP51A0_DEVELOPER", retry_a)
retry=s[retry_a:retry_b]
assert retry.index("g_usleep (gx_capture_gap_us (self))") < retry.index("gxfp_build_get_image (&packet)")
assert "if (self->capture_retry_seen)" in auth
assert auth.index("gx_capture_pacing_success (self)") < auth.index("images++")
PY2

echo 'test_same_press_verify_source_safety: OK (quality-only same-press retry)'
