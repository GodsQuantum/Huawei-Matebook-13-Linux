"""QA62 shadow-image processing: security and no-persistence invariants."""
import re
import unittest
from pathlib import Path

C = (Path(__file__).resolve().parents[2] /
     "driver/goodix51a0/goodix51a0.c").read_text(encoding="utf-8")


class StaleShadowQA(unittest.TestCase):
    def test_default_off_and_stale_only(self):
        self.assertIn('g_getenv ("GXFP_STALE_LOCAL_SHADOW_QA")', C)
        self.assertIn("gx_background_age_us (self) > GX_BG_FRESH_MAX_US", C)

    def test_same_frame_and_no_additional_sensor_io(self):
        self.assertIn("gx_stale_shadow_features (self, px)", C)
        shadow = C.split("gx_stale_shadow_features (FpiDeviceGoodix51A0 *self,")[1]
        shadow = shadow.split("/* Captures one press")[0]
        self.assertNotIn("gx_capture_frame", shadow)
        self.assertNotIn("gx_fdt_probe_ex", shadow)
        self.assertNotIn("fp_warn", shadow)
        self.assertIn("gx_sift_extract (ridge, w, h)", shadow)

    def test_primary_score_only_is_accepted(self):
        self.assertIn("decision=primary_only", C)
        self.assertIn("if (attempt_score >= GX_MATCH_THRESHOLD)", C)
        self.assertIn("gx_send_capture_cleanup (self)", C)
        self.assertIn("#define GX_VERIFY_MAX_ATTEMPTS 3", C)

    def test_no_persistence(self):
        body = C.split("gx_stale_shadow_features (FpiDeviceGoodix51A0 *self,")[1].split("/* Captures one press")[0]
        for needle in ("fopen", "write (", "g_file_set_contents", "printf", "gx_dump_capture"):
            self.assertNotIn(needle, body)

    def test_alternative_is_diagnostic_only(self):
        p = C.split("GXFP_STALE_LOCAL_SHADOW_QA")[1].split("if (perf_qa)")[0]
        self.assertNotIn("FPI_MATCH_SUCCESS", p)
        self.assertNotIn("score = shadow_best", p)
        self.assertNotIn("best_score = shadow_best", p)
        self.assertIn('g_clear_pointer (&shadow, gx_sift_free)', p)


if __name__ == "__main__":
    unittest.main()
