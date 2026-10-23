#!/usr/bin/env python3
"""Synthetic log tests; no biometric samples or network/hardware access."""
from __future__ import annotations

import importlib.util
import unittest
from pathlib import Path

FILE = Path(__file__).resolve().parents[2] / "scripts/gxfp51a0-hardware-evidence.py"
spec = importlib.util.spec_from_file_location("fingerprint_qa", FILE)
m = importlib.util.module_from_spec(spec)
spec.loader.exec_module(m)


def fp(t, message):
    return f"2026-10-23T11:{t}+02:00 Pegasus fprintd[101]: GXFP51A0 {message}"


def kernel(t, message):
    return f"2026-10-23T11:{t}+02:00 Pegasus kernel: {message}"


class EvidenceTests(unittest.TestCase):
    def setUp(self):
        self.kernel = "\n".join([kernel("00:02", "PM: suspend entry (deep)"),
                                 kernel("08:02", "PM: suspend exit")])

    def test_resume_and_matching_do_not_claim_first_finger(self):
        trace = "\n".join([
            fp("00:01", "NATIVE_S3_CLEAN_REFERENCE_SAVED age_ms=45000 RAM-only"),
            fp("00:01", "NATIVE_S3_PARK idle=1 attempted=1 ack=1 warm=1"),
            fp("08:03", "NATIVE_S3_RESUME_BEGIN idle=1; native prewarm candidate"),
            fp("08:07", "NATIVE_S3_EARLY_FINGER_VERIFIED_REFERENCE"),
            fp("08:09", "VERIFY_TRACE physical press 1/3 READY"),
            fp("08:10", "AUTH_TRACE mode=verify same-press image 1/3 score=9 threshold=7"),
            fp("08:11", "AUTH_TRACE verified positive terminal without release wait (score=9 threshold=7)"),
        ])
        result = m.report(trace, self.kernel)
        self.assertEqual(result["hardware_certification"], "PENDING")
        self.assertEqual(result["records_examined"], 1)
        s = result["deep_s3_sessions"][0]
        self.assertEqual(s["deep_sleep_seconds"], 480)
        self.assertTrue(s["sleep_packet_ack"])
        self.assertTrue(s["clean_reference_saved"])
        self.assertTrue(s["cached_reference_used"])
        self.assertTrue(s["native_resume_called"])
        self.assertEqual(s["successful_match_seconds_after_wake"], 9)
        self.assertEqual(s["matched_score"], 9)
        self.assertEqual(s["first_physical_placement"], "unverified_requires_human_report")
        self.assertFalse(s["certified"])

    def test_rejections_and_assertions_are_counted_not_misrepresented_as_matches(self):
        trace = "\n".join([
            fp("00:01", "NATIVE_S3_PARK idle=1 attempted=1 ack=1 warm=1"),
            fp("08:05", "CALIBRATION_LIFT_REQUIRED touch=0x3f"),
            fp("08:06", "CALIBRATION_LIFT_REQUIRED touch=0x3f"),
            fp("08:07", "fpi_device_get_cancellable: assertion 'current_action != NONE' failed"),
            fp("08:08", "NATIVE_S3_RESUME_PREWARM_FALLBACK"),
        ])
        s = m.report(trace, self.kernel)["deep_s3_sessions"][0]
        self.assertEqual(s["calibration_lift_required_count"], 2)
        self.assertEqual(s["idle_cancellable_assertions"], 1)
        self.assertTrue(s["prewarm_fallback"])
        self.assertIsNone(s["successful_match_seconds_after_wake"])

    def test_missing_wake_cannot_be_counted(self):
        result = m.report("", kernel("00:02", "PM: suspend entry (deep)"))
        self.assertEqual(result["records_examined"], 0)

    def test_two_cycles_do_not_mix_scores(self):
        extra = "\n".join([kernel("20:02", "PM: suspend entry (deep)"),
                           kernel("20:10", "PM: suspend exit")])
        f = "\n".join([fp("08:09", "AUTH_TRACE verified positive terminal without release wait (score=7 threshold=7)"),
                       fp("20:15", "NATIVE_S3_RESUME_PREWARM_READY")])
        sessions = m.report(f, self.kernel + "\n" + extra)["deep_s3_sessions"]
        self.assertEqual(len(sessions), 2)
        self.assertEqual(sessions[0]["matched_score"], 7)
        self.assertIsNone(sessions[1]["matched_score"])
        self.assertEqual(sessions[1]["prewarm_ready_seconds_after_wake"], 5)

    def test_old_match_hours_later_is_not_s3_success(self):
        fake_fprintd = "\n".join([
            fp("00:01", "NATIVE_S3_PARK idle=1 attempted=1 ack=1 warm=1"),
            "2026-10-23T16:09:10+02:00 Pegasus fprintd[101]: GXFP51A0 AUTH_TRACE verified positive terminal without release wait (score=12 threshold=7)",
        ])
        s3 = m.report(fake_fprintd, self.kernel)["deep_s3_sessions"][0]
        self.assertIsNone(s3["successful_match_seconds_after_wake"])
        self.assertIsNone(s3["matched_score"])

    def test_non_iso_or_naive_time_is_ignored(self):
        malformed = "Oct 23 11:08:09 user kernel: PM: suspend exit"
        self.assertEqual(m.parse_events(malformed, "kernel"), [])
        naive = "2026-10-23T11:08:09 Pegasus kernel: PM: suspend exit"
        self.assertEqual(m.parse_events(naive, "kernel"), [])


if __name__ == "__main__":
    unittest.main(verbosity=2)
