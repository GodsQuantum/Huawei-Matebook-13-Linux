#!/usr/bin/env python3
"""The rel71.57 opt-in timings cannot perturb the target capture/cleanup recipe."""
from pathlib import Path
import unittest
import re
ROOT=Path(__file__).resolve().parents[2]
C=(ROOT/"driver/goodix51a0/goodix51a0.c").read_text()
A=C.index("static gboolean\ngx_send_capture_cleanup")
B=C.index("/* Captures one full image",A)
F=C[A:B]

class CleanupLatencySafety(unittest.TestCase):
 def test_qa_is_off_by_default(self):
  self.assertIn('g_strcmp0 (g_getenv ("GXFP_TIMING_DIAGNOSTICS"), "1") == 0',F)
  self.assertIn('if (profile)',F)
  self.assertIn('LATENCY_TRACE cleanup step=',F)
  self.assertIn('LATENCY_TRACE cleanup end',F)
 def test_protocol_untouched(self):
  self.assertIn("gxfp_build_capture_cleanup_recipe",F)
  self.assertIn("gx_send_plain_drain",F)
  self.assertIn("gx_take_tls_frame",F)
  self.assertIn("gx_tls_decrypt_record",F)
  self.assertIn("g_usleep (gap_us);",F)
  self.assertIn("guint gap_us = gx_capture_gap_us (self);",F)
  self.assertLess(F.index("gx_send_plain_drain"),F.index("gx_take_tls_frame"))
  self.assertLess(F.index("gx_tls_decrypt_record"),F.index("g_usleep (gap_us);"))
 def test_security_unchanged(self):
  self.assertRegex(C,r"(?m)^#define GX_MATCH_THRESHOLD\s+7\s*$")
  self.assertIn("decision=final-for-pose",C)
  self.assertIn("verified positive terminal ",C)
if __name__=="__main__":
 unittest.main(verbosity=2)
