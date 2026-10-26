#!/usr/bin/env python3
import unittest
from pathlib import Path
S=(Path(__file__).resolve().parents[2]/"driver/goodix51a0/goodix51a0.c").read_text()
a=S.index("gx_send_capture_cleanup (FpiDeviceGoodix51A0 *self)")
b=S.index("gx_capture_frame_ex (",a)
C=S[a:b]
class CleanupSettleSafety(unittest.TestCase):
 def test_flag_off_default_and_bounded_gap(self):
  self.assertIn('g_strcmp0 (g_getenv ("GXFP_CLEANUP_SETTLE_QA"), "1") == 0',C)
  self.assertIn("g_usleep (gx_capture_gap_us (self));",C)
  self.assertIn("self->tls_up && !self->capture_recovery_pending",C)
  self.assertIn("#define GX_SEQ_GAP_US 30000",S)
  self.assertIn("#define GX_CAPTURE_SCALE_MAX 300",S)
 def test_extra_wait_is_only_for_cleanup_image(self):
  self.assertIn("packet->inner[0] == 0x20u",C)
  self.assertLess(C.index("GXFP_CLEANUP_SETTLE_QA"),C.index("gx_send_plain_drain (self"))
 def test_tls_and_cleanup_never_omitted(self):
  self.assertIn("gxfp_build_capture_cleanup_recipe (&recipe)",C)
  self.assertIn("gx_send_plain_drain (self, packet->inner",C)
  self.assertIn("gx_take_tls_frame (self, rec",C)
  self.assertIn("gx_tls_decrypt_record (self->tls",C)
  self.assertIn("gx_capture_transport_desync (self)",C)
 def test_matching_threshold_and_one_scored_frame(self):
  self.assertIn("GX_MATCH_THRESHOLD            7",S)
  self.assertIn("decision=final-for-pose",S)
  self.assertIn("first usable image is the only scored draw",S)
 def test_no_unrelated_experiments_reintroduced(self):
  self.assertNotIn("GXFP_FAST_RETRY_QA",S)
  self.assertNotIn("GXFP_VERIFY_SCORE_CACHE_QA",S)
if __name__=="__main__":
 unittest.main(verbosity=2)
