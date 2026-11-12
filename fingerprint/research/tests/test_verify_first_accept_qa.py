#!/usr/bin/env python3
"""QA52 source and acceptance-decision invariance: no biometric data."""
import random
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SRC = (ROOT / "driver/goodix51a0/goodix51a0.c").read_text()
THRESHOLD = 7


def full_accept(scores, rescue):
    return max(scores, default=0) >= THRESHOLD or rescue


def guarded_accept(scores, rescue, enable, identify=False, diagnostic=False):
    if enable and not identify and not diagnostic:
        for score in scores:
            if score >= THRESHOLD:
                return True, True
    return full_accept(scores, rescue), False


class FirstAcceptQA(unittest.TestCase):
    def test_source_call_sites_and_security_controls(self):
        body = SRC.split("gx_score_probe_against_print (FpPrint *tmpl,", 1)[1].split(
            "static int\ngx_score_probe_against_gallery", 1)[0]
        self.assertIn("first_accept_qa && score >= GX_MATCH_THRESHOLD", body)
        self.assertIn('!g_getenv ("GXFP_MATCH_DIAGNOSTICS")', body)
        self.assertIn("gx_pixel_rescue_view_pass", body)
        self.assertLess(body.index("first_accept_qa &&"), body.index(
            "gx_pixel_rescue_view_pass"))
        self.assertIn("GXFP_VERIFY_FIRST_ACCEPT_QA", SRC)
        self.assertIn("NULL, NULL, FALSE", SRC)  # Identify always full scan.
        self.assertIn("GX_MATCH_THRESHOLD", SRC)
        self.assertIn("gx_send_capture_cleanup", SRC)
        self.assertIn("gx_tls_teardown", SRC)

    def test_all_accept_decisions_equal(self):
        rng = random.Random(0x51a0)
        for _ in range(30000):
            scores = [rng.randrange(0, 15) for _ in range(rng.randrange(0, 48))]
            rescue = bool(rng.getrandbits(1))
            expected = full_accept(scores, rescue)
            for identify, diagnostic in ((False, False), (True, False),
                                         (False, True), (True, True)):
                got, early = guarded_accept(scores, rescue, True, identify, diagnostic)
                self.assertEqual(got, expected)
                self.assertFalse(early and (identify or diagnostic))
            got, early = guarded_accept(scores, rescue, False)
            self.assertEqual(got, expected)
            self.assertFalse(early)

    def test_no_bypass_on_rejected_or_nearmiss(self):
        for scores in ([3, 4, 6], [0], [], [6] * 64):
            success, early = guarded_accept(scores, False, True)
            self.assertFalse(success)
            self.assertFalse(early)
        self.assertEqual(guarded_accept([3, 4, 6], True, True), (True, False))

    def test_first_accepted_view_can_stop_only_verify(self):
        scores = [1, 2, 9, 14, 0, 2]
        self.assertEqual(guarded_accept(scores, False, True), (True, True))
        self.assertEqual(guarded_accept(scores, False, True, identify=True),
                         (True, False))
        self.assertEqual(guarded_accept(scores, False, True, diagnostic=True),
                         (True, False))


if __name__ == "__main__":
    unittest.main(verbosity=2)
