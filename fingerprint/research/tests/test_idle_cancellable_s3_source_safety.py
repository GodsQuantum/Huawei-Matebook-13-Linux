#!/usr/bin/env python3
'''No libfprint action cancellable may be retrieved during idle S3 prewarm.'''
import re
import unittest
from pathlib import Path
SRC = Path(__file__).resolve().parents[2]/'driver/goodix51a0/goodix51a0.c'
s=SRC.read_text()
class CancellablePrewarm(unittest.TestCase):
 def test_single_guarded_getter(self):
  self.assertEqual(len(re.findall(r'(?m)^\s*return fpi_device_get_cancellable\s*\(',s)),1)
  helper=s[s.index('gx_action_cancellable_if_active ('):s.index('gx_action_cancellable_if_active (')+710]
  self.assertIn('fpi_device_get_current_action (dev)',helper)
  self.assertIn('FPI_DEVICE_ACTION_NONE',helper)
  self.assertLess(helper.index('FPI_DEVICE_ACTION_NONE'),helper.index('fpi_device_get_cancellable (dev)'))
 def test_tls_setup_checks_idle_guard(self):
  self.assertIn('GCancellable *c = gx_action_cancellable_if_active (self);',s)
  self.assertEqual(s.count('gx_action_cancellable_if_active (self)'),3)
 def test_ssm_cancellation_checks_guard(self):
  key='gx_cancelled (FpDevice *dev, FpiSsm *ssm)'
  chunk=s[s.rindex(key):s.rindex(key)+525]
  self.assertIn('gx_action_cancellable_if_active (FPI_DEVICE_GOODIX51A0 (dev))',chunk)
 def test_cancel_still_detected_for_live_user_auth(self):
  self.assertIn('g_cancellable_is_cancelled (c)',s)
  self.assertIn('fpi_ssm_mark_failed (ssm, g_error_new (G_IO_ERROR, G_IO_ERROR_CANCELLED',s)
 def test_biometric_thresholds_and_no_bypass(self):
  self.assertRegex(s,r'(?m)^#define GX_MATCH_THRESHOLD\s+7\s*$')
  self.assertIn('background contaminated by touch',s)
  self.assertIn('gx_warm_abandon (self)',s)
  self.assertNotIn('pam_permit.so',s)
if __name__=='__main__': unittest.main(verbosity=2)
