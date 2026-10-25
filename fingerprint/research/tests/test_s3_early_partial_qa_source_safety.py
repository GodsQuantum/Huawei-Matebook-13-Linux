#!/usr/bin/env python3
"""Regression gates: S3 prewarm may use only proven-clean saved reference."""
import unittest
from pathlib import Path

S=(Path(__file__).resolve().parents[2]/"driver/goodix51a0/goodix51a0.c").read_text()

class S3EarlyPartialQA(unittest.TestCase):
 def test_strict_opt_in_and_no_normal_change(self):
  a=S.index("gx_s3_try_early_finger_reference (")
  b=S.index("gx_prepare_capture_context_once (",a)
  s=S[a:b]
  self.assertIn('g_strcmp0 (g_getenv ("GXFP_S3_EARLY_PARTIAL_QA"), "1") == 0',s)
  self.assertIn("self->s3_first_tls_pending && n >= 4 && n < 6",s)
  self.assertIn("if (mask != 0x3fu && !partial_qa)",s)
  self.assertIn("gx_fdt_mean (self->s3_saved_fdt) - mean >= 50",s)
 def test_second_real_fdt_proof_and_cancel_on_movement(self):
  a=S.index("gx_s3_try_early_finger_reference (")
  b=S.index("gx_prepare_capture_context_once (",a)
  s=S[a:b]
  self.assertIn("g_usleep (GX_CALIB_POLL_US)",s)
  self.assertIn("gx_fdt_probe_ex (self, confirm, &confirm_touch)",s)
  self.assertIn("confirm_n < 4",s)
  self.assertIn("gx_fdt_mean (self->s3_saved_fdt) - mean < 50",s)
  self.assertIn("mean >= self->s3_saved_abs",s)
  self.assertIn("mean >= GOODIX_FDT_ABS - 30",s)
 def test_reference_came_from_proven_clean_s3_and_expires(self):
  self.assertIn("pre_sleep_clean &&",S)
  self.assertIn("!self->bg_dirty && self->bg_frame && self->have_fdt",S)
  self.assertIn("age > GX_S3_CLEAN_REFERENCE_PRE_AGE_MAX_US",S)
  self.assertIn("elapsed + saved_age > GX_S3_CLEAN_REFERENCE_MAX_US",S)
  self.assertIn("gx_s3_discard_clean_bg (self)",S)
  self.assertIn("GX_S3_CLEAN_REFERENCE_MAX_US (G_GINT64_CONSTANT(12)",S)
 def test_biometric_security_and_never_overwrite_enrollment(self):
  self.assertIn("GX_MATCH_THRESHOLD            7",S)
  self.assertIn("decision=final-for-pose",S)
  self.assertIn("gx_send_capture_cleanup (self)",S)
  self.assertIn("self->s3_saved_bg",S)
  self.assertNotIn("GXFP_AUTO_REENROLL",S)
  self.assertNotIn("GXFP_FAST_RETRY_QA",S)
  self.assertNotIn("GXFP_VERIFY_SCORE_CACHE_QA",S)
 def test_partial_contact_quality_examples(self):
  def eligible(first,second,mean1,mean2,ref=357,abs_level=330,qa=True,pending=True):
   def count(mask): return (mask & 0x3f).bit_count()
   if not qa or not pending: return False
   if not (4 <= count(first) < 6): return False
   if mean1 >= 310 or mean1 >= abs_level or ref-mean1 < 50: return False
   if count(second)<4: return False
   if mean2 >= 310 or mean2 >= abs_level or ref-mean2 < 50: return False
   return True
  self.assertTrue(eligible(0x3c,0x3f,285,220))
  self.assertTrue(eligible(0x3c,0x3c,285,285))
  self.assertFalse(eligible(0x38,0x3f,285,220))
  self.assertFalse(eligible(0x3c,0x00,285,357))
  self.assertFalse(eligible(0x3c,0x3f,335,220))
  self.assertFalse(eligible(0x3c,0x3f,285,220,qa=False))
  self.assertFalse(eligible(0x3c,0x3f,285,220,pending=False))
if __name__=="__main__":
 unittest.main(verbosity=2)
