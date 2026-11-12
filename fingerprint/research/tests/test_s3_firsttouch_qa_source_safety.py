#!/usr/bin/env python3
"""Opt-in post-S3 recovery safety."""
from pathlib import Path
import re
import unittest
ROOT=Path(__file__).resolve().parents[2]
S=(ROOT/"driver/goodix51a0/goodix51a0.c").read_text()
class Safety(unittest.TestCase):
 def test_native_resume_only(self):
  a=S.index("static void\ngx_dev_resume")
  b=S.index("/* --- verification completion",a)
  self.assertIn("self->s3_first_verify_guard =",S[a:b])
  self.assertIn('g_strcmp0 (g_getenv ("GXFP_S3_FIRSTTOUCH_QA"), "1") == 0', S)
 def test_fresh_reference_never_bypasses_s3_rebase(self):
  a=S.index("if (t && t->rebase_before_retry)")
  b=S.index('else if (g_getenv ("GXFP_DIAGNOSTIC_TLS_ONLY"))',a)
  chunk=S[a:b]
  self.assertLess(chunk.index("if (self->s3_rebase_required"),
                  chunk.index("else if (gx_background_is_fresh (self))"))
  self.assertIn("gx_s3_firsttouch_rebase_qa (self)",chunk)
 def test_bad_quality_across_verification_boundary(self):
  self.assertIn("S3_FIRSTTOUCH_QA first verify had",S)
  self.assertIn("S3_FIRSTTOUCH_QA first S3 score below",S)
  a=S.index("static gboolean\ngx_session_start")
  b=S.index("/* Pre-processing: subtract the background",a)
  self.assertIn("gx_s3_firsttouch_rebase_qa (self)",S[a:b])
 def test_bounded_clean_image(self):
  a=S.index("gx_s3_firsttouch_rebase_qa (")
  b=S.index("static gboolean\ngx_session_start",a)
  part=S[a:b]
  self.assertIn("pass <= 2",part)
  self.assertIn("GX_CALIB_RETOUCH_DWELL_US, 4000",part)
  self.assertLess(part.index("gx_wait_stable_sensor_clear_bounded"),
                  part.index("gx_warm_validate (self)"))
  self.assertIn("self->bg_last_refresh_us > prior",part)
  self.assertIn("return FALSE;",part)
 def test_no_biometric_security_shortcuts(self):
  self.assertRegex(S,r"(?m)^#define GX_MATCH_THRESHOLD\s+7\s*$")
  self.assertIn("decision=final-for-pose",S)
  self.assertIn("without release wait",S)
  a=S.index("if (attempt_score >= GX_MATCH_THRESHOLD)")
  b=S.index("else if (t->tries >= GX_VERIFY_MAX_ATTEMPTS)",a)
  self.assertIn("self->s3_first_verify_guard = FALSE;",S[a:b])
if __name__=="__main__":unittest.main(verbosity=2)
