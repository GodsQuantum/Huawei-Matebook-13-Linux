#!/usr/bin/env python3
"""Execute the ACTUAL FDT-down drain C function with deterministic IRQ timing.

No sensor, enrolled prints, keys or SPI device accessed. A late report must
pass ONLY with the opt-in, and the report must validate structurally.
"""
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
SOURCE = (ROOT/"driver/goodix51a0/goodix51a0.c").read_text()
START = "static gboolean\ngx_send_plain_drain ("
a = SOURCE.index(START)
b = SOURCE.index("/* ------------------------------------------------------------------ */\n/*  Transport glue:", a)
BLOCK = SOURCE[a:b]
assert "GXFP_FDT_DOWN_LATE_REPLY_QA" in BLOCK
assert "gxfp_parse_fdt_response" in BLOCK
assert "gx_capture_transport_desync (self)" in BLOCK

C = r"""
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
typedef unsigned char guint8;
typedef uint16_t guint16;
typedef size_t gsize;
typedef long long gint64;
typedef int gboolean;
#define TRUE 1
#define FALSE 0
#define GOODIX_RX_MAX 512
#define GOODIX_PKT_PLAIN 0xA0
#define GOODIX_PKT_TLS 0xB0
#define GXFP_FDT_ZONE_COUNT 6
#define G_STATIC_ASSERT(x) _Static_assert((x),"assertion failed")
#define GX_DRAIN_SILENCE 4
#define GX_DRAIN_IRQ_POLL_MS 10
#define GX_FDT_DOWN_SECOND_REPLY_TIMEOUT_MS 100
#define GX_FDT_DOWN_LATE_REPLY_QA_TIMEOUT_MS 250
#define GX_GET_IMAGE_ATTEMPTS 2
#define GX_GET_IMAGE_ACK_TIMEOUT_MS 1000
typedef struct {
  int s3_presleep_deadline_us,timing_scale,irq_fd,desync;
  int capture_retry_seen;
  guint8 tls_rx[512];
  int tls_rxlen,tls_rxpos;
} FpiDeviceGoodix51A0;
static int scenario, ack_sent, data_sent, duplicate_sent;
static gint64 clock_us;
#define fp_dbg(...) ((void)0)
#define fp_info(...) do { if(0) printf(__VA_ARGS__); } while(0)
#define fp_warn(...) do { if(0) printf(__VA_ARGS__); } while(0)
static const char *g_getenv(const char *n){return getenv(n);}
static int g_strcmp0(const char *a,const char *b)
{return a == NULL ? -1 : b == NULL ? 1 : strcmp(a,b);}
static gint64 g_get_monotonic_time(void){return clock_us;}
static void g_usleep(unsigned long n){clock_us+=(gint64)n;}
static int gx_write_frame(FpiDeviceGoodix51A0 *d,int type,
                          const guint8 *body,gsize n)
{(void)d;(void)type;(void)body;(void)n;return TRUE;}
static void gx_capture_transport_desync(FpiDeviceGoodix51A0 *d)
{d->desync++;}
static gboolean gxfp_parse_ack(const guint8 *b,gsize n,guint8 cmd,guint8 *status)
{(void)n;(void)cmd;*status=1;return b[0]==0xA1;}
static gboolean gxfp_ack_status_success(guint8 s){return (s & 1)!=0;}
static gboolean gxfp_parse_fdt_response(const guint8 *b,gsize n,
                                        guint8 *touch,guint16 zones[6])
{(void)n;*touch=0;memset(zones,0,6*sizeof(*zones));return b[0]==0xD1;}
static int gx51_wait_irq_gpio48(int fd,int timeout_ms)
{
 (void)fd;
 clock_us+=(gint64)timeout_ms*1000;
 if(!ack_sent)return 0;
 if((scenario==4 || scenario==6) && !duplicate_sent && clock_us>=50000)return 0;
 if(!data_sent && scenario!=2 && scenario!=6 && scenario!=7 && clock_us >= (scenario==0 ? 70000 : 180000))
   return 0;
 return (errno=ETIMEDOUT,-1);
}
static int gx_read_frame(FpiDeviceGoodix51A0 *d,guint8 *type,
                         guint8 *buf,gsize cap)
{
 (void)d;(void)cap;*type=GOODIX_PKT_PLAIN;
 if(!ack_sent){ack_sent=1;buf[0]=scenario==5?0xE1:0xA1;}
 else if((scenario==4 || scenario==6) && !duplicate_sent){duplicate_sent=1;buf[0]=0xA1;}
 else {data_sent=1;buf[0]=(scenario==3?0xE1:0xD1);}
 return 32;
}
""" + BLOCK + r"""
int main(int argc,char **argv)
{
 if(argc != 3)return 98;
 scenario=atoi(argv[1]);
 if(atoi(argv[2]))setenv("GXFP_FDT_DOWN_LATE_REPLY_QA","1",1);
 else unsetenv("GXFP_FDT_DOWN_LATE_REPLY_QA");
 if(scenario==7)unsetenv("GXFP_FDT_DOWN_PROOF_QA");
 else setenv("GXFP_FDT_DOWN_PROOF_QA","1",1);
 FpiDeviceGoodix51A0 dev={0};
 dev.timing_scale=100;
 const guint8 body[]={0x32};
 int ok=gx_send_plain_drain(&dev,body,sizeof body,NULL,NULL);
 if(ok && dev.desync!=0)return 97;
 if(!ok && dev.desync!=1)return 96;
 return ok?0:1;
}
"""

class FdtDownLateReply(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tmp=tempfile.TemporaryDirectory(prefix="gxfp_late_fdt_")
        cls.exe=Path(cls.tmp.name)/"fdt_late"
        src=Path(cls.tmp.name)/"fdt_late.c"
        src.write_text(C)
        subprocess.run(["cc","-std=c11","-O2","-Wall","-Wextra",
                        "-Werror","-pedantic",str(src),"-o",str(cls.exe)],
                       check=True,capture_output=True,text=True)
    @classmethod
    def tearDownClass(cls): cls.tmp.cleanup()
    def case(self,scenario,optin):
        return subprocess.run([str(self.exe),str(scenario),str(int(optin))],
                              check=False).returncode
    def test_early_ack_and_data_default(self):
        self.assertEqual(self.case(0,False),0)
    def test_late_second_reply_default_fails_closed(self):
        self.assertEqual(self.case(1,False),1)
    def test_late_second_reply_optin_preserves_strict_proof(self):
        self.assertEqual(self.case(1,True),0)
    def test_qa_forces_proof_even_when_base_flag_is_unset(self):
        self.assertEqual(self.case(7,True),1)
    def test_missing_second_reply_remains_fail_closed(self):
        self.assertEqual(self.case(2,True),1)
    def test_unrelated_plain_frame_does_not_count(self):
        self.assertEqual(self.case(3,True),1)
    def test_duplicate_ack_does_not_count_as_data(self):
        self.assertEqual(self.case(4,True),0)
    def test_duplicate_ack_without_data_is_not_proof(self):
        self.assertEqual(self.case(6,True),1)
    def test_data_without_valid_ack_is_not_proof(self):
        self.assertEqual(self.case(5,True),1)

if __name__=="__main__": unittest.main()
