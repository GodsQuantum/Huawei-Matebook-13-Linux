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

    def test_qa54_new_record_qa42_preserved_and_qa50_rejected(self):
        by_id = {r["trial_id"]: r for r in self.rows}
        old = by_id["lock-20261030-qa42-0144"]
        latest = by_id["lock-20261101-qa54-1212"]
        s3 = by_id["s3-20261101-qa54-1213"]
        self.assertEqual((old["version"], old["press_to_terminal_ms"],
                          old["first_touch"]), ("rel71.74-qa42", "1181", "pass"))
        self.assertEqual((latest["first_touch"], latest["poses"],
                          latest["press_to_terminal_ms"],
                          latest["score"]), ("pass", "1", "1158", "12"))
        self.assertEqual((s3["first_touch"], s3["press_to_terminal_ms"],
                          s3["wake_to_terminal_ms"], s3["prewarm_ms"]),
                         ("unknown", "1711", "7384", "3787"))
        locks = [r for r in self.rows if r["kind"] == "LOCK"
                 and r["first_touch"] == "pass"
                 and r["press_to_terminal_ms"]]
        champion = min(locks, key=lambda r: int(r["press_to_terminal_ms"]))
        self.assertEqual(champion["trial_id"], latest["trial_id"])
        qa49 = by_id["lock-20261101-qa49-held-poll-0310"]
        qa50 = by_id["lock-20261101-qa50-failed-0357"]
        self.assertEqual((qa49["first_touch"], qa49["press_to_terminal_ms"]),
                         ("pass", "1328"))
        self.assertEqual((qa50["first_touch"], qa50["press_to_terminal_ms"]),
                         ("fail", ""))
        page = hof.render(self.rows)
        self.assertIn("QA42", page)
        self.assertIn("1/5 LOCK", page)
        self.assertIn("rel71.74-qa54+kde47", page)

    def test_qa56_first_lock_and_second_touch_s3_preserve_record(self):
        ids = {r["trial_id"]: r for r in self.rows}
        lock = ids["lock-20261101-qa56-1633"]
        s3 = ids["s3-20261101-qa56-1634"]
        self.assertEqual(
            (lock["kind"], lock["first_touch"], lock["poses"],
             lock["press_to_terminal_ms"], lock["fast_ready_ms"], lock["score"]),
            ("LOCK", "pass", "1", "1193", "51", "7"))
        self.assertEqual(
            (s3["kind"], s3["first_touch"], s3["poses"],
             s3["prewarm_ms"], s3["fast_ready_ms"],
             s3["press_to_terminal_ms"], s3["wake_to_terminal_ms"], s3["score"]),
            ("S3", "fail", "2", "3542", "84", "1679", "11780", "7"))
        # A second-touch S3 success is NOT a first-touch speed record.
        self.assertGreater(int(lock["press_to_terminal_ms"]),
                           int(ids["lock-20261101-qa54-1212"]["press_to_terminal_ms"]))
        self.assertIn("s3-20261101-qa56-1634", hof.render(self.rows))

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

    def test_rel7162_s3_fail_is_distinct_from_next_lock(self):
        a = [r for r in self.rows if r["trial_id"] == "s3-20261025-7162-natural-2106"]
        b = [r for r in self.rows if r["trial_id"] == "lock-20261025-7162-after-s3-2107"]
        self.assertEqual(len(a), 1)
        self.assertEqual(len(b), 1)
        self.assertEqual((a[0]["kind"], a[0]["first_touch"], a[0]["poses"]),
                         ("S3", "fail", "multiple"))
        self.assertEqual((a[0]["prewarm_ms"], a[0]["score"]), ("37625", ""))
        self.assertEqual((b[0]["kind"], b[0]["first_touch"], b[0]["poses"]),
                         ("LOCK", "pass", "1"))
        self.assertEqual((b[0]["press_to_terminal_ms"],
                          b[0]["greeter_to_terminal_ms"], b[0]["score"]),
                         ("1956", "4843", "9"))

    def test_rel7164_unknown_physical_pose_remains_unknown(self):
        ids = {r["trial_id"]:r for r in self.rows}
        s3 = ids["s3-20261026-7164-1120"]
        boot = ids["boot-20261026-7164-0844"]
        lock = ids["lock-20261026-7164-0846"]
        self.assertEqual((s3["kind"],s3["first_touch"],s3["poses"]),("S3","unknown","unknown"))
        self.assertEqual((s3["score"],s3["press_to_terminal_ms"],s3["wake_to_terminal_ms"]),("20","2995","9541"))
        self.assertEqual((boot["kind"],boot["first_touch"],boot["poses"]),("BOOT","pass","1"))
        self.assertEqual((lock["kind"],lock["first_touch"],lock["poses"],lock["score"]),("LOCK","pass","1","17"))

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
