#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/../.." && pwd)"
sig="$root/driver/goodix51a0/fastbrief/sigfm.c"
driver="$root/driver/goodix51a0/goodix51a0.c"

python3 - "$sig" "$driver" <<'PY2'
from pathlib import Path
import re, sys
sig=Path(sys.argv[1]).read_text()
drv=Path(sys.argv[2]).read_text()

assert "sigfm_match_score_mask" in sig
assert "ransac_score_mask" in sig
assert "query_mask[matches[m].qi] = 1" in sig
maskfn=sig[sig.index("sigfm_match_score_mask"):sig.index("void\nsigfm_free_info")]
assert "cross_check_filter" in maskfn
assert "ransac_score_mask" in maskfn

start=drv.index("gx_score_probe_against_print")
end=drv.index("/* One physical press", start)
score=drv[start:end]
assert "gx_sift_match_mask" in score
assert "MATCH_FUSION baseline=%d fused=%d" in score
assert "decision=baseline" in score
assert re.search(r"\breturn\s+best\s*;", score)
assert not re.search(r"\breturn\s+fused\s*;", score)
PY2

echo 'test_match_view_fusion_shadow_source_safety: OK'
