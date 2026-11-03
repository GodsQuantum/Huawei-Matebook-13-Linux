#!/usr/bin/env python3
"""Compile and invoke the ACTUAL FDT IRQ missing-report helper without sensor IO."""
from pathlib import Path
import subprocess
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[2]
SOURCE=(ROOT/"driver/goodix51a0/goodix51a0.c").read_text()
START=SOURCE.index("static void\ngx_fdt_missing_report_irq_diag (")
END=SOURCE.index("\n/* Sends a cleartext command",START)
HELPER=SOURCE[START:END]
HARNESS=r"""
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <linux/gpio.h>
typedef long gint64;
typedef unsigned int guint;
typedef struct {int irq_fd;} FpiDeviceGoodix51A0;
static const char *g_getenv(const char *k) {return getenv(k);}
static int g_strcmp0(const char *a,const char *b) {return !a ? (b?-1:0):(!b?1:strcmp(a,b));}
static gint64 g_get_monotonic_time(void){return 2000000;}
static int gpio_reads=0, logs=0;
static int mock_ioctl(int fd, unsigned long request, void *data) {
  struct gpio_v2_line_values *v=data;
  gpio_reads++;
  if(request!=GPIO_V2_LINE_GET_VALUES_IOCTL || fd<0) return -1;
  v->bits=(fd==3?1:0);
  return 0;
}
#define ioctl mock_ioctl
static void capture_warn(const char *format,...) {
  va_list args;
  logs++;
  va_start(args, format);
  vprintf(format, args);
  putchar('\n');
  va_end(args);
}
#define fp_warn(...) capture_warn(__VA_ARGS__)
""" + HELPER +r"""
int main(int argc,char **argv){
  FpiDeviceGoodix51A0 dev={.irq_fd=3};
  if (argc!=2) return 9;
  if (argv[1][0]=='0') unsetenv("GXFP_FDT_IRQ_DIAG_QA");
  else setenv("GXFP_FDT_IRQ_DIAG_QA","1",1);
  gx_fdt_missing_report_irq_diag(&dev,1000000,2,10);
  if(argv[1][0]=='0')return gpio_reads==0&&logs==0?0:1;
  return gpio_reads==1&&logs==1?0:2;
}
"""
class QA60(unittest.TestCase):
 @classmethod
 def setUpClass(cls):
  cls.tmp=tempfile.TemporaryDirectory(prefix="gxfp60-")
  d=Path(cls.tmp.name)
  (d/"test.c").write_text(HARNESS)
  cls.exe=d/"test"
  subprocess.run(["cc","-std=c11","-Wall","-Wextra","-Werror","-pedantic",
                  str(d/"test.c"),"-o",str(cls.exe)],check=True,capture_output=True)
 @classmethod
 def tearDownClass(cls):cls.tmp.cleanup()
 def test_default_off_no_gpio_read_no_log(self):
  r=subprocess.run([str(self.exe),"0"],capture_output=True,text=True)
  self.assertEqual((r.returncode,r.stdout), (0,""))
 def test_enabled_one_read_only_gpio_sample_and_only_metadata(self):
  r=subprocess.run([str(self.exe),"1"],capture_output=True,text=True)
  self.assertEqual(r.returncode,0,r.stderr)
  for value in ("missing_typed_data=1","irq_level=1","irq_after_ack=2",
                "silence_polls=10","ack_age_ms=1000",
                "action=existing_strict_recovery"):
   self.assertIn(value,r.stdout)
  for forbidden in ("sensor_frame","tls_record","template","PMK","spi_frame=",
                    "raw_bytes="):
   self.assertNotIn(forbidden,r.stdout)
 def test_fail_closed_and_no_transport_changes(self):
  drain=SOURCE[SOURCE.index("static gboolean\ngx_send_plain_drain ("):
               SOURCE.index("\n/* ------------------------------------------------------------------ */\n/*  Transport glue:")]
  self.assertIn("gx_capture_transport_desync (self)",drain)
  self.assertIn("!fdt_data_seen",drain)
  self.assertIn("gxfp_parse_fdt_down_reply",drain)
  self.assertEqual(drain.count("gx_fdt_missing_report_irq_diag (self,"),1)
  self.assertIn("fdt_typed_qa && ack_seen &&",drain)
  self.assertIn("fdt_irq_after_ack++",drain)
  self.assertIn("GX_FDT_DOWN_SECOND_REPLY_TIMEOUT_MS 100",drain)
if __name__=="__main__":unittest.main(verbosity=2)
