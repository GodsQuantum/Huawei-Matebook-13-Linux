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
b=s.find("{", m.end()-1); depth=0
for i in range(b, len(s)):
    if s[i]=="{": depth+=1
    elif s[i]=="}":
        depth-=1
        if depth==0:
            f=s[m.start():i+1]
            break
else:
    raise AssertionError("gx_send_plain_drain")

assert "case 0x32u: expected_plain_replies = 2" in f
assert "FDT-down ACK + data" in f
assert "#define GX_FDT_DOWN_SECOND_REPLY_TIMEOUT_MS 100" in f
assert "GX_FDT_DOWN_SECOND_REPLY_POLLS == 10" in f
assert "body[0] == 0x32u ? GX_FDT_DOWN_SECOND_REPLY_POLLS : GX_DRAIN_SILENCE" in f
assert "GXFP51A0 FDT_DOWN_DRAIN replies=%d expected=%d ack=%d" in f
assert "case 0x36u: expected_plain_replies = 2" in f
assert "case 0x50u: expected_plain_replies = 2" in f
assert "#define GX_GET_IMAGE_ACK_TIMEOUT_MS 1000" in f
assert "#define GX_GET_IMAGE_ATTEMPTS 2" in f
PY2

echo 'test_fdt_down_two_reply_source_safety: OK'
