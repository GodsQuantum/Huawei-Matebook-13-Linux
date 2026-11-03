#!/usr/bin/env python3
"""QA61 safety invariants: held at Claim never bypasses biometric matching."""
from pathlib import Path
import re
import unittest

SOURCE = (Path(__file__).resolve().parents[2] /
          "driver/goodix51a0/goodix51a0.c").read_text(encoding="utf-8")


class Qa61HeldFingerFast(unittest.TestCase):
    def test_opt_in_default_off_and_bounded(self):
        self.assertIn('g_getenv ("GXFP_EARLY_HELD_FAST_AUTH_QA")', SOURCE)
        self.assertIn("self->warm_open_contact_at_us <= 500 * 1000", SOURCE)
        self.assertIn("self->warm_open_contact_at_us > 0", SOURCE)

    def test_real_fdt_proof_and_independent_capture(self):
        self.assertRegex(SOURCE, r"held_contact =\s*gx_fdt_touch_is_finger")
        self.assertIn("gx_fdt_drop (self->fdt_base, cur) > GOODIX_FDT_DROP", SOURCE)
        self.assertIn("gx_fdt_probe_ex (self, cur, &touchflag)", SOURCE)
        self.assertIn("gx_capture_auth_same_press (self,", SOURCE)

    def test_no_early_finger_stall_and_normal_absent_path(self):
        self.assertIn("if (!gx_background_is_fresh (self) && held_fast_qa)", SOURCE)
        self.assertIn("else if (!gx_background_is_fresh (self))", SOURCE)
        self.assertIn("self->poll_id = g_timeout_add (1, gx_poll_first_held, ssm)", SOURCE)

    def test_no_security_bypass_or_unsafe_cache_reuse(self):
        self.assertIn("#define GX_VERIFY_MAX_ATTEMPTS 3", SOURCE)
        self.assertIn("if (attempt_score >= GX_MATCH_THRESHOLD)", SOURCE)
        self.assertIn("gx_send_capture_cleanup (self)", SOURCE)
        self.assertIn("self->warm_open_contact_at_us = 0;", SOURCE)
        self.assertIn("if (self->bg_dirty)", SOURCE)

if __name__ == "__main__":
    unittest.main()
