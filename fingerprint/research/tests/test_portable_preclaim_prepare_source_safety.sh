#!/usr/bin/env bash
set -euo pipefail
driver="$(cd "$(dirname "$0")/../.." && pwd)/driver/goodix51a0/goodix51a0.c"

python3 - "$driver" <<'PY2'
from pathlib import Path
import re, sys
s=Path(sys.argv[1]).read_text()

def fn(name):
    m=re.search(r"\b"+re.escape(name)+r"\s*\([^;{}]*\)\s*\n\{", s)
    assert m is not None, name
    b=s.find("{", m.end()-1); depth=0
    for i in range(b, len(s)):
        if s[i]=="{": depth+=1
        elif s[i]=="}":
            depth-=1
            if depth==0: return s[m.start():i+1]
    raise AssertionError(name)

cold=fn("gx_cold_prepare")
probe=fn("gx_dev_probe")
open_=fn("gx_dev_open")
session=fn("gx_session_start")
close=fn("gx_dev_close")

assert "production_ready" in s and "warm_valid" in s
assert "probe_prewarm" not in s

# rel71.38 restores rel23 early probe readiness using FULL rel71.36 prep,
# not the rejected rel71.37 shortened TLS path.
assert "gx_cold_prepare (self)" in probe
assert "gx_warm_available (self)" in probe
assert "gx_transport_open" in probe and "gx_transport_close" in probe
assert "gx_gpio_reset (self)" in probe
assert probe.index("gx_transport_close (self)") < probe.index("gx_gpio_reset (self)")
assert probe.index("gx_gpio_reset (self)") < probe.rindex("gx_transport_open (dev, &err)")
assert "if (diagnostic)" in probe

# Claim performs bounded preparation and never reports success after a failed prepare.
assert "gx_prepare_capture_context (self, FALSE)" in cold
assert "if (!gx_cold_prepare (self))" in open_
assert "fpi_device_error_new_msg" in open_
assert "gx_warm_fast_ready (self)" in open_
assert "gx_warm_validate (self)" in open_
assert open_.index("gx_warm_fast_ready (self)") < open_.index("gx_warm_validate (self)")
assert "reusing native warm context without blocking background recapture" in open_

# Operations can still defensively rebuild if an external client bypasses normal lifecycle.
assert "gx_prepare_capture_context (self, FALSE)" in session
assert "capture_recovery_pending" in session

# A validated warm session is retained across ordinary Claim/Release,
# including repeated close() cleanup on the same FpDevice. Completeness is
# centralized in gx_warm_available(); production_ready is only an action flag.
assert "self->production_ready = FALSE" in close
assert "!self->force_cold_reset && gx_warm_available (self)" in close
assert "self->production_ready && self->warm_valid" not in close
assert "self->force_cold_reset" in close
PY2

echo 'test_portable_preclaim_prepare_source_safety: OK'
