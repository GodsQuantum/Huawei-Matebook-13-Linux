#!/usr/bin/env python3
"""Safe one-score-per-physical-press Verify cache: exact reference stays 7."""
from pathlib import Path
import unittest

S=(Path(__file__).resolve().parents[2]/"driver/goodix51a0/goodix51a0.c").read_text()

class OneScore(unittest.TestCase):
 def test_proven_only_with_env_and_exact_same_image(self):
  self.assertIn('g_strcmp0 (g_getenv ("GXFP_VERIFY_SCORE_CACHE_QA"), "1") == 0', S)
  self.assertIn("cached_verify_score",S)
  self.assertIn("cached_verify_views",S)
  self.assertIn("w->cached_verify_score < 0 || w->verify_print != tmpl",S)
  self.assertIn("cached verification score/template mismatch; fail closed",S)
  self.assertIn("w->feat = gx_capture_auth_same_press",S)
  self.assertIn("&w->cached_verify_score",S)
  self.assertIn("attempt_score = w->cached_verify_score;",S)
 def test_declaration_and_definition_agree(self):
  proto=S[S.index("static GxSiftFeatures *\ngx_capture_auth_same_press"):S.index("static void\ngx_work_free")]
  self.assertIn("int                  *out_score",proto)
  self.assertIn("guint                *out_views",proto)
  self.assertIn("guint                *out_adapt_views",proto)
 def test_existing_negative_budget_and_identity_unchanged(self):
  self.assertIn("score=%d threshold=%d candidate=%u decision=final-for-pose",S)
  self.assertIn("GX_MATCH_THRESHOLD            7",S)
  self.assertIn("t->tries >= GX_VERIFY_MAX_ATTEMPTS",S)
  self.assertIn("t->rearm_mcu_before_retry = TRUE;",S)
  self.assertIn("gx_score_probe_against_gallery (gallery, probe, &candidate)",S)
  self.assertIn("gx_score_probe_against_print (candidate, t->probe",S)
 def test_mandatory_cleanup_precedes_cached_score_delivery(self):
  start=S.index("gx_capture_auth_same_press (")
  end=S.index("static gboolean\ngx_enroll_view_connected",start)
  part=S[start:end]
  self.assertLess(part.index("gx_send_capture_cleanup (self)"),
                  part.index("*out_score = best_probe ? best_score : -1"))
  self.assertIn("g_clear_pointer (&best_probe, gx_sift_free)",part)
  self.assertIn("w->cached_verify_adapt_views",S)
  self.assertIn("self->capture_recovery_pending",part)
 def test_no_automatic_enrollment_or_new_decision(self):
  self.assertNotIn("GXFP_AUTO_REENROLL",S)
  self.assertIn("GX_TEMPLATE_VERSION 4u",S)
  self.assertIn("GX_PIXEL_RESCUE_MIN_AGREE_PERMILLE 845",S)
if __name__=="__main__": unittest.main(verbosity=2)
