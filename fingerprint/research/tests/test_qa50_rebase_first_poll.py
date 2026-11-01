#!/usr/bin/env python3
"""QA50 strict FDT proof after warm rebase; source invariants."""
from pathlib import Path
s=(Path(__file__).resolve().parents[2]/'driver/goodix51a0/goodix51a0.c').read_text()
assert s.count('QA50_REBASE_TOUCH_ARMED') == 2
assert '(before_touchflag & 0x3fu) == 0x3fu' in s
assert '(after_touchflag & 0x3fu) == 0x3fu' in s
assert 'before_mean < self->fdt_abs' in s
assert 'after_mean < self->fdt_abs' in s
assert s.count('self->fast_first_poll_armed = TRUE;') == 2
assert 'self->fast_first_poll_armed = FALSE;' in s
assert s.count('g_timeout_add (1, gx_poll_first_held, ssm)') == 1
assert s.count('g_timeout_add (GX_POLL_MS, gx_poll_on, ssm)') == 2
assert 'g_strcmp0 (g_getenv ("GXFP_FIRST_POLL_HELD_QA"), "1") == 0' in s
assert 'return G_SOURCE_REMOVE;' in s[s.index('gx_poll_first_held (gpointer user_data)'):s.index('gx_poll_off (gpointer user_data)')]
assert 'memcpy (self->bg_frame, fresh_bg' in s
assert s.index('WARM_REBASE discarded: finger landed during') < s.index('memcpy (self->bg_frame, fresh_bg')
assert '#define GX_MATCH_THRESHOLD            7' in s
print('QA50_REBASE_TOUCH_SAFETY=PASS')
