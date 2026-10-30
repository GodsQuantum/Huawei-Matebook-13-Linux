#!/usr/bin/env python3
"""QA44 verify metadata-only instrumentation preserves original FDT/image ordering."""
from pathlib import Path
import re
root=Path(__file__).resolve().parents[2]
s=(root/"driver/goodix51a0/goodix51a0.c").read_text()
f=s[s.index("static gboolean\ngx_warm_validate (FpiDeviceGoodix51A0 *self)"):s.index("static gboolean\ngx_cold_prepare (FpiDeviceGoodix51A0 *self)")]
assert 'g_strcmp0 (g_getenv ("GXFP_WARM_REBASE_STAGE_QA"), "1") == 0' in f
assert 'if (trace_stages)\n    fp_info ("GXFP51A0 WARM_REBASE_STAGE_QA' in f
assert f.count("gx_fdt_probe_ex (self,")==2, "no extra FDT I/O"
assert f.count("gx_capture_warm_background_frame (self, fresh_bg)")==1
assert f.index("gx_fdt_probe_ex (self, cur, &before_touchflag)") < f.index("gx_capture_warm_background_frame (self, fresh_bg)") < f.index("gx_fdt_probe_ex (self, after, &after_touchflag)")
assert 'if (gx_fdt_touch_is_finger (before_touchflag) ||' in f
assert 'if (gx_fdt_touch_is_finger (after_touchflag) ||' in f
assert f.index('if (gx_fdt_touch_is_finger (after_touchflag) ||') < f.index('memcpy (self->bg_frame, fresh_bg,')
assert f.index('memcpy (self->bg_frame, fresh_bg,') < f.index('self->bg_last_refresh_us = g_get_monotonic_time ();')
assert 'self->capture_pacing_suppressed = previous_pacing_suppression;' in f
assert f.count('return ok;') == 1
assert f.count('fp_info ("GXFP51A0 WARM_REBASE_STAGE_QA') == 1
msg=f[f.index('fp_info ("GXFP51A0 WARM_REBASE_STAGE_QA'):f.index('self->capture_pacing_suppressed = previous_pacing_suppression;')]
for expected in ('outcome=%s','total_ms=%ld','fdt_before_ms=%ld','warm_image_ms=%ld','fdt_after_ms=%ld','refresh_accepted=%d'):
    assert expected in msg, expected
assert not re.search(r'%p|%x|%02x|raw|secret|template|psk|pmk',msg,re.I), msg
print("WARM_REBASE_STAGE_METADATA_SAFETY=PASS")
