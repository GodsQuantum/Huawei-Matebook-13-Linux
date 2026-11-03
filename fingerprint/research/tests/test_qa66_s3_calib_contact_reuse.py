#!/usr/bin/env python3
from pathlib import Path
import unittest
F=Path(__file__).resolve().parents[2]
S=(F/'driver/goodix51a0/goodix51a0.c').read_text()
ARCH=(F/'packaging/arch/fprintd-goodix51a0.conf').read_text()
PORT=(F/'install-linux.sh').read_text()
class QA66(unittest.TestCase):
 def test_wait_proof(self):
  a=S.index('gx_wait_stable_sensor_clear (');b=S.index('gx_wait_clean_anchor (',a)
  code=S[a:b]
  for part in ('*trusted_s3_reused = FALSE','self->s3_first_tls_pending',
               'self->s3_saved_bg','(touchflag & 0x3fu) == 0x3fu',
               'mean < GOODIX_FDT_ABS - 30',
               'gx_s3_try_early_finger_reference (self)',
               '*trusted_s3_reused = TRUE','CALIBRATION_LIFT_REQUIRED',
               'g_cancellable_is_cancelled','gx_fdt_clear_stable_sample'):
   self.assertIn(part,code)
  self.assertLess(code.index('gx_s3_try_early_finger_reference (self)'),
                  code.index('*trusted_s3_reused = TRUE;'))
 def test_only_before_background(self):
  a=S.index('gx_prepare_capture_context_once (');b=S.index('#define GX_PREPARE_ATTEMPTS',a)
  code=S[a:b]
  self.assertIn('GX_CALIB_CLEAN_DWELL_US,\n                                           &trusted_s3_reused',code)
  self.assertIn('GX_CALIB_PREIMAGE_DWELL_US,\n                                               &trusted_s3_reused',code)
  self.assertEqual(code.count('if (trusted_s3_reused)'),2)
  self.assertIn('else if (!gx_wait_clean_anchor (self, &off_anchor_mean))',code)
  self.assertIn('else if (!gx_wait_sensor_clear (self, off_anchor_mean, &clear_mean))',code)
  self.assertIn('background contaminated by touch',code)
 def test_strict_baseline(self):
  a=S.index('gx_s3_try_early_finger_reference (FpiDeviceGoodix51A0 *self)\n{')
  b=S.index('gx_prepare_capture_context_once (',a)
  proof=S[a:b]
  for part in ('GX_S3_CLEAN_REFERENCE_MAX_US','gx_s3_boottime_us',
               'gx_fdt_probe_ex','mask != 0x3fu','self->s3_saved_abs',
               'memcpy (self->bg_frame','gx_s3_discard_clean_bg'):
   self.assertIn(part,proof)
  self.assertIn('#define GX_MATCH_THRESHOLD            7',S)
  self.assertNotIn('GXFP_FDT_IRQ_EDGE_WATCH_QA',S)
 def test_experimental_optin(self):
  self.assertEqual(ARCH.count('GXFP_S3_CALIB_CONTACT_REUSE_QA=1'),1)
  self.assertEqual(PORT.count('GXFP_S3_CALIB_CONTACT_REUSE_QA=1'),2)
if __name__=='__main__': unittest.main(verbosity=2)
