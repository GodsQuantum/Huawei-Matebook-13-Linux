#!/usr/bin/env python3
from pathlib import Path
import re,unittest
src=(Path(__file__).resolve().parents[2]/'driver/goodix51a0/goodix51a0.c').read_text()
def function(n):
 begin=src.index(chr(10)+n+' (')
 a=src.index('{',begin);d=0
 for i in range(a,len(src)):
  if src[i]=='{':d+=1
  elif src[i]=='}':
   d-=1
   if d==0:return src[a:i+1]
 raise AssertionError(n)
class ResumeFastSettle(unittest.TestCase):
 def test_qa_fast_settle_is_default_off_and_bounded(self):
  f=function('gx_resume_preflight_thread')
  self.assertIn('GXFP_RESUME_FAST_SETTLE_QA',f)
  self.assertIn('fast_settle ? (250 * 1000) : G_USEC_PER_SEC',f)
  self.assertIn('fast_settle ? (500 * 1000) : G_USEC_PER_SEC',f)
  self.assertIn('gx_reset_detached_s3_boundary (self)',f)
 def test_optin_diagnostics_only_no_biometric_bytes(self):
  f=function('gx_resume_preflight_thread')
  self.assertIn('GXFP_RESUME_PERF_QA',f)
  self.assertIn('RESUME_PERF_QA fast_settle=',f)
  self.assertIn('cold_tls_background_ms=',f)
  self.assertNotIn('s3_saved_bg,',f)
 def test_native_security_and_fallback_preserved(self):
  f=function('gx_resume_preflight_thread')
  for marker in ('gx_warm_abandon (self)','gx_transport_close (self)',
                  'gx_transport_open (dev, &err)','gx_cold_prepare (self)',
                  'g_task_return_boolean (task, ok)'): self.assertIn(marker,f)
  self.assertIn('#define GX_MATCH_THRESHOLD            7',src)
if __name__=='__main__':unittest.main()
