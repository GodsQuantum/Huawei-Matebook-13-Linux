#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/../.." && pwd)"
sig="$root/driver/goodix51a0/fastbrief/sigfm.c"
driver="$root/driver/goodix51a0/goodix51a0.c"
research="$root/research/tools/compare_capture_pair.c"

python3 - "$sig" "$driver" "$research" <<'PY2'
from pathlib import Path
import re, sys
sig=Path(sys.argv[1]).read_text()
drv=Path(sys.argv[2]).read_text()
research=Path(sys.argv[3]).read_text()

# The mask-capable primitive remains available for offline research only.
assert "sigfm_match_score_mask" in sig
assert "ransac_score_mask" in sig
assert "gx_sift_match_mask" in research

start=drv.index("gx_score_probe_against_print")
end=drv.index("/* One physical press", start)
score=drv[start:end]

# Live authentication must use only the historical per-view matcher.
assert "gx_sift_match (probe," in score
assert "gx_sift_match_mask" not in score
assert "MATCH_FUSION" not in score
assert "fusion_mask" not in score
assert re.search(r"\breturn\s+best\s*;", score)
assert not re.search(r"\breturn\s+fused\s*;", score)
PY2

echo 'test_match_view_fusion_rejected_source_safety: OK'
