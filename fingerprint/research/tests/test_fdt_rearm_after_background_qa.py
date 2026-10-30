#!/usr/bin/env python3
"""QA45 source gate: FDT-down only after truly accepted finger-off warm rebase."""
from pathlib import Path
root=Path(__file__).resolve().parents[2]
s=(root/'driver/goodix51a0/goodix51a0.c').read_text()
a=s[s.index('static gboolean\ngx_post_background_fdt_arm_qa'):s.index('/* Pre-processing: subtract the background',s.index('static gboolean\ngx_post_background_fdt_arm_qa'))]
assert 'GXFP_FDT_REARM_AFTER_BG_QA' in a
assert 'warm_bg_refreshed = FALSE;' in a
assert a.count('warm_bg_refreshed = TRUE;') == 1
assert a.index('warm_bg_refreshed = TRUE;') > a.index('self->bg_last_refresh_us > previous_refresh')
assert a.index('self->bg_last_refresh_us > previous_refresh') > a.index('gx_warm_validate (self)')
assert a.index('!gx_wakeup_mcu (self)') < a.index('warm_bg_refreshed &&')
assert 'if (warm_bg_refreshed &&' in a
assert 'g_strcmp0 (g_getenv ("GXFP_FDT_REARM_AFTER_BG_QA"), "1") == 0' in a
assert '!gx_post_background_fdt_arm_qa (self))\n        return FALSE;' in a
assert 'self->capture_recovery_pending = TRUE;' in a
assert 'gx_send_plain_drain (self, recipe.steps[i].inner' in a
assert a.index('gxfp_build_post_background_fdt_arm_recipe (&recipe)') < a.index('gx_send_plain_drain')
assert 'return gx_wakeup_mcu (self);' not in a[a.index('gx_session_start ('):a.index('/* Defensive fallback')]
src=(root/'driver/goodix51a0/gx51_capture_recipe.c').read_text()
fn=src[src.index('bool gxfp_build_post_background_fdt_arm_recipe('):src.index('bool gxfp_build_fdt_probe(',src.index('bool gxfp_build_post_background_fdt_arm_recipe('))]
assert 'gxfp_build_fdt_command(0x0du, FDT_FINGER_MODE' in fn
assert 'gxfp_build_fdt_command(0x0cu, FDT_FINGER_MODE' in fn
assert '0x20u' not in fn and 'get_image' not in fn
assert 'gxfp_build_post_background_fdt_arm_recipe' in (root/'driver/goodix51a0/gx51_capture_recipe.h').read_text()
print("FDT_REARM_SOURCE_GATE=PASS")
