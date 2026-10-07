#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/../.." && pwd)"
src="$root/driver/goodix51a0/goodix51a0.c"

grep -Fq '#define GX_MATCH_THRESHOLD            7' "$src"
grep -Fq '#define GX_TEMPLATE_VERSION 4u' "$src"
grep -Fq '#define GX_WARM_IDLE_TTL_US (G_GINT64_CONSTANT(12) * 60 * 60 * G_USEC_PER_SEC)' "$src"
grep -Fq 'G_STATIC_ASSERT (GX_WARM_IDLE_TTL_US == G_GINT64_CONSTANT(43200000000));' "$src"
grep -Fq 'gx_warm_fast_ready' "$src"
grep -Fq 'GXFP51A0 FAST_READY in %d ms' "$src"
grep -Fq '#define GX_BG_FRESH_MAX_US (G_GINT64_CONSTANT(20) * G_USEC_PER_SEC)' "$src"
grep -Fq 'G_STATIC_ASSERT (GX_BG_FRESH_MAX_US == G_GINT64_CONSTANT(20000000));' "$src"
grep -Fq 'fresh ImageBase ready before biometric READY' "$src"

python3 - "$src" <<'PY'
from pathlib import Path
import re, sys
s=Path(sys.argv[1]).read_text()

def block(name):
    m=re.search(r"\b"+re.escape(name)+r"\s*\([^;{}]*\)\s*\n\{", s)
    assert m, name
    b=s.find("{", m.end()-1)
    d=0
    for i in range(b,len(s)):
        if s[i]=="{": d+=1
        elif s[i]=="}":
            d-=1
            if d==0: return s[m.start():i+1]
    raise AssertionError(name)

fast=block("gx_warm_fast_ready")
opn=block("gx_dev_open")
session=block("gx_session_start")
warm=block("gx_warm_validate")
sus=block("gx_dev_suspend")

# Fast readiness proves only the finger-detect/FDT path. It must not block on
# encrypted image/background capture before declaring the reader ready.
assert "gx_fdt_probe_ex" in fast
assert "gx_capture_frame" not in fast
assert "gx_prepare_capture_context" not in fast
assert "gx_gpio_reset" not in fast

# Healthy warm Claim takes the FDT-only path first. ImageBase freshness is a
# separate biometric concern handled later on the worker before READY.
assert opn.index("gx_warm_fast_ready (self)") < opn.index("gx_warm_validate (self)")
assert "gx_background_is_fresh (self)" in session
assert "gx_warm_validate (self)" in session
assert "refreshing before biometric READY" in session
prod = session[session.index("if (self->production_ready"): ]
assert prod.index("gx_background_is_fresh (self)") < prod.index("gx_wakeup_mcu (self)")
assert "self->bg_last_refresh_us = g_get_monotonic_time ()" in warm
assert "GX_BG_FRESH_MAX_US" not in fast

# S3 always invalidates warm reuse.
assert "self->force_cold_reset = TRUE" in sus
assert "self->warm_valid = FALSE" in sus

# No matcher/template weakening.
assert "#define GX_MATCH_THRESHOLD            7" in s
assert "#define GX_TEMPLATE_VERSION 4u" in s
PY

echo 'test_fast_ready_source_safety: OK'
