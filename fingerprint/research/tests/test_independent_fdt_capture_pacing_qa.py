#!/usr/bin/env python3
"""QA48 source-level boundary regression: only FDT-only cold seed is opt-out."""
from pathlib import Path
p=Path(__file__).resolve().parents[2]
s=(p/'driver/goodix51a0/goodix51a0.c').read_text()
flag='GXFP_CAPTURE_GAP_FDT_INDEPENDENT_QA'
assert s.count(flag)==1, 'one opt-in env check expected'
seed=s[s.index('/* Protocol ACK/FDT misses measured'):s.index('self->capture_recovery_pending = FALSE;',s.index('/* Protocol ACK/FDT misses measured'))]
assert 'self->timing_scale - GX_CAPTURE_SCALE_STEP' in seed
assert 'if (protocol_floor > previous &&' in seed
assert 'g_strcmp0 (g_getenv ("'+flag+'"), "1") == 0' in seed
assert seed.index('if (protocol_floor > previous &&') < seed.index('if (protocol_floor > previous)')
assert 'protocol_floor = GX_CAPTURE_SCALE_MIN' in seed
assert 'self->capture_gap_scale = protocol_floor' in seed
assert 'capture pacing seeded from protocol calibration' in seed
assert 'int previous = MAX (self->capture_gap_scale, GX_CAPTURE_SCALE_MIN)' in seed
# Existing real GET_IMAGE transport retry calibrator remains independent.
retry=s[s.index('gx_capture_pacing_success (FpiDeviceGoodix51A0 *self)\n{'):s.index('/* Establishes the TLS channel.',s.index('gx_capture_pacing_success (FpiDeviceGoodix51A0 *self)\n{'))]
assert 'if (self->capture_retry_seen)' in retry
assert 'MAX (previous + GX_CAPTURE_SCALE_STEP, protocol_floor)' in retry
assert flag not in retry
assert 'GX_GET_IMAGE_ACK_TIMEOUT_MS 1000' in s
assert 'GX_MATCH_THRESHOLD            7' in s
# Model guarded seed arithmetic: OFF remains old behavior, ON does not
# demote actual already-learned image pacing.
def effective_seed(protocol_pct, learned_pct, enabled):
    floor=max(100,min(250,protocol_pct-50))
    old=max(100, learned_pct)
    if enabled and floor>old:floor=100
    return max(old, floor)
assert effective_seed(300,100,False)==250
assert effective_seed(300,100,True)==100
assert effective_seed(300,200,True)==200
assert effective_seed(100,100,True)==100
assert effective_seed(150,100,False)==100
print('FDT_CAPTURE_PACING_INDEPENDENCE_SOURCE_GATE=PASS')
