#!/usr/bin/env python3
"""An opt-in S3 retry must not re-score stale or finger-contaminated reference."""
from pathlib import Path
import unittest
ROOT=Path(__file__).resolve().parents[2]
C=(ROOT/"driver/goodix51a0/goodix51a0.c").read_text()
class SafeRebase(unittest.TestCase):
 def test_no_default_behavior_change(self):
  self.assertIn('g_strcmp0 (g_getenv ("GXFP_S3_STALE_REBASE_QA"), "1") == 0', C)
  self.assertIn("else if (gx_warm_validate (self))",C)
  self.assertIn("retry ImageBase refresh deferred",C)
 def test_fresh_reference_before_rearm(self):
  a=C.index('g_strcmp0 (g_getenv ("GXFP_S3_STALE_REBASE_QA"), "1")')
  b=C.index("else if (gx_warm_validate (self))",a)
  code=C[a:b]
  self.assertIn("gx_wait_stable_sensor_clear_bounded",code)
  self.assertIn("GX_CALIB_RETOUCH_DWELL_US, 4000",code)
  self.assertIn("gx_warm_validate (self)",code)
  self.assertIn("self->bg_last_refresh_us > previous_refresh",code)
  self.assertIn("if (refreshed)",code)
  self.assertIn("w->ok = FALSE;",code)
  self.assertIn("guint pass = 0; pass < 2",code)
  self.assertIn("no clean ImageBase",code)
  self.assertIn("failing closed",code)
 def test_security_and_cancellation(self):
  self.assertRegex(C,r"(?m)^#define GX_MATCH_THRESHOLD +7$")
  self.assertIn("decision=final-for-pose",C)
  self.assertIn("g_cancellable_is_cancelled (c)",C)
  self.assertIn("return gx_wait_stable_sensor_clear_bounded (self, anchor_mean, out_mean,",C)
  self.assertIn("dwell_us, GX_CLEAR_WAIT_MS",C)
if __name__=="__main__":
 unittest.main(verbosity=2)
