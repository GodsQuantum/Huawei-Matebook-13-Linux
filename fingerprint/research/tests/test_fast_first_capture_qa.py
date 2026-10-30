#!/usr/bin/env python3
"""Compile actual QA fastlane guard from goodix51a0.c, no sensor access."""
from pathlib import Path
import subprocess
import tempfile
src=(Path(__file__).resolve().parents[2]/'driver/goodix51a0/goodix51a0.c').read_text()
start=src.index('#define GX_BG_FAST_FIRST_MAX_US')
end=src.index('static void\ngx_invalidate_capture_context',start)
actual=src[start:end]
assert src.count('gx_fast_first_capture_qa_eligible (self)')==1
assert 'GXFP_FIRST_CAPTURE_FAST_QA' in actual
session=src[src.index('static gboolean\ngx_session_start ('):src.index('/* Keep a healthy same-process context')]
assert 'if (!gx_background_is_fresh (self) &&\n          gx_fast_first_capture_qa_eligible (self))' in session
assert 'else if (!gx_background_is_fresh (self))' in session
assert 'gx_warm_validate (self)' in session
retry=src[src.index('gx_session_thread (GTask *task'):src.index('gx_capture_thread (GTask *task')]
assert 't->rebase_before_retry' in retry and 'gx_warm_validate (self)' in retry
assert 'if (gx_background_is_fresh (self))' in retry
h=r"""
#define _POSIX_C_SOURCE 200809L
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef long long gint64;
typedef int gboolean;
#define TRUE 1
#define FALSE 0
#define G_GINT64_CONSTANT(x) (x##LL)
#define G_USEC_PER_SEC 1000000LL
#define GX_BG_FRESH_MAX_US (20LL*1000000LL)
typedef struct {
 void *tls, *bg_frame;
 int production_ready, warm_valid, tls_up, have_fdt,
     bg_dirty, force_cold_reset, capture_recovery_pending;
 gint64 warm_last_activity_us;
} FpiDeviceGoodix51A0;
static gint64 test_now=500000000LL;
static gint64 test_bg_age=249214000LL;
static gint64 g_get_monotonic_time(void){return test_now;}
static gint64 gx_background_age_us(FpiDeviceGoodix51A0 *d){(void)d;return test_bg_age;}
static const char *g_getenv(const char *key){return getenv(key);}
static int g_strcmp0(const char *a,const char *b){return a?strcmp(a,b):b?-1:0;}
"""
ending=r"""
int main(void){
 FpiDeviceGoodix51A0 d={
  .tls=(void*)1, .bg_frame=(void*)1, .production_ready=1, .warm_valid=1,
  .tls_up=1, .have_fdt=1, .warm_last_activity_us=500000000LL-52000LL
 };
 unsetenv("GXFP_FIRST_CAPTURE_FAST_QA");
 if(gx_fast_first_capture_qa_eligible(&d))return 1;
 setenv("GXFP_FIRST_CAPTURE_FAST_QA","1",1);
 if(!gx_fast_first_capture_qa_eligible(&d))return 2;
 test_bg_age=300001000LL;
 if(gx_fast_first_capture_qa_eligible(&d))return 3;
 test_bg_age=20000000LL;
 if(gx_fast_first_capture_qa_eligible(&d))return 4;
 test_bg_age=249214000LL;
 d.bg_dirty=1;
 if(gx_fast_first_capture_qa_eligible(&d))return 5;
 d.bg_dirty=0; d.tls_up=0;
 if(gx_fast_first_capture_qa_eligible(&d))return 6;
 d.tls_up=1; d.capture_recovery_pending=1;
 if(gx_fast_first_capture_qa_eligible(&d))return 7;
 d.capture_recovery_pending=0; d.force_cold_reset=1;
 if(gx_fast_first_capture_qa_eligible(&d))return 8;
 d.force_cold_reset=0; d.warm_last_activity_us=test_now-2000001LL;
 if(gx_fast_first_capture_qa_eligible(&d))return 9;
 d.warm_last_activity_us=test_now+1;
 if(gx_fast_first_capture_qa_eligible(&d))return 10;
 d.warm_last_activity_us=test_now-52000; d.warm_valid=0;
 if(gx_fast_first_capture_qa_eligible(&d))return 11;
 puts("FAST_FIRST_CAPTURE_REAL_GUARD=PASS; default off, age/ready/TLS/recovery/dirty gates");
 return 0;
}
"""
with tempfile.TemporaryDirectory(prefix="gxfp-firstcapture-") as tmp:
 ff=Path(tmp)/"case.c";exe=Path(tmp)/"case";ff.write_text(h+actual+ending)
 r=subprocess.run(["cc","-std=c11","-O2","-Wall","-Wextra","-Werror","-pedantic",str(ff),"-o",str(exe)],capture_output=True,text=True)
 if r.returncode:print(r.stderr);r.check_returncode()
 subprocess.run([str(exe)],check=True)
print("FAST_FIRST_CAPTURE_REGRESSION=PASS")
