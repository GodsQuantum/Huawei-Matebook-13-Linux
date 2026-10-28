#!/usr/bin/env python3
"""Compile and RUN the real post-auth capture-cleanup decision (not keyword checks)."""
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[2]
SOURCE=(ROOT/"driver/goodix51a0/goodix51a0.c").read_text()
a=SOURCE.index("      if (!cleanup_ok)\n        {",SOURCE.index("gx_capture_auth_same_press ("))
b=SOURCE.index("\n    }\n\n  if (out_images)",a)
BLOCK=SOURCE[a:b]
assert "self->force_cold_reset = TRUE;" in BLOCK
assert "g_clear_pointer (&best_probe, gx_sift_free)" in BLOCK

C = r"""
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define TRUE 1
#define GX_MATCH_THRESHOLD 7
typedef struct {int capture_recovery_pending,force_cold_reset,pacing;} Fake;
#define fp_warn(...) ((void)0)
#define g_clear_pointer(pp,freefn) (*(pp)=NULL)
#define gx_capture_pacing_success(self) ((self)->pacing++)
static const char *g_getenv(const char *name){return getenv(name);}
static int g_strcmp0(const char *a,const char *b)
{return a==NULL?-1:strcmp(a,b);}
static int run(int scenario)
{
  Fake d={0};Fake *self=&d;
  int score=12,cleanup_ok=0,optin=1,verify=1,identify=0,probe=1;
  int marker=0;
  switch(scenario){
  case 0: break; /* positive, failed cleanup and recovery is pending */
  case 1: score=6; break; /* negative / threshold fail */
  case 2: optin=0; break; /* QA feature disabled */
  case 3: probe=0; break; /* missing authenticated usable probe */
  case 4: verify=0; break; /* neither Verify nor Identify */
  case 5: d.capture_recovery_pending=0; break; /* no recorded desync */
  case 6: cleanup_ok=1; break; /* normal successful cleanup */
  case 7: verify=0;identify=1; break; /* Identify positive */
  case 8: score=7; break; /* boundary-positive score */
  default: return 99;
  }
  if (scenario!=5) d.capture_recovery_pending=1;
  if (optin) setenv("GXFP_POSTAUTH_CLEANUP_DEFER_QA","1",1);
  else unsetenv("GXFP_POSTAUTH_CLEANUP_DEFER_QA");
  void *tmpl=verify ? &marker : NULL;
  void *gallery=identify ? &marker : NULL;
  void *best_probe=probe ? &marker : NULL;
  int best_score=score;
""" + BLOCK + r"""
  return (best_probe ? 1 : 0)
       + (d.force_cold_reset ? 2 : 0)
       + (d.pacing ? 4 : 0);
}
int main(int argc,char **argv)
{
 if(argc!=2)return 98;
 return run(atoi(argv[1]));
}
"""

class CleanupBehavior(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tmp=tempfile.TemporaryDirectory(prefix="gxfp_auth_")
        cls.exe=Path(cls.tmp.name)/"cleanup_test"
        c=Path(cls.tmp.name)/"cleanup_test.c"
        c.write_text(C)
        subprocess.run(["cc","-std=c11","-O2","-Wall","-Wextra",
                        "-Werror","-pedantic",str(c),"-o",str(cls.exe)],
                       check=True,capture_output=True,text=True)
    @classmethod
    def tearDownClass(cls):cls.tmp.cleanup()
    def case(self,n):return subprocess.run([str(self.exe),str(n)]).returncode
    def test_positive_cleanup_failure_retains_match_and_retires_tls(self):
        self.assertEqual(self.case(0),3)
    def test_negative_cleanup_failure_drops_match(self):
        self.assertEqual(self.case(1),0)
    def test_feature_disabled_fails_closed(self):
        self.assertEqual(self.case(2),0)
    def test_missing_image_fails_closed(self):
        self.assertEqual(self.case(3),0)
    def test_missing_claimant_fails_closed(self):
        self.assertEqual(self.case(4),0)
    def test_no_recorded_recovery_fails_closed(self):
        self.assertEqual(self.case(5),0)
    def test_successful_cleanup_keeps_original_probe_and_pacing(self):
        self.assertEqual(self.case(6),5)
    def test_identify_positive_cleanup_failure_retires_tls(self):
        self.assertEqual(self.case(7),3)
    def test_threshold_inclusive(self):
        self.assertEqual(self.case(8),3)
    def test_cancelled_capture_is_dropped_before_callback_claim(self):
        # Complements the compiled worker policy with a safety-order check
        a=SOURCE.index("gx_capture_done (GObject *src")
        b=SOURCE.index('if (g_getenv ("GXFP_DIAGNOSTIC_CAPTURE_ONCE"))',a)
        self.assertIn("if (gx_cancelled (dev, ssm))",SOURCE[a:b])
        self.assertIn("g_clear_pointer (&f, gx_sift_free)",SOURCE[a:b])

if __name__=="__main__":unittest.main()
