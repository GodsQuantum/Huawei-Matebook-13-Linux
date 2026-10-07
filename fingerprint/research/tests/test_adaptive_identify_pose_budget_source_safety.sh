#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/../.." && pwd)"
d="$root/driver/goodix51a0/goodix51a0.c"

grep -Fq '#define GX_SAME_PRESS_CAPTURE_ATTEMPTS 3' "$d"
! grep -Fq 'GX_REPOSE_SCORE_CUTOFF' "$d"
grep -Fq 'decision=final-for-pose' "$d"
grep -Fq 'rejected before scoring: keypoints=%u minimum=%u' "$d"
grep -Fq 'identify: match reported on press %d/%d' "$d"
grep -Fq 'identify: no-match reported after %d fixed presses' "$d"
grep -Fq 'identify: no-match press %d/%d' "$d"
grep -Fq 't->best = MAX (t->best, best);' "$d"
grep -Fq 'capture pacing seeded from protocol calibration' "$d"
grep -Fq 'self->timing_scale - GX_CAPTURE_SCALE_STEP' "$d"
grep -Fq 'not persisted' "$d"

python3 - "$d" <<'PY2'
from pathlib import Path
import re,sys
s=Path(sys.argv[1]).read_text()
assert re.search(r'^#define\s+GX_MATCH_THRESHOLD\s+7\s*$', s, re.M)
assert re.search(r'^#define\s+GX_VERIFY_MAX_ATTEMPTS\s+3\b', s, re.M)
assert re.search(r'^#define\s+GX_MIN_CAPTURE_KEYPOINTS\s+25\b', s, re.M)

b=s.index("static void\ngx_capture_done")
a=s.rfind("static GxSiftFeatures *\ngx_capture_auth_same_press", 0, b)
assert a >= 0
auth=s[a:b]

# Security invariant: no score-conditioned same-press retry. The matcher is
# called only after contrast/keypoint quality gates and the first score ends
# the current physical pose.
assert "GX_REPOSE_SCORE_CUTOFF" not in s
assert "score <= GX_REPOSE_SCORE_CUTOFF" not in s
assert "decision=final-for-pose" in auth
assert "keypoints < GX_MIN_CAPTURE_KEYPOINTS" in auth
assert auth.index("keypoints < GX_MIN_CAPTURE_KEYPOINTS") < min(
    auth.index("gx_score_probe_against_print (tmpl, probe"),
    auth.index("gx_score_probe_against_gallery (gallery, probe")
)
security=auth.index("Security boundary: the first usable image")
assert "break;" in auth[security:]
assert "score +=" not in auth
assert "best_score +=" not in auth

# Identify/Verify may retry only as new physical presses, bounded to three.
assert "t->verifying && !t->match_reported" in s
assert "t->tries < GX_VERIFY_MAX_ATTEMPTS" in s
assert "best_print && best >= GX_MATCH_THRESHOLD" in s
assert "t->tries >= GX_VERIFY_MAX_ATTEMPTS" in s
assert "t->rearm_mcu_before_retry = TRUE;" in s
PY2

echo 'test_adaptive_identify_pose_budget_source_safety: OK (single scored draw per pose)'
