#!/usr/bin/env python3
from pathlib import Path
import unittest
S=(Path(__file__).resolve().parents[2]/'driver/goodix51a0/goodix51a0.c').read_text()
a=S.index('gx_capture_auth_same_press (',S.index('gx_score_probe_against_gallery ('))
b=S.index('gx_score_probe_against_gallery (gallery, probe',a)
class CapturePerformance(unittest.TestCase):
 def test_profiler_only_with_opt_in(self):
  self.assertIn('GXFP_CAPTURE_PERF_QA',S[a:b])
  self.assertIn('perf_qa ? g_get_monotonic_time () : 0',S[a:b])
  self.assertIn('if (perf_qa)',S[a:])
  self.assertIn('CAPTURE_PERF_QA mode=',S[a:])
 def test_real_match_path_retained(self):
  for needle in ('gx_capture_frame_ex (self, px, GX_CAPTURE_FINGER, FALSE)',
   'gx_features_from_pixels (self, px)','GX_MIN_CAPTURE_KEYPOINTS','GX_MATCH_THRESHOLD'):
   self.assertIn(needle,S[a:])
 def test_no_biometric_material_or_secret_emitted(self):
  line=S[S.index('fp_warn ("GXFP51A0 CAPTURE_PERF_QA'):][:410]
  for bad in ('template=', 'px=', 'rec=', 'key=', 'psk='):self.assertNotIn(bad,line)
if __name__=='__main__':unittest.main()
