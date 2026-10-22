#!/usr/bin/env python3
"""Static and control-flow invariant test for successful verify/identify fast completion.
No hardware access, biometrics, keys, or TLS material.
"""
from pathlib import Path
import re
from unittest import TestCase, main

C = Path(__file__).resolve().parents[2] / "driver/goodix51a0/goodix51a0.c"
SOURCE = C.read_text(encoding="utf8")
CAP = SOURCE[SOURCE.index("static void\ngx_capture_done ("):SOURCE.index("static void\ngx_run_state (")]
AUTH = SOURCE[SOURCE.index("static GxSiftFeatures *\ngx_capture_auth_same_press ("):SOURCE.index("static gboolean\ngx_enroll_view_connected (")]
POLL = SOURCE[SOURCE.index("static gboolean\ngx_poll_off ("):SOURCE.index("static gboolean\ngx_pixel_rescue_view_pass (")]

class FastTerminalSafety(TestCase):
    def test_cleanup_occurs_before_score_delivered(self):
        self.assertLess(AUTH.index("gx_send_capture_cleanup (self)"),
                        AUTH.index("return best_probe;"))
        self.assertIn("g_steal_pointer (&w->feat)", CAP)
        self.assertIn("GX_MATCH_THRESHOLD", CAP)

    def test_positive_match_only_terminal(self):
        gate = "if (t->verifying && t->match_reported &&\n          t->best >= GX_MATCH_THRESHOLD)"
        self.assertIn(gate, CAP)
        self.assertLess(CAP.index("fpi_device_verify_report (dev, FPI_MATCH_SUCCESS"),
                        CAP.index(gate))
        self.assertLess(CAP.index("fpi_device_identify_report (dev,"),
                        CAP.index(gate))

    def test_terminal_before_release_wait(self):
        gate = "if (t->verifying && t->match_reported &&"
        start = CAP.index(gate)
        done = CAP.index("fpi_ssm_jump_to_state (ssm, GX_ST_DONE);", start)
        returned = CAP.index("return;", done)
        wait = CAP.index("g_timeout_add (GX_POLL_MS, gx_poll_off, ssm);", start)
        self.assertLess(done, wait)
        self.assertLess(returned, wait)
        self.assertIn("FP_FINGER_STATUS_NONE", CAP[start:returned])

    def test_negative_and_recovery_unchanged(self):
        self.assertIn("t->verifying && !t->match_reported &&", POLL)
        self.assertIn("capture transport failed; recovering before", CAP)
        self.assertIn("t->tries >= GX_VERIFY_MAX_ATTEMPTS", CAP)
        self.assertRegex(SOURCE, r"(?m)^#define\s+GX_MATCH_THRESHOLD\s+7\s*$")
        self.assertRegex(SOURCE, r"(?m)^#define\s+GX_VERIFY_MAX_ATTEMPTS\s+3\b")

if __name__=="__main__":
    main(verbosity=2)
