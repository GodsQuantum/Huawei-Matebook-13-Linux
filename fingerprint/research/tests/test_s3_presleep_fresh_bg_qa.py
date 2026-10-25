#!/usr/bin/env python3
"""Guard native pre-S3 ImageBase refresh against stale or finger-on data."""
import unittest
from pathlib import Path
S=(Path(__file__).resolve().parents[2]/"driver/goodix51a0/goodix51a0.c").read_text()
class S3PreSleepQA(unittest.TestCase):
 def setUp(self):
  start=S.index("gx_dev_suspend (FpDevice *dev)")
  end=S.index("gx_resume_preflight_thread",start)
  self.part=S[start:end]
 def test_opt_in_and_stale_only(self):
  s=self.part
  self.assertIn('g_strcmp0 (g_getenv ("GXFP_S3_FRESH_BG_QA"), "1") == 0',s)
  self.assertIn("if (pre_sleep_clean &&",s)
  self.assertIn("!gx_background_is_fresh (self)",s)
 def test_refresh_before_sleep_and_park_ack(self):
  s=self.part
  self.assertLess(s.index("gx_warm_validate (self)"),s.index("gxfp_build_sleep (&packet)"))
  self.assertLess(s.index("gxfp_build_sleep (&packet)"),s.index("gx_s3_snapshot_clean_bg (self)"))
  self.assertIn("self->bg_last_refresh_us <= old_refresh_us",s)
  self.assertIn("!gx_background_is_fresh (self)",s)
 def test_after_refresh_must_prove_finger_off(self):
  s=self.part
  self.assertIn("gx_fdt_probe_ex (self, fdt_after, &touch_after)",s)
  self.assertIn("(touch_after & 0x3fu) != 0",s)
  self.assertIn("gx_fdt_mean (fdt_after) < GOODIX_FDT_ABS",s)
  self.assertIn("GX_DIAG_BASELINE_MAX_DRIFT",s)
  self.assertIn("pre_sleep_clean = FALSE",s)
 def test_expired_or_incomplete_reference_never_saved(self):
  self.assertIn("idle && parked && warm && pre_sleep_clean",self.part)
  self.assertIn("!self->bg_dirty && self->bg_frame && self->have_fdt",self.part)
  self.assertIn("GX_S3_CLEAN_REFERENCE_PRE_AGE_MAX_US",S)
  self.assertIn("GX_S3_CLEAN_REFERENCE_MAX_US",S)
  self.assertIn("gx_s3_discard_clean_bg (self)",self.part)
 def test_security_and_native_s3_no_custom_daemon(self):
  self.assertIn("GX_MATCH_THRESHOLD            7",S)
  self.assertIn("decision=final-for-pose",S)
  self.assertIn("gx_send_capture_cleanup (self)",S)
  self.assertIn("GXFP51A0 NATIVE_S3_PARK idle=",S)
  self.assertNotIn("GXFP_AUTO_REENROLL",S)
  self.assertNotIn("GXFP_FAST_RETRY_QA",S)
  self.assertNotIn("GXFP_VERIFY_SCORE_CACHE_QA",S)
if __name__=="__main__":
 unittest.main(verbosity=2)
