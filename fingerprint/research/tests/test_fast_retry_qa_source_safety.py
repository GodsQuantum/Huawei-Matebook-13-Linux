#!/usr/bin/env python3
from pathlib import Path
import unittest
S=(Path(__file__).resolve().parents[2]/"driver/goodix51a0/goodix51a0.c").read_text()
class FastRetry(unittest.TestCase):
 def test_opt_in_only_on_second_placement(self):
  a=S.index("if (t && t->rebase_before_retry)")
  b=S.index('else if (g_getenv (\"GXFP_DIAGNOSTIC_TLS_ONLY\"))',a)
  sub=S[a:b]
  self.assertIn('g_strcmp0 (g_getenv ("GXFP_FAST_RETRY_QA"), "1") == 0',sub)
  self.assertIn("if (gx_background_is_fresh (self) || fast_retry_qa)",sub)
  self.assertIn("gx_wakeup_mcu (self)",sub)
  self.assertIn("gx_warm_validate (self)",sub)
 def test_guard_clean_native_transport(self):
  a=S.index("gboolean fast_retry_qa =")
  b=S.index("if (gx_background_is_fresh",a)
  sub=S[a:b]
  for c in ["self->warm_valid", "self->production_ready",
            "self->tls_up", "self->bg_frame", "self->have_fdt",
            "!self->bg_dirty", "!self->s3_first_tls_pending"]:
   self.assertIn(c,sub)
 def test_matcher_security_same(self):
  self.assertIn("GX_MATCH_THRESHOLD            7",S)
  self.assertIn("score=%d threshold=%d candidate=%u decision=final-for-pose",S)
  self.assertIn("t->tries >= GX_VERIFY_MAX_ATTEMPTS",S)
  self.assertIn("t->rearm_mcu_before_retry = TRUE;",S)
  self.assertIn("gx_send_capture_cleanup (self)",S)
 def test_no_quick_reenroll_or_s3_mods(self):
  a=S.index("GXFP_FAST_RETRY_QA")
  self.assertIn("FAST_RETRY_QA skip stale retry ImageBase",S)
  self.assertNotIn("GXFP_AUTO_REENROLL",S)
  self.assertNotIn("GXFP_S3_PREPARK_FRESH_QA",S)
if __name__=="__main__":
 unittest.main(verbosity=2)
