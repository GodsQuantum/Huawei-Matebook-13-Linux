#!/usr/bin/env python3
from pathlib import Path
import unittest
S=(Path(__file__).resolve().parents[2]/'driver/goodix51a0/goodix51a0.c').read_text()
def fn(name):
 a=S.index(chr(10)+name+' (')
 i=S.index('{',a);d=0
 for k in range(i,len(S)):
  if S[k]=='{':d+=1
  elif S[k]=='}':
   d-=1
   if d==0:return S[i:k+1]
 raise ValueError(name)
class HeldFinger(unittest.TestCase):
 def test_rolling_background_stored_only_after_proven_clear_calibration(self):
  f=fn('gx_prepare_capture_context_once')
  self.assertIn('GXFP_HELD_FINGER_ROLLING_BG_QA',f)
  self.assertIn('gx_s3_snapshot_clean_bg (self)',f)
  cache=fn('gx_s3_snapshot_clean_bg')
  for x in ('self->have_fdt','self->bg_frame','self->bg_dirty','self->tls_up',
            'GX_S3_CLEAN_REFERENCE_PRE_AGE_MAX_US'):self.assertIn(x,cache)
 def test_rolling_saved_reference_persists_when_suspended_without_warm_tls(self):
  f=fn('gx_dev_suspend')
  self.assertIn('GXFP_HELD_FINGER_ROLLING_BG_QA',f)
  self.assertIn('self->s3_saved_bg',f)
  self.assertIn('gx_s3_discard_clean_bg (self)',f)
 def test_same_pressed_finger_can_use_valid_pre_sleep_reference(self):
  f=fn('gx_prepare_capture_context_once')
  self.assertIn('GXFP_HELD_FINGER_ROLLING_BG_QA',f)
  self.assertIn('gx_s3_try_early_finger_reference (self)',f)
  self.assertIn('GX_S3_CLEAN_REFERENCE_MAX_US',S)
  self.assertIn('mask != 0x3fu && !partial_qa',S)
  self.assertIn('gx_tls_handshake (self)',S)
 def test_authenticated_image_survives_only_post_capture_cleanup_failure(self):
  start=S.rindex('gx_capture_auth_same_press (FpiDeviceGoodix51A0 *self')
  f=S[start:S.index('gx_enroll_view_connected (',start)]
  self.assertIn('GXFP_POSTAUTH_CLEANUP_DEFER_QA',f)
  self.assertIn('best_score >= GX_MATCH_THRESHOLD',f)
  self.assertIn('self->force_cold_reset = TRUE',f)
  self.assertIn('g_clear_pointer (&best_probe, gx_sift_free)',f)
 def test_recovering_session_not_reused_after_postauth_error(self):
  f=fn('gx_dev_close')
  self.assertIn('self->force_cold_reset',f)
  self.assertIn('gx_warm_abandon (self)',f)
  self.assertIn('#define GX_MATCH_THRESHOLD            7',S)
if __name__=='__main__': unittest.main()
