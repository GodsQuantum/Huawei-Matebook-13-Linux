#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/../.." && pwd)"
driver="$root/driver/goodix51a0/goodix51a0.c"

python3 - "$driver" <<'PY2'
from pathlib import Path
import re, sys
s=Path(sys.argv[1]).read_text()
m=re.search(r"\bgx_send_plain_drain\s*\([^;{}]*\)\s*\n\{", s)
assert m
b=s.find("{",m.end()-1); depth=0
for i in range(b,len(s)):
    if s[i]=="{": depth+=1
    elif s[i]=="}":
        depth-=1
        if depth==0:
            f=s[m.start():i+1]
            break
else:
    raise AssertionError("gx_send_plain_drain block")

assert "#define GX_DRAIN_IRQ_POLL_MS 10" in f
assert "#define GX_GET_IMAGE_ACK_TIMEOUT_MS 1000" in f
assert "GX_GET_IMAGE_ACK_TIMEOUT_MS / GX_DRAIN_IRQ_POLL_MS" in f
assert "GX_GET_IMAGE_NO_REPLY_POLLS == 100" in f
assert "body[0] == 0x20u ? GX_GET_IMAGE_NO_REPLY_POLLS :" in f
assert "body[0] == 0xaeu ? 12 : 25" in f
assert "if (tls_seen || ack_seen)" in f
assert "GET_IMAGE had no ACK/TLS after %d ms ACK window;" in f
assert "GX_GET_IMAGE_ACK_TIMEOUT_MS, attempt + 1, attempts" in f
# Only a complete no-evidence window can reach the replay.
assert f.index("if (tls_seen || ack_seen)") < f.index("GET_IMAGE had no ACK/TLS after %d ms ACK window;")
PY2

echo 'test_get_image_windows_ack_window_source_safety: OK'
