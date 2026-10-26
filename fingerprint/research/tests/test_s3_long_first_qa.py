#!/usr/bin/env python3
"""Opt-in long initial reset after true S3, not a matcher or transport bypass."""
import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SOURCE = (ROOT / "driver/goodix51a0/goodix51a0.c").read_text()
FN = SOURCE.split("static void\ngx_reset_detached_s3_boundary", 1)[1].split("static gboolean\ngx_target_read_body", 1)[0]

class NativeS3LongFirstQA(unittest.TestCase):
    def test_flag_is_strict_opt_in_and_not_global(self):
        self.assertIn('g_strcmp0 (g_getenv ("GXFP_S3_LONG_FIRST_QA"), "1") == 0', FN)
        self.assertEqual(SOURCE.count('GXFP_S3_LONG_FIRST_QA'), 1)

    def test_only_on_genuine_s3_and_detached_transport(self):
        self.assertIn("self->s3_first_tls_pending && !self->s3_rel61_tls_fallback", FN)
        self.assertIn("self->spi_fd < 0 && self->irq_fd < 0", FN)
        self.assertIn('NATIVE_S3_LONG_FIRST_QA', FN)

    def test_fallback_remains_reviewed_short_and_then_long(self):
        self.assertIn("gx51_reset_gpio264_short () == 0", FN)
        self.assertIn("gx_gpio_reset (self);", FN)
        self.assertRegex(FN, r"NATIVE_S3_LONG_FIRST_QA[\s\S]*?gx_gpio_reset \(self\);[\s\S]*?return;[\s\S]*?NATIVE_S3_SHORT_RESET")
        self.assertIn("gx_gpio_reset (self);\n}", FN)

    def test_no_tls_matcher_or_security_bypass(self):
        self.assertIn("if (gx_upload_config_and_reqtls (self, TRUE) &&", SOURCE)
        self.assertIn("gx_tls_handshake (self)", SOURCE)
        self.assertIn("GXFP51A0 NATIVE_S3_TLS_EARLY_RECOVER", SOURCE)
        self.assertIn("GXFP51A0 NATIVE_S3_LONG_RECOVERY", SOURCE)
        self.assertIn("threshold", SOURCE)
        self.assertIn("GXFP_S3_FRESH_BG_QA", SOURCE)
        self.assertNotIn("GXFP_CLEANUP_SETTLE_QA", SOURCE)

if __name__ == "__main__":
    unittest.main()
