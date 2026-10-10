#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/../.." && pwd)"
driver="$root/driver/goodix51a0/goodix51a0.c"

python3 - "$driver" <<'PY2'
from pathlib import Path
import sys
s=Path(sys.argv[1]).read_text()

rec=s[s.index("gx_recover_capture_context"):s.index("gx_prepare_capture_context", s.index("gx_recover_capture_context"))]
assert rec.index("gx_transport_close (self);") < rec.index("gx_reset_detached_s3_boundary (self);")
assert rec.index("gx_reset_detached_s3_boundary (self);") < rec.index("gx_transport_open (FP_DEVICE (self)")
assert "RESET_TRACE transport reopened after detached GPIO264 reset" in rec

op=s[s.index("gx_dev_open (FpDevice *dev)"):s.index("gx_dev_close (FpDevice *dev)")]
cold=op.index('NATIVE_COLD_QUIESCE before SPI open')
reset=op.index("gx_reset_detached_s3_boundary (self);", cold)
open_=op.index("gx_transport_open (dev, &err)", reset)
assert cold < reset < open_
assert "open -> GPIO reset" in op
assert "Healthy same-process warm state is the only case that skips this reset" in op

warm=op.index("warm context failed readiness validation")
close2=op.index("gx_transport_close (self);", warm)
reset2=op.index("gx_gpio_reset (self);", close2)
open2=op.index("gx_transport_open (dev, &err)", reset2)
assert warm < close2 < reset2 < open2
PY2

echo 'test_detached_reset_transport_source_safety: OK'
