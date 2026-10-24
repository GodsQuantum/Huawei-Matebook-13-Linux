#!/usr/bin/env python3
from pathlib import Path
import re
import unittest
S=(Path(__file__).resolve().parents[2]/"driver/goodix51a0/goodix51a0.c").read_text()
class Prepark(unittest.TestCase):
 def test_opt_in_only_in_suspend(self):
  sub=S[S.index("static void\ngx_dev_suspend"):S.index("/* After true S3, the normal first Claim")]
  self.assertIn('g_strcmp0 (g_getenv ("GXFP_S3_PREPARK_FRESH_QA"), "1") == 0',sub)
  self.assertIn("!gx_background_is_fresh (self)",sub)
  self.assertIn("warm && pre_sleep_clean",sub)
 def test_update_requires_new_proven_clear_reference(self):
  a=S.index("/* rel71.60 OPT-IN")
  b=S.index("gxfp_build_sleep (&packet)",a)
  sub=S[a:b]
  self.assertIn("gx_warm_validate (self)",sub)
  self.assertIn("self->bg_last_refresh_us > prev_refresh",sub)
  self.assertIn("(refreshed_touch & 0x3fu) == 0",sub)
  self.assertIn("pre_sleep_clean = refreshed",sub)
 def test_protocol_no_weakening(self):
  self.assertRegex(S,r"(?m)^#define GX_MATCH_THRESHOLD\s+7\s*$")
  self.assertIn("gx_target_send_ack (self, &packet, 0x60, NULL)",S)
  self.assertIn("gx_s3_snapshot_clean_bg (self)",S)
  self.assertIn("gx_s3_discard_clean_bg (self)",S)
  self.assertNotIn("GXFP_S3_PREPARK_FRESH_QA",S[S.index("static void\ngx_dev_resume"):])
if __name__=="__main__":unittest.main(verbosity=2)
