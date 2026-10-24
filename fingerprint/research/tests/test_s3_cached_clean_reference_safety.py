#!/usr/bin/env python3
'''S3 reference is RAM-only, previously verified clean, short-lived and one-time.'''
from pathlib import Path
import unittest
S=(Path(__file__).resolve().parents[2]/'driver/goodix51a0/goodix51a0.c').read_text()
def method(begin,end):return S[S.index(begin):S.index(end,S.index(begin))]
class CachedCleanReference(unittest.TestCase):
 def test_s3_reference_not_persistent(self):
  self.assertIn('s3_saved_bg',S)
  self.assertIn('CLOCK_BOOTTIME',S)
  self.assertIn('g_clear_pointer (&self->s3_saved_bg, g_free)',S)
  self.assertNotIn('open (GX_S3_BG',S)
 def test_store_only_existing_clean_reference(self):
  s=method('gx_dev_suspend (FpDevice *dev)','gx_resume_preflight_thread (')
  for v in ('gx_warm_available (self)','!self->bg_dirty','self->bg_frame','gx_s3_snapshot_clean_bg (self)'):
   self.assertIn(v,s)
  self.assertLess(s.index('gx_s3_snapshot_clean_bg (self)'),s.index('gx_warm_abandon (self)'))
 def test_s3_reuse_only_under_strict_live_finger(self):
  s=method('gx_s3_try_early_finger_reference (','gx_prepare_capture_context_once (')
  for v in ('s3_saved_bg','gx_fdt_probe_ex','0x3fu','GOODIX_FDT_ABS','gx_s3_boottime_us ()','s3_saved_at_boot_us','memcpy (self->bg_frame','gx_s3_discard_clean_bg (self)'):
   self.assertIn(v,s)
  self.assertIn('GX_S3_CLEAN_REFERENCE_MAX_US',s)
 def test_preparation_exits_with_proven_context(self):
  s=method('gx_prepare_capture_context_once (','GX_PREPARE_ATTEMPTS')
  self.assertIn('gx_s3_try_early_finger_reference (self)',s)
  self.assertIn('gx_wait_clean_anchor (self',s)
  self.assertLess(s.index('gx_s3_try_early_finger_reference (self)'),s.index('gx_wait_clean_anchor (self'))
 def test_real_biometric_match_and_background_gates_intact(self):
  self.assertIn('#define GX_MATCH_THRESHOLD            7',S)
  self.assertIn('#define GOODIX_FINGER_STD    150.0',S)
  self.assertIn('background contaminated by touch',S)
  self.assertIn('GXFP51A0 AUTH_TRACE verified positive terminal',S)
if __name__=='__main__':unittest.main(verbosity=2)
