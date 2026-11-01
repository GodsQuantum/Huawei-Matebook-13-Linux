#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/../.." && pwd)"
d="$root/driver/goodix51a0/goodix51a0.c"
grep -Fq 'case 0x32u: expected_plain_replies = 2' "$d"
grep -Fq 'case 0x36u: expected_plain_replies = 2' "$d"
grep -Fq 'case 0x50u: expected_plain_replies = 2' "$d"
grep -Fq 'case 0x80u: expected_plain_replies = 1' "$d"
grep -Fq 'case 0x82u: expected_plain_replies = 2' "$d"
grep -Fq 'case 0xaeu: expected_plain_replies = 2' "$d"
grep -Fq 'expected %d replies received; finish' "$d"
echo 'test_capture_event_drain_source_safety: OK'

grep -Fq '#define GX_FDT_DOWN_SECOND_REPLY_TIMEOUT_MS 100' "$d"
grep -Fq 'fdt_late_report_qa ? GX_FDT_DOWN_LATE_REPLY_POLLS' "$d"
grep -Fq 'g_strcmp0 (g_getenv ("GXFP_FDT_DOWN_LATE_REPORT_QA"), "1") == 0' "$d"
