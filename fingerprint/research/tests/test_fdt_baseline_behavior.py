#!/usr/bin/env python3
"""Compile and RUN the actual driver FDT baseline C block with scripted probes."""
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
SOURCE = (ROOT / "driver/goodix51a0/goodix51a0.c").read_text()
START = "  /* Detection baseline. In one-shot diagnostic mode"
END = "  return self->have_fdt;"
assert SOURCE.count(START) == 1 and SOURCE.count(END) >= 1
a = SOURCE.index(START)
b = SOURCE.index(END, a) + len(END)
BLOCK = SOURCE[a:b]
assert "if (!nb)" in BLOCK and "gx_s3_snapshot_clean_bg (self)" in BLOCK

C = r"""
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define TRUE 1
#define FALSE 0
#define GOODIX_FDT_ABS 330
#define GOODIX_FDT_ABS_MARGIN 10
#define GX_DIAG_BASELINE_MAX_DRIFT 20
#define GX_DIAG_BASELINE_ATTEMPTS 4
#define GXFP_FDT_ZONE_COUNT 6
#define GX_CALIB_RETOUCH_DWELL_US 650000
typedef unsigned char guint8;
typedef struct {
  int have_fdt, fdt_base[6], fdt_abs, saved, discarded, cursor;
  int ok[4], touch[4], mean[4];
} Fake;
#define fp_warn(...) ((void)0)
#define fp_info(...) ((void)0)
static const char *g_getenv(const char *v) { return getenv(v); }
static int g_strcmp0(const char *a, const char *b)
{ return a == NULL ? -1 : strcmp(a,b); }
static int gx_fdt_mean(const int *t)
{ int sum=0; for(int i=0;i<6;i++) sum+=t[i]; return sum/6; }
static int gx_fdt_probe_ex(Fake *self,int *t,guint8 *touchflag)
{
 int i=self->cursor++ % 4;
 if (!self->ok[i]) return -1;
 *touchflag=(guint8)self->touch[i];
 for(int j=0;j<6;j++) t[j]=self->mean[i];
 return 0;
}
static int gx_wait_sensor_clear_stable(Fake *self,int m,void *p,long us)
{ (void)self; (void)m; (void)p; (void)us; return TRUE; }
static int gx_diag_press_countdown(Fake *self,int m)
{ (void)self; (void)m; return TRUE; }
static void gx_s3_snapshot_clean_bg(Fake *self){self->saved++;}
static void gx_s3_discard_clean_bg(Fake *self){self->discarded++;}
static int actual_baseline(Fake *self)
{
 int q,k,off_anchor_mean=-1,capture_diagnostic=0;
""" + BLOCK + r"""
}
static int run(int scenario)
{
 Fake d={0};
 for(int i=0;i<4;i++){d.ok[i]=1; d.touch[i]=0; d.mean[i]=355;}
 switch(scenario){
 case 0: break;  /* all four clean */
 case 1: d.ok[1]=d.ok[2]=d.ok[3]=0; break; /* 1 good, 3 no responses */
 case 2: d.touch[2]=d.touch[3]=0x3f; break; /* 2 clean, 2 touched */
 case 3: for(int i=0;i<4;i++) d.ok[i]=0; break; /* no responses */
 case 4: for(int i=0;i<4;i++) d.touch[i]=0x3f; break; /* 4 touched */
 case 5: for(int i=0;i<4;i++) d.mean[i]=220; break; /* too low */
 default: return 99;
 }
 setenv("GXFP_HELD_FINGER_ROLLING_BG_QA","1",1);
 int accepted=actual_baseline(&d);
 if (accepted!=d.have_fdt) return 80;
 if (d.saved!=(accepted ? 1 : 0)) return 81;
 return accepted ? 1 : 0;
}
int main(int argc,char **argv)
{
 if(argc!=2)return 98;
 return run(atoi(argv[1]));
}
"""

class RealFdtBaseline(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tmp = tempfile.TemporaryDirectory(prefix="gxfp_fdt_")
        cls.exe = Path(cls.tmp.name)/"fdt_test"
        src = Path(cls.tmp.name)/"fdt_test.c"
        src.write_text(C)
        subprocess.run(["cc","-std=c11","-O2","-Wall","-Wextra",
                        "-Werror","-pedantic",str(src),"-o",str(cls.exe)],
                       check=True, capture_output=True, text=True)
    @classmethod
    def tearDownClass(cls):
        cls.tmp.cleanup()
    def run_case(self, scenario):
        return subprocess.run([str(self.exe), str(scenario)], check=False).returncode
    def test_four_clean_accepted_and_saved(self):
        self.assertEqual(self.run_case(0),1)
    def test_one_clean_three_failed_currently_accepted(self):
        """Characterization: current driver stores a 1/4 snapshot; NOT an endorsement."""
        self.assertEqual(self.run_case(1),1)
    def test_two_clean_two_touched_currently_accepted(self):
        self.assertEqual(self.run_case(2),1)
    def test_all_failed_rejected(self):
        self.assertEqual(self.run_case(3),0)
    def test_all_touch_rejected(self):
        self.assertEqual(self.run_case(4),0)
    def test_all_low_baselines_rejected(self):
        self.assertEqual(self.run_case(5),0)

if __name__ == "__main__":
    unittest.main()
