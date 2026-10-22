#!/usr/bin/env python3
"""Protect ST411 detached-reset ordering in both in-operation recovery paths.

This is a SOURCE safety regression, not proof of hardware S3 correctness.
"""
from pathlib import Path
import re
import unittest

DRIVER = (Path(__file__).resolve().parents[2] /
          "driver/goodix51a0/goodix51a0.c").read_text(encoding="utf-8")

def body(name):
    match = re.search(r"(?m)^" + re.escape(name) + r"\s*\([^;{}]*\)\s*\{", DRIVER)
    if not match:
        raise AssertionError(f"missing C function {name}")
    start = DRIVER.find("{", match.start())
    depth = 0
    for idx in range(start, len(DRIVER)):
        depth += (DRIVER[idx] == "{") - (DRIVER[idx] == "}")
        if depth == 0:
            return DRIVER[start:idx + 1]
    raise AssertionError(f"unterminated C function {name}")

class DetachedRecoveryTests(unittest.TestCase):
    def test_existing_recovery_detaches_spi_before_gpio_reset(self):
        code = body("gx_recover_capture_context")
        steps = ("gx_transport_close (self)", "gx_gpio_reset (self)",
                 "gx_transport_open (FP_DEVICE (self)", "gx_read_fw_version_stage2e (self")
        offsets = [code.find(x) for x in steps]
        self.assertTrue(all(x >= 0 for x in offsets), offsets)
        self.assertEqual(offsets, sorted(offsets), "reset must be detached and firmware checked")

    def test_active_s3_reuses_detached_recovery(self):
        code = body("gx_session_start")
        s3 = code.split("if (self->force_cold_reset)", 1)[1].split(
            "if (self->capture_recovery_pending)", 1)[0]
        self.assertIn("gx_warm_abandon (self)", s3)
        self.assertIn("gx_recover_capture_context (self)", s3)
        self.assertNotIn("gx_gpio_reset (self)", s3,
                         "GPIO reset is unsafe while SPI transport is still open")
        self.assertLess(s3.index("gx_recover_capture_context (self)"),
                        s3.index("gx_cold_prepare (self)"))
        self.assertLess(s3.index("gx_cold_prepare (self)"),
                        s3.index("self->force_cold_reset = FALSE"),
                        "don't clear cold-recovery requirement until preparation succeeds")

    def test_stale_imagebase_reuses_detached_recovery(self):
        code = body("gx_session_start")
        stale = code.split("if (!gx_warm_validate (self))", 1)[1].split(
            "else if (self->bg_last_refresh_us", 1)[0]
        self.assertIn("gx_warm_abandon (self)", stale)
        self.assertIn("gx_recover_capture_context (self)", stale)
        self.assertNotIn("gx_gpio_reset (self)", stale)

if __name__ == "__main__":
    unittest.main(verbosity=2)
