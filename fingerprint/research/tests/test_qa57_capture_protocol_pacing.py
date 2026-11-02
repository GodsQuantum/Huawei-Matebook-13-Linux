#!/usr/bin/env python3
"""QA57: compile and exercise the production C helper in isolation."""
from pathlib import Path
import subprocess, tempfile, unittest
root=Path(__file__).resolve().parents[2]
src=(root/"driver/goodix51a0/goodix51a0.c").read_text()
a=src.index("static int\ngx_capture_protocol_floor (")
b=src.index("\nstatic void\ngx_capture_pacing_success (",a)
helper=src[a:b]
assert 'GXFP_DECOUPLE_ACK_PACING_QA' in helper
code=r"""
#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <string.h>
typedef struct {int timing_scale;} FpiDeviceGoodix51A0;
#define GX_CAPTURE_SCALE_MIN 100
#define GX_CAPTURE_SCALE_STEP 50
#define CLAMP(x,l,h) ((x)<(l)?(l):((x)>(h)?(h):(x)))
static const char *g_getenv(const char *v){return getenv(v);}
static int g_strcmp0(const char *a,const char *b){
 if (!a) return b ? -1 : 0;
 if (!b) return 1;
 return strcmp(a,b);
}
""" + helper + r"""
int main(int argc,char **argv) {
 if(argc!=4)return 9;
 FpiDeviceGoodix51A0 dev={.timing_scale=atoi(argv[1])};
 if(atoi(argv[2]))setenv("GXFP_DECOUPLE_ACK_PACING_QA","1",1);
 else unsetenv("GXFP_DECOUPLE_ACK_PACING_QA");
 return gx_capture_protocol_floor(&dev)==atoi(argv[3])?0:1;
}
"""
class QA57(unittest.TestCase):
 @classmethod
 def setUpClass(cls):
  cls.temp=tempfile.TemporaryDirectory(prefix="gxfp57-")
  path=Path(cls.temp.name)
  (path/"t.c").write_text(code)
  cls.exe=path/"test"
  subprocess.run(["cc","-std=c11","-Wall","-Wextra","-Werror","-pedantic",
                  str(path/"t.c"),"-o",str(cls.exe)],check=True,capture_output=True)
 @classmethod
 def tearDownClass(cls):cls.temp.cleanup()
 def checks(self,enabled,vals):
  for scale,expected in vals:
   with self.subTest(enabled=enabled,scale=scale):
    self.assertEqual(subprocess.run([str(self.exe),str(scale),str(int(enabled)),
                     str(expected)]).returncode,0)
 def test_nominal_default(self):self.checks(False,[(100,100),(150,100),(200,150),(250,200),(300,250)])
 def test_qa57_protocol_miss_does_not_raise_capture_floor(self):
  self.checks(True,[(100,100),(150,100),(200,100),(250,100),(300,100)])
 def test_qa54_default_and_image_retry_remain(self):
  self.assertEqual(src.count("gx_capture_protocol_floor (self)"),2)
  self.assertIn("self->capture_retry_seen",src)
  self.assertIn("MAX (previous + GX_CAPTURE_SCALE_STEP, protocol_floor)",src)
  self.assertIn("GX_CAPTURE_CLEAN_DECAY_STREAK",src)
  self.assertIn("gx_capture_transport_desync (FpiDeviceGoodix51A0 *self)",src)
 def test_biometric_gates_and_fdt_are_unchanged(self):
  self.assertIn("FDT_DOWN_PROOF incomplete before GET_IMAGE",src)
  self.assertIn("GX_MATCH_THRESHOLD",src)
  self.assertIn("gx_tls_decrypt_record",src)
  self.assertIn("GXFP_ROTATION_RESCUE_QA",src)
if __name__=="__main__":unittest.main(verbosity=2)
