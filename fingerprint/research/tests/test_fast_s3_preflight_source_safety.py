#!/usr/bin/env python3
"""QA fast S3 must be opt-in and preserve detached conservative fallback."""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[2]
C = (ROOT / "driver/goodix51a0/goodix51a0.c").read_text()

class FastResumeSafety(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        start = C.index("gx_resume_preflight_thread (")
        end = C.index("static void\ngx_resume_preflight_done", start)
        cls.worker = C[start:end]

    def test_no_default_unsafe_acceleration(self):
        s = self.worker
        self.assertIn('g_strcmp0 (g_getenv ("GXFP_S3_FAST_PREFLIGHT"), "1") == 0', s)
        self.assertIn('fast_prewarm ? 200 * 1000 : G_USEC_PER_SEC', s)
        self.assertEqual(s.count("g_usleep (fast_prewarm ? 200 * 1000 : G_USEC_PER_SEC);"), 2)

    def test_transport_closed_for_both_resets(self):
        s = self.worker
        self.assertLess(s.index("gx_transport_close (self)"), s.index("gx_reset_detached_s3_boundary (self)"))
        retry = s[s.index("if (fast_prewarm && !ok)"):]
        self.assertLess(retry.index("gx_transport_close (self)"), retry.index("gx_gpio_reset (self)"))
        self.assertLess(retry.index("gx_gpio_reset (self)"), retry.index("gx_transport_open (dev, &safe_err)"))

    def test_safe_failure_fallback(self):
        s = self.worker
        self.assertIn("if (fast_prewarm && !ok)", s)
        retry = s[s.index("if (fast_prewarm && !ok)"):]
        self.assertEqual(retry.count("g_usleep (G_USEC_PER_SEC);"), 2)
        self.assertIn("gx_warm_abandon (self)", retry)
        self.assertIn("gx_cold_prepare (self)", retry)
        self.assertIn("if (!ok)", retry)
        self.assertIn("g_task_return_boolean (task, ok)", retry)

    def test_security_and_s3_unchanged(self):
        self.assertRegex(C, r'(?m)^#define GX_MATCH_THRESHOLD\s+7\s*$')
        self.assertIn("fpi_device_resume_complete (dev, NULL)", C)
        self.assertIn("g_task_run_in_thread (task, gx_resume_preflight_thread)", C)
        self.assertIn("GX_SAME_PRESS_CAPTURE_ATTEMPTS 3", C)

if __name__ == "__main__":
    unittest.main(verbosity=2)
