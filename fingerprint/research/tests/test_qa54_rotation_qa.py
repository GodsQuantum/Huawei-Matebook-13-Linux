#!/usr/bin/env python3
import pathlib, re, unittest
R=pathlib.Path(__file__).resolve().parents[2]
code=(R/"driver/goodix51a0/goodix51a0.c").read_text()
sift=(R/"driver/goodix51a0/fastbrief/sigfm.c").read_text()
class RotationQA(unittest.TestCase):
 def test_verify_only(self):
  self.assertIn("gx_score_probe_against_print (candidate, probe, FALSE, NULL, NULL)", code)
  self.assertRegex(code, r"gx_score_probe_against_print \(tmpl, probe, TRUE,")
  self.assertIn('if (rotation_verify &&', code)
  self.assertIn('g_strcmp0 (g_getenv ("GXFP_ROTATION_RESCUE_QA"), "1")', code)
 def test_fixed_gate_same_view(self):
  self.assertIn("#define GX_MATCH_THRESHOLD            7",code)
  self.assertIn('best >= 3 && best < GX_MATCH_THRESHOLD',code)
  self.assertIn('score < GX_MATCH_THRESHOLD',code)
  for name in ('GX_PIXEL_RESCUE_MIN_OVERLAP','GX_PIXEL_RESCUE_MIN_ZNCC_MILLI','GX_PIXEL_RESCUE_MIN_AGREE_PERMILLE'):
   self.assertIn(name, code)
  self.assertIn('rotated, view',code)
  self.assertIn('return GX_MATCH_THRESHOLD;',code)
 def test_bounded_and_no_raw_persistence(self):
  self.assertIn('fabs (degrees) > 20.0',sift)
  self.assertIn('memset (rotated, 0',sift)
  self.assertIn('const double angles[] = { -12.0, 12.0, -6.0, 6.0 }', code)
  self.assertNotIn('fwrite (rotated',sift)
if __name__=="__main__":unittest.main(verbosity=2)
