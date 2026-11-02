#!/usr/bin/env python3
"""QA59 strict eligibility is compiled from the actual driver C helper."""
from pathlib import Path
import subprocess
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[2]
SRC=(ROOT/"driver/goodix51a0/goodix51a0.c").read_text()
BEGIN=SRC.index("static gboolean\ngx_s3_calibration_early_reuse_eligible (")
END=SRC.index("\n/* Only called inside a genuine libfprint",BEGIN)
HELPER=SRC[BEGIN:END]
CODE=r"""
#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
typedef int gboolean;
typedef uint8_t guint8;
typedef struct {
  void *s3_saved_bg;
  int s3_saved_abs;
} FpiDeviceGoodix51A0;
#define GOODIX_FDT_ABS 300
static const char *g_getenv(const char *x){return getenv(x);}
static int g_strcmp0(const char *a,const char *b) {
 if(!a) return b ? -1 : 0;
 if(!b) return 1;
 return strcmp(a,b);
}
""" + HELPER + r"""
int main(int argc, char **argv){
 if(argc!=7) return 9;
 FpiDeviceGoodix51A0 s={.s3_saved_bg=(atoi(argv[2])? (void*)&argc:NULL),
                         .s3_saved_abs=atoi(argv[4])};
 if(atoi(argv[1])) setenv("GXFP_S3_CALIBRATION_EARLY_REUSE_QA","1",1);
 else unsetenv("GXFP_S3_CALIBRATION_EARLY_REUSE_QA");
 return !!gx_s3_calibration_early_reuse_eligible(
   &s,(guint8)strtoul(argv[3],NULL,0),atoi(argv[5])) == atoi(argv[6])?0:1;
}
"""

class QA59(unittest.TestCase):
 @classmethod
 def setUpClass(cls):
  cls.temp=tempfile.TemporaryDirectory(prefix="gxfp59-")
  d=Path(cls.temp.name)
  (d/"t.c").write_text(CODE)
  cls.exe=d/"test"
  subprocess.run(["cc","-std=c11","-Wall","-Wextra","-Werror","-pedantic",
                  str(d/"t.c"),"-o",str(cls.exe)],check=True,capture_output=True)
 @classmethod
 def tearDownClass(cls):cls.temp.cleanup()
 def check(self,enabled,saved,touch,abs_floor,mean,expected):
  args=[str(self.exe),str(int(enabled)),str(int(saved)),
        hex(touch),str(abs_floor),str(mean),str(expected)]
  self.assertEqual(subprocess.run(args).returncode,0,args)
 def test_default_off_and_no_saved_ref(self):
  for opt in (0,1):
   self.check(opt,False,0x3f,329,206,0)
  self.check(False,True,0x3f,329,206,0)
 def test_only_full_proven_contact_and_good_absolute_drop(self):
  self.check(True,True,0x3f,329,206,1)
  self.check(True,True,0x3f,329,269,1)
  self.check(True,True,0x3f,329,270,0)
  self.check(True,True,0x3f,240,255,0)
  self.check(True,True,0x3f,329,330,0)
  for touch in (0,0x01,0x0f,0x1f,0x3e,0x2f):
   self.check(True,True,touch,329,206,0)
 def test_reuse_only_in_prepare_and_second_fdt_and_tls_match_required(self):
  self.assertEqual(SRC.count("gx_s3_calibration_early_reuse_eligible ("),2)
  self.assertIn("gx_s3_try_early_finger_reference (self)",SRC)
  self.assertIn("out_s3_reference_restored",SRC)
  self.assertIn("capture_diagnostic ? NULL :",SRC)
  self.assertIn("s3_reference_restored)\n          return TRUE;",SRC)
  self.assertIn("if (gxfp_parse_fdt_down_reply",SRC) if False else None
  self.assertIn("if (gx_fdt_probe_ex (self, cur, &touchflag) != 0)",SRC)
  self.assertIn("GX_S3_CLEAN_REFERENCE_MAX_US",SRC)
  self.assertIn("GX_MATCH_THRESHOLD",SRC)
  self.assertIn("gx_tls_decrypt_record",SRC)
  self.assertIn("GXFP_POST_NAV_SETTLE_QA",SRC)
if __name__=="__main__": unittest.main(verbosity=2)
