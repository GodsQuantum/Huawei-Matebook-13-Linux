#!/usr/bin/env python3
from pathlib import Path
s=(Path(__file__).resolve().parents[2]/'driver/goodix51a0/goodix51a0.c').read_text()
assert '#define GX_POLL_MS     100' in s
assert s.count('g_timeout_add (1, gx_poll_first_held, ssm)') == 1
assert s.count('g_timeout_add (GX_POLL_MS, gx_poll_on, ssm)') == 2
assert 'if (gx_poll_on (user_data) == G_SOURCE_CONTINUE)' in s
assert 't->tries == 0' in s
assert 'self->fast_first_poll_armed = FALSE;' in s
assert '(touchflag & 0x3fu) == 0x3fu && mean < self->fdt_abs' in s
assert 'g_strcmp0 (g_getenv ("GXFP_FIRST_POLL_HELD_QA"), "1") == 0' in s
assert '#define GX_MATCH_THRESHOLD            7' in s
assert 'GX_BG_FRESH_MAX_US (G_GINT64_CONSTANT(20) * G_USEC_PER_SEC)' in s
print('QA49_GUARDED_FIRST_POLL=PASS')
