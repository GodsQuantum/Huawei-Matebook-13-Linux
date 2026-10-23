#!/usr/bin/env python3
"""Native post-S3 prewarm must finish before libfprint resumes new operations.
Guards production candidate source only, not a real hardware power test.
"""
from pathlib import Path
import re
import unittest
ROOT=Path(__file__).resolve().parents[2]
C=(ROOT/"driver/goodix51a0/goodix51a0.c").read_text()
PATCH=(ROOT/"driver/goodix51a0/libfprint-v1.94.100.patch").read_text()
def fn(name):
 m=re.search(r'\b'+re.escape(name)+r'\s*\([^;{}]*\)\s*\n\{',C)
 assert m,name
 a=C.find('{',m.end()-1);depth=0
 for i in range(a,len(C)):
  if C[i]=='{':depth+=1
  elif C[i]=='}':
   depth-=1
   if depth==0:return C[m.start():i+1]
 raise AssertionError(name)
class ResumePrewarmSource(unittest.TestCase):
 def test_exact_target_idle_vfunc(self):
  self.assertIn('fp_device_get_driver (device), "goodix51a0"',PATCH)
  self.assertIn('FP_DEVICE_GET_CLASS (device)->resume (device)',PATCH)
  self.assertIn('FPI_DEVICE_ACTION_NONE:',PATCH)
 def test_resume_only_idles_preheat(self):
  s=fn("gx_dev_resume")
  self.assertIn("fpi_device_get_current_action (dev) == FPI_DEVICE_ACTION_NONE",s)
  self.assertIn("self->spi_fd < 0 && self->irq_fd < 0",s)
  self.assertIn("g_task_run_in_thread (task, gx_resume_preflight_thread)",s)
  self.assertIn("NATIVE_S3_RESUME_BEGIN",s)
 def test_resume_worker_safely_reinitializes(self):
  s=fn("gx_resume_preflight_thread")
  for phrase in ("gx_warm_abandon (self)","gx_transport_close (self)","gx_reset_detached_s3_boundary (self)","gx_transport_open (dev, &err)","gx_cold_prepare (self)"):
   self.assertIn(phrase,s)
  self.assertLess(s.index("gx_transport_close (self)"),s.index("gx_reset_detached_s3_boundary (self)"))
  self.assertLess(s.index("gx_reset_detached_s3_boundary (self)"),s.index("gx_transport_open (dev, &err)"))
  self.assertIn("gx_transport_close (self)",s[s.index("gx_cold_prepare (self)"):])
  self.assertNotIn("fpi_device_resume_complete",s)
 def test_resume_main_loop_completion(self):
  s=fn("gx_resume_preflight_done")
  self.assertIn("g_task_propagate_boolean",s)
  self.assertIn("fpi_device_resume_complete (dev, NULL)",s)
  self.assertIn("NATIVE_S3_RESUME_PREWARM_READY",s)
  self.assertIn("NATIVE_S3_RESUME_PREWARM_FALLBACK",s)
  self.assertIn("self->force_cold_reset = TRUE",s)
  self.assertIn("self->force_cold_reset = FALSE",s)
 def test_failed_prewarm_always_allows_normal_claim_recovery(self):
  s=fn("gx_resume_preflight_done")
  self.assertIn("gx_warm_abandon (self)",s)
  self.assertIn("fpi_device_resume_complete (dev, NULL)",s)
  c=fn("gx_dev_open")
  self.assertIn("if (!warm_candidate || self->capture_recovery_pending)",c)
  self.assertIn("gx_cold_prepare (self)",c)
 def test_security_controls_unchanged(self):
  self.assertRegex(C,r'(?m)^#define GX_MATCH_THRESHOLD\s+7\s*$')
  self.assertRegex(C,r'(?m)^#define GX_ENROLL_STAGES\s+20\s*$')
  self.assertIn("background contaminated by touch",C)
  self.assertNotIn("pam_permit.so",C)
if __name__=='__main__':
 unittest.main(verbosity=2)
