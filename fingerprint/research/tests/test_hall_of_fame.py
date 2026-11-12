#!/usr/bin/env python3
"""Safety for Hall of Fame chronology and version-specific hardware QA."""
import importlib.util
import json
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
P = ROOT / "scripts/gxfp51a0-hall-of-fame.py"
spec = importlib.util.spec_from_file_location("gxfp_hof", P)
hof = importlib.util.module_from_spec(spec)
spec.loader.exec_module(hof)


class HallOfFameTests(unittest.TestCase):
    def setUp(self):
        self.rows = hof.load()

    def test_generated_page_is_current(self):
        self.assertEqual(hof.PAGE.read_text(encoding="utf-8"), hof.render(self.rows))

    def test_no_failed_s3_can_win(self):
        s3 = [r for r in self.rows if r["kind"] == "S3" and
              r["first_touch"] == "pass" and r["wake_to_terminal_ms"]]
        champion = min(s3, key=lambda r: int(r["wake_to_terminal_ms"]))
        self.assertEqual(champion["version"], "rel71.55")
        self.assertEqual(int(champion["wake_to_terminal_ms"]), 6951)
        self.assertFalse(any(r["version"] == "rel71.56" for r in s3))

    def test_version_gates_are_separated(self):
        v = [r for r in self.rows if r["version"] == "rel71.58"]
        self.assertEqual(len(v), 4)
        self.assertEqual([r["kind"] for r in v], ["LOCK", "S3", "S3", "S3"])
        self.assertEqual(v[0]["first_touch"], "pass")
        self.assertEqual(int(v[0]["press_to_terminal_ms"]), 1902)
        self.assertEqual(int(v[0]["fast_ready_ms"]), 293)
        self.assertEqual(v[0]["score"], "11")
        self.assertEqual(v[0]["threshold"], "7")
        self.assertEqual([r["first_touch"] for r in v[1:]], ["fail", "fail", "unknown"])
        self.assertFalse(any(r["first_touch"] == "pass" for r in v[1:]))

    def test_rel7160_first_touch_regression_is_blocking(self):
        v = [r for r in self.rows if r["version"] == "rel71.60"]
        self.assertEqual(len(v), 1)
        self.assertEqual(v[0]["kind"], "LOCK")
        self.assertEqual(v[0]["first_touch"], "fail")
        self.assertEqual(v[0]["poses"], "2")
        self.assertEqual(v[0]["threshold"], "7")
        self.assertEqual(int(v[0]["press_to_terminal_ms"]), 7885)
        self.assertEqual(int(v[0]["fast_ready_ms"]), 83)

    def test_rel7161_failed_first_touch_is_immutable(self):
        v = [r for r in self.rows if r["version"] == "rel71.61"]
        self.assertEqual(len(v), 1)
        trial = v[0]
        self.assertEqual(trial["kind"], "LOCK")
        self.assertEqual(trial["first_touch"], "fail")
        self.assertEqual(trial["poses"], "2")
        self.assertEqual(trial["score"], "7")
        self.assertEqual(trial["threshold"], "7")
        self.assertEqual(int(trial["press_to_terminal_ms"]), 7453)
        self.assertEqual(int(trial["fast_ready_ms"]), 82)

    def test_known_rejected_s3_was_not_concealed(self):
        s3 = [r for r in self.rows if r["kind"] == "S3" and
              r["version"] == "rel71.55"]
        self.assertEqual((sum(r["first_touch"] == "pass" for r in s3), len(s3)),
                         (2, 3))
        self.assertTrue(any(r["poses"] == "3-4" and
                            r["first_touch"] == "fail" for r in s3))

    def test_restored_legacy_provenance(self):
        root = hof.ROOT / "docs"
        data = json.loads((root / "LEGACY_HALL_OF_FAME_20261013.json").read_text())
        self.assertEqual(data["source_version_count"], 31)
        self.assertEqual(len(data["versions"]), 31)
        self.assertEqual(data["source_git_refs_count"], 125)
        self.assertEqual(data["source_leaderboard_count"], 14)
        self.assertEqual(data["source_sha256"],
                         "4545cf38f7d9a33c25d6d7226e5a4ba56bfaa75f6d3f2aef369cd96d7368ff5e")
        self.assertIn("rel71.46", (root / "LEGACY_HALL_OF_FAME_20261013.md").read_text())
        self.assertIn("LEGACY_HALL_OF_FAME_20261013.md", hof.PAGE.read_text())

    def test_ledger_has_no_biometric_payloads(self):
        raw = hof.TRIALS.read_text(encoding="utf-8").lower()
        for word in ("pmk=", "tls record=", "imagebase dump", "template bytes"):
            self.assertNotIn(word, raw)


if __name__ == "__main__":
    unittest.main(verbosity=2)
