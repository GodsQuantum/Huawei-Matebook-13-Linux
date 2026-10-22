#!/usr/bin/env bash
set -euo pipefail
driver="$(cd "$(dirname "$0")/../.." && pwd)/driver/goodix51a0/goodix51a0.c"
python3 - "$driver" <<'PYTEST'
from pathlib import Path
import sys, tempfile, subprocess
s=Path(sys.argv[1]).read_text()
start=s.index("static gboolean\ngx_fdt_clear_stable_sample (")
end=s.index("\n/* Only called inside a genuine libfprint",start)
helper=s[start:end]
assert "touchflag & 0x3fu" in helper
assert "mean < GOODIX_FDT_ABS" in helper
assert "anchor_mean - mean > GX_DIAG_BASELINE_MAX_DRIFT" in helper
assert "now_us - *clean_since_us >= dwell_us" in helper
assert "GX_CALIB_RETOUCH_DWELL_US" in s
assert "CALIBRATION_LIFT_REQUIRED" in s
assert "gx_wait_sensor_clear_stable (self, off_anchor_mean, NULL," in s
assert "mean >= GOODIX_FDT_ABS" in s[s.index("if ((touchflag & 0x3fu) == 0"):s.index("if ((touchflag & 0x3fu) == 0")+180]
# Test real C helper extracted from driver, not an independently reimplemented algorithm.
test=r"""
#include <assert.h>
#include <stdio.h>
typedef unsigned char guint8;
typedef long long gint64;
typedef int gboolean;
#define FALSE 0
#define TRUE 1
#define GOODIX_FDT_ABS 340
#define GX_DIAG_BASELINE_MAX_DRIFT 20
"""+helper+r"""
int main(void) {
  gint64 since=0;
  const gint64 dwell=260000;
  assert(!gx_fdt_clear_stable_sample(0x00,352,-1,1000000,dwell,&since));
  assert(!gx_fdt_clear_stable_sample(0x00,351,-1,1120000,dwell,&since));
  assert(gx_fdt_clear_stable_sample(0x00,353,-1,1260001,dwell,&since));
  assert(!gx_fdt_clear_stable_sample(0x01,352,-1,1300000,dwell,&since));
  assert(since==0);
  assert(!gx_fdt_clear_stable_sample(0x00,351,-1,1400000,dwell,&since));
  assert(!gx_fdt_clear_stable_sample(0x00,351,-1,1550000,dwell,&since));
  assert(gx_fdt_clear_stable_sample(0x00,351,-1,1660001,dwell,&since));
  assert(!gx_fdt_clear_stable_sample(0x00,223,-1,1700000,dwell,&since));
  assert(since==0);
  assert(!gx_fdt_clear_stable_sample(0x20,353,-1,1800000,dwell,&since));
  assert(!gx_fdt_clear_stable_sample(0x00,343,370,1900000,dwell,&since));
  assert(!gx_fdt_clear_stable_sample(0x00,340,352,2000000,650000,&since));
  assert(!gx_fdt_clear_stable_sample(0x00,348,352,2500000,650000,&since));
  assert(gx_fdt_clear_stable_sample(0x00,348,352,2650001,650000,&since));
  puts("calibration stable-FDT C unit: PASS (partial contact, bounce, dwell, baseline drift)");
  return 0;
}
"""
with tempfile.TemporaryDirectory(prefix="gxfp-fdt-test-") as tmp:
    src=Path(tmp)/"test.c"
    exe=Path(tmp)/"test"
    src.write_text(test)
    subprocess.run(["cc","-std=c11","-Wall","-Wextra","-Werror","-O2",str(src),"-o",str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
PYTEST
echo 'test_calibration_stable_fdt_source_safety: PASS'
