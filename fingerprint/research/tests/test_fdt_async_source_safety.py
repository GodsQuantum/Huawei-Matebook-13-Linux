#!/usr/bin/env python3
"""QA46 invariants: never synchronize on an absent finger or bypass TLS/matcher."""
from pathlib import Path
p=Path(__file__).resolve().parents[2]
s=(p/'driver/goodix51a0/goodix51a0.c').read_text()
arm=s[s.index("gx_fdt_async_arm_qa ("):s.index("gx_session_start (",s.index("gx_fdt_async_arm_qa ("))]
wait=s[s.index("gx_poll_on (gpointer"):s.index("gx_poll_off (gpointer")]
session=s[s.index("gx_session_start ("):s.index("/* Pre-processing: subtract",s.index("gx_session_start ("))]
assert 'gxfp_build_fdt_async_arm' in arm
assert 'gxfp_parse_ack (rx, (gsize) n, 0x32u' in arm
assert 'gxfp_parse_fdt_down_event' not in arm
assert 'gx_send_plain_drain' not in arm
assert 'gxfp_parse_fdt_down_event' in wait
assert 'if (self->fdt_async_armed)' in wait
assert wait.index('if (self->fdt_async_armed)') < wait.index('else if (gx_fdt_probe_ex (self, cur')
assert 'GXFP_FDT_ASYNC_EVENT_QA' in session
assert 'fresh_bg_committed &&' in session
assert 'self->bg_last_refresh_us > previous_refresh' in session
assert 'self->capture_recovery_pending = TRUE;' in wait
assert 'self->fdt_async_armed = FALSE;' in s[s.index('gx_warm_abandon (FpiDeviceGoodix51A0 *self)'):s.index('gx_warm_discard (',s.index('gx_warm_abandon (FpiDeviceGoodix51A0 *self)'))]
assert 'if (self->capture_recovery_pending &&' in s
assert 'g_strcmp0 (g_getenv ("GXFP_FDT_ASYNC_EVENT_QA"), "1") == 0)' in s
assert 'if (!self->force_cold_reset && gx_warm_available (self))' in s
assert 'GX_MATCH_THRESHOLD' in s and 'gx_tls_decrypt_record' in s
print("QA46_ASYNC_STATE_SOURCE_GATE=PASS")
