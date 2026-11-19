#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "$0")/../.." && pwd)"
src="$root/driver/goodix51a0/goodix51a0.c"

grep -Fq '#define GX_MATCH_THRESHOLD            7' "$src"
grep -Fq '#define GX_TEMPLATE_VERSION 4u' "$src"
grep -Fq 'rebase_before_retry' "$src"
grep -Fq 'scheduling fresh background after' "$src"
grep -Fq 'refreshing stale background+FDT after' "$src"

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

poll_on = block("gx_poll_on")
poll_off = block("gx_poll_off")
rebase = block("gx_retry_rebase_after_release")
session = block("gx_session_thread")
fast = block("gx_warm_fast_ready")

# First-touch latency stays on the FDT-only fast path. A background image is
# never added to FAST_READY itself.
assert "gx_fdt_probe_ex" in fast
assert "gx_capture_frame" not in fast
assert "gx_warm_validate" not in fast

# A usable below-threshold press already sets rearm_mcu_before_retry elsewhere.
# Once release is confirmed, consume that signal by scheduling SESSION work,
# never by performing a blocking background capture in the GLib main loop.
for poll in (poll_off, poll_on):
    assert poll.index("gx_fdt_release_confirmed (self, cur, touchflag)") < poll.index(
        "gx_retry_rebase_after_release (ssm, t)"
    )
    assert "gx_warm_validate" not in poll
assert "gx_warm_validate" not in rebase
assert rebase.index("t->rebase_before_retry = TRUE") < rebase.index(
    "fpi_ssm_jump_to_state (ssm, GX_ST_SESSION)"
)

# The SESSION worker clears the one-shot retry flag. A still-fresh ImageBase
# rearms immediately; only an actually stale reference pays WARM_REBASE.
assert "t->rebase_before_retry = FALSE" in session
assert "gx_background_is_fresh (self)" in session
assert "retry ImageBase still fresh age=%d ms" in session
assert "rearming MCU immediately for retry" in session
assert "previous_refresh = self->bg_last_refresh_us" in session
assert "gx_warm_validate (self)" in session
assert "self->bg_last_refresh_us > previous_refresh" in session
assert "retry ImageBase refresh deferred" in session
assert "gx_wakeup_mcu (self)" in session
assert session.index("gx_background_is_fresh (self)") < session.index("gx_warm_validate (self)")
assert "gx_session_start (self)" in session  # bounded recovery fallback remains

# Biometric policy is unchanged.
assert "#define GX_MATCH_THRESHOLD            7" in s
assert "#define GX_TEMPLATE_VERSION 4u" in s
PY

echo 'test_retry_fresh_rebase_source_safety: OK'
