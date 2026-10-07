#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "$0")/../.." && pwd)"
src="$root/driver/goodix51a0/goodix51a0.c"

grep -Fq 'CLOSE_TRACE retained complete warm context' "$src"
grep -Fq '!self->force_cold_reset && gx_warm_available (self)' "$src"

python3 - "$src" <<'PY'
from pathlib import Path
import re, sys

s = Path(sys.argv[1]).read_text()

def block(name):
    m = re.search(r"\b" + re.escape(name) + r"\s*\([^;{}]*\)\s*\n\{", s)
    assert m, name
    b = s.find("{", m.end() - 1)
    depth = 0
    for i in range(b, len(s)):
        if s[i] == "{":
            depth += 1
        elif s[i] == "}":
            depth -= 1
            if depth == 0:
                return s[m.start():i + 1]
    raise AssertionError(name)

close = block("gx_dev_close")
available = block("gx_warm_available")

assert "self->warm_valid" in available
assert "self->tls_up" in available
assert "self->tls" in available
assert "self->bg_frame" in available
assert "self->have_fdt" in available

# Warm preservation is driven by context completeness, not by the transient
# production_ready action flag. A repeated close must therefore be harmless.
assert "!self->force_cold_reset && gx_warm_available (self)" in close
assert "self->production_ready && self->warm_valid" not in close
assert "self->production_ready = FALSE" in close
assert "gx_transport_close (self)" in close

# Real lifecycle invalidation remains authoritative.
assert "if (self->force_cold_reset)" in close
assert "gx_warm_abandon (self)" in close
PY

echo 'test_warm_close_idempotent_source_safety: OK'
