#!/usr/bin/env python3
"""Long-deep-S3 cached reference is validated at park and bounded across sleep."""
from pathlib import Path
import re
import unittest
SRC=(Path(__file__).resolve().parents[2]/"driver/goodix51a0/goodix51a0.c").read_text()
def between(a,b):
    return SRC[SRC.index(a):SRC.index(b,SRC.index(a))]
class LongSleepReference(unittest.TestCase):
 def test_bounded_sleep_and_presleep_age(self):
    self.assertIn("GX_S3_CLEAN_REFERENCE_MAX_US (G_GINT64_CONSTANT(12) * 60 * 60 * G_USEC_PER_SEC)",SRC)
    self.assertIn("GX_S3_CLEAN_REFERENCE_PRE_AGE_MAX_US (G_GINT64_CONSTANT(2) * 60 * 60 * G_USEC_PER_SEC)",SRC)
    snap=between("gx_s3_snapshot_clean_bg (","gx_s3_try_early_finger_reference (")
    self.assertIn("GX_S3_CLEAN_REFERENCE_PRE_AGE_MAX_US",snap)
    awake=between("gx_s3_try_early_finger_reference (","gx_prepare_capture_context_once (")
    self.assertIn("elapsed + saved_age > GX_S3_CLEAN_REFERENCE_MAX_US",awake)
 def test_live_clear_proof_before_park(self):
    suspend=between("gx_dev_suspend (","gx_resume_preflight_thread (")
    self.assertIn("gx_fdt_probe_ex (self, park_fdt, &park_touchflag)",suspend)
    self.assertIn("(park_touchflag & 0x3fu) == 0",suspend)
    self.assertIn("park_mean >= GOODIX_FDT_ABS",suspend)
    self.assertLess(suspend.index("gx_fdt_probe_ex (self, park_fdt"),suspend.index("gx_target_send_ack (self, &packet, 0x60"))
    self.assertIn("pre_sleep_clean",suspend)
    self.assertIn("idle && parked && warm && pre_sleep_clean",suspend)
 def test_ref_not_persisted_and_no_security_shortcuts(self):
    self.assertIn("gx_s3_discard_clean_bg",SRC)
    self.assertIn("g_clear_pointer (&self->s3_saved_bg, g_free)",SRC)
    self.assertIn("#define GX_MATCH_THRESHOLD            7",SRC)
    self.assertIn("GOODIX_FINGER_STD    150.0",SRC)
    self.assertIn("background contaminated by touch",SRC)
 def test_12h_long_sleep_does_not_fail_clock_calculation(self):
    reuse=between("gx_s3_try_early_finger_reference (","gx_prepare_capture_context_once (")
    self.assertIn("MAX (G_GINT64_CONSTANT(1)",reuse)
if __name__=="__main__":
 unittest.main(verbosity=2)
