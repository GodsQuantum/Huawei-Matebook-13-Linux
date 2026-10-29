#!/usr/bin/env python3
"""Compile the actual rel71.74 C pacing function and FDT-proof branch; no device I/O."""
from pathlib import Path
import subprocess
import tempfile

src = (Path(__file__).resolve().parents[2] / "driver/goodix51a0/goodix51a0.c").read_text()
assert "GXFP_FDT_DOWN_LATE_REPLY_QA" not in src, "must be based on stable rel71.74, not PR #40"
start = "static void\ngx_capture_transport_desync (FpiDeviceGoodix51A0 *self)"
end = "static void\ngx_capture_pacing_success (FpiDeviceGoodix51A0 *self)"
assert src.count(start) == 1 and src.count(end) == 1
pacing = src[src.index(start):src.index(end)]
start = "          gboolean was_pacing_suppressed = self->capture_pacing_suppressed;"
assert src.count(start) == 1
stop = "          return FALSE;"
fragment = src[src.index(start):src.index(stop, src.index(start))]
assert fragment.count("gx_capture_transport_desync (self);") == 1
assert "self->capture_pacing_suppressed = TRUE;" in fragment
assert "self->capture_pacing_suppressed = was_pacing_suppressed;" in fragment
assert src.count("got < expected_plain_replies || !ack_seen") == 1
body = r"""
#include <stdio.h>
#define GX_CAPTURE_SCALE_MIN 100
#define GX_CAPTURE_SCALE_MAX 300
#define GX_CAPTURE_SCALE_STEP 50
#define GX_SEQ_GAP_US 30000
#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))
#define CLAMP(a,b,c) MIN(MAX(a,b),c)
#define TRUE 1
#define FALSE 0
#define fp_warn(...) ((void)0)
typedef unsigned int guint;
typedef unsigned long long guint64;
typedef int gboolean;
typedef struct { int capture_gap_scale, capture_clean_streak; gboolean capture_recovery_pending, capture_pacing_suppressed; } FpiDeviceGoodix51A0;
static __attribute__((unused)) guint gx_capture_gap_us(FpiDeviceGoodix51A0 *self)
 { return (guint)((guint64)GX_SEQ_GAP_US*(guint)CLAMP(self->capture_gap_scale,GX_CAPTURE_SCALE_MIN,GX_CAPTURE_SCALE_MAX)/100u); }
""" + pacing + r"""
static void fdt_failure(FpiDeviceGoodix51A0 *self) {
 int got=1, expected_plain_replies=2, ack_seen=1;
 (void)got; (void)expected_plain_replies; (void)ack_seen;
""" + fragment + r"""
}
int main(void) {
 FpiDeviceGoodix51A0 d = { .capture_gap_scale=100, .capture_clean_streak=3 };
 for(int n=0;n<6;n++) fdt_failure(&d);
 if(d.capture_gap_scale!=100 || !d.capture_recovery_pending || d.capture_clean_streak!=0 || d.capture_pacing_suppressed) return 1;
 d.capture_recovery_pending=0;
 gx_capture_transport_desync(&d);
 if(d.capture_gap_scale!=150 || !d.capture_recovery_pending) return 2;
 d.capture_pacing_suppressed=1;
 for(int n=0;n<3;n++) fdt_failure(&d);
 if(d.capture_gap_scale!=150 || !d.capture_pacing_suppressed) return 3;
 d.capture_pacing_suppressed=0;
 d.capture_gap_scale=300;
 gx_capture_transport_desync(&d);
 if(d.capture_gap_scale!=300) return 4;
 puts("PASS: FDT proof failures preserve pacing; unrelated capture desync still adapts");
 return 0;
}
"""
with tempfile.TemporaryDirectory(prefix="gxfp-fdt-pacing-") as tmp:
    c = Path(tmp) / "check.c"
    out = Path(tmp) / "check"
    c.write_text(body)
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Werror",
                    "-pedantic", "-O2", str(c), "-o", str(out)],
                   check=True, capture_output=True, text=True)
    subprocess.run([str(out)], check=True)
print("FDT_PACING_C_REGRESSION=PASS")
