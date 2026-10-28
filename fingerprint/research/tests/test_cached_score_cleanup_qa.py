#!/usr/bin/env python3
from pathlib import Path
import unittest
src=(Path(__file__).resolve().parents[2]/'driver/goodix51a0/goodix51a0.c').read_text()
callback=src[src.index('gx_capture_done ('):src.index('gx_enroll_view_connected (',src.index('gx_capture_done ('))]
worker=src[src.index('gx_capture_auth_same_press (',src.index('gx_score_probe_against_gallery (')):src.index('gx_enroll_view_connected (',src.index('gx_capture_auth_same_press (',src.index('gx_score_probe_against_gallery (')))]
class ScoreCacheQA(unittest.TestCase):
 def test_match_computed_once_for_1to1(self):
  self.assertIn('int attempt_score = w->worker_score;',callback)
  self.assertNotIn('gx_score_probe_against_print (tmpl, t->probe',callback)
  self.assertIn('w->worker_score < 0',callback)
 def test_identify_order_checked(self):
  self.assertIn('best = w->worker_score;',callback)
  self.assertIn('identify gallery reordered',callback)
  self.assertIn('identify gallery changed',callback)
 def test_cached_score_invalid_when_cleanup_fails(self):
  self.assertIn('best_probe ? best_score : -1',worker)
  self.assertIn('g_clear_pointer (&best_probe, gx_sift_free)',worker)
  self.assertIn('CAPTURE_PERF_QA cleanup_ms=',worker)
 def test_decision_keeps_original_security(self):
  self.assertIn('#define GX_MATCH_THRESHOLD            7',src)
  self.assertIn('if (attempt_score >= GX_MATCH_THRESHOLD)',callback)
  self.assertIn('if (best_print && best >= GX_MATCH_THRESHOLD)',callback)
if __name__=='__main__':
 unittest.main()
