#!/usr/bin/env python3
'''Source-path QA guard for S4 mid-capture contact rescue.

Lab only, cannot replace physical S4 or enrollment tests.
'''
from pathlib import Path
import re
import unittest

SRC=(Path(__file__).resolve().parents[2] /
     'driver/goodix51a0/goodix51a0.c').read_text()

def function(name):
    anchor=chr(10)+name+' ('
    pos=SRC.index(anchor)
    start=SRC.index('{',pos)
    depth=0
    for i in range(start,len(SRC)):
        if SRC[i]=='{': depth+=1
        elif SRC[i]=='}':
            depth-=1
            if depth==0:return SRC[start:i+1]
    raise ValueError(name)

class MidcaptureReuseSafety(unittest.TestCase):
    def test_midcapture_can_retry_saved_clean_reference_before_blocking_on_lift(self):
        code=function('gx_prepare_capture_context_once')
        rejection=code.index('background contaminated by touch:')
        fallback=code.index('gx_wait_sensor_clear_stable (self, off_anchor_mean',rejection)
        rescue=code.index('GXFP51A0 S4_MIDCAPTURE_SAVED_REFERENCE',rejection)
        assert rejection<rescue<fallback
        self.assertIn('gx_s3_try_early_finger_reference (self)',code[rejection:fallback])
        self.assertIn('GXFP_S4_MIDCAPTURE_REUSE_QA',code[rejection:fallback])
        self.assertIn('return TRUE;',code[rescue:fallback])

    def test_existing_saved_reference_is_strict_and_age_limited(self):
        code=function('gx_s3_try_early_finger_reference')
        for invariant in ('GX_S3_CLEAN_REFERENCE_MAX_US','gx_s3_boottime_us',
                          'gx_fdt_probe_ex','mask != 0x3fu',
                          'self->s3_saved_bg','self->s3_saved_fdt',
                          'self->s3_saved_abs'):
            self.assertIn(invariant,code)
        self.assertIn('return FALSE;',code)
        self.assertIn('gx_s3_discard_clean_bg',code)

    def test_no_weakened_matching_or_background_acceptance(self):
        code=function('gx_prepare_capture_context_once')
        assert code.index('background contaminated by touch:') > code.index('gx_capture_avg (self, GOODIX_BG_FRAMES, self->bg_frame)')
        self.assertIn('GX_CALIB_RETOUCH_DWELL_US',code)
        self.assertIn('GX_MATCH_THRESHOLD',SRC)
        self.assertIn('gx_tls_handshake',SRC)
        self.assertIn('g_strcmp0 (g_getenv',code)

if __name__=='__main__':
    unittest.main()
