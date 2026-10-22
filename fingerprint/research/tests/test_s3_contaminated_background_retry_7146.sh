#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/../../" && pwd)"
source="$root/driver/goodix51a0/goodix51a0.c"
grep -Fq 'GX_S3_BG_RETRY_LIMIT' "$source"
grep -Fq 'gx_s3_background_retry_should_reset' "$source"
grep -Fq 'GXFP51A0 NATIVE_S3_FDT_GHOST_RECOVERY' "$source"
grep -Fq 'GX_S3_CLEAR_DWELL_US' "$source"
grep -Fq 'GX_S3_RETOUCH_DWELL_US' "$source"
grep -Fq 'if (gx_s3_background_retry_should_reset (self->s3_first_tls_pending,' "$source"
python3 - "$source" <<'PY'
from pathlib import Path
import sys,tempfile,subprocess
s=Path(sys.argv[1]).read_text()
a=s.index("static gboolean\ngx_s3_background_retry_should_reset")
start=s.index("{",a)
depth=1;end=start+1
while depth:
    depth+=(s[end]=="{")-(s[end]=="}")
    end+=1
fn=s[a:end]
assert "guint contaminated_frames" in fn
assert "post_s3" in fn
assert "GX_S3_BG_RETRY_LIMIT" in fn
assert "return" in fn
unit="""#include <glib.h>
#define GX_S3_BG_RETRY_LIMIT 2
"""+fn+"""
int main(void) {
 g_assert_false(gx_s3_background_retry_should_reset(FALSE, 50));
 g_assert_false(gx_s3_background_retry_should_reset(FALSE, 2));
 g_assert_false(gx_s3_background_retry_should_reset(TRUE, 0));
 g_assert_false(gx_s3_background_retry_should_reset(TRUE, 1));
 g_assert_true(gx_s3_background_retry_should_reset(TRUE, 2));
 g_assert_true(gx_s3_background_retry_should_reset(TRUE, 3));
 return 0;
}
"""
with tempfile.TemporaryDirectory(prefix="gx7146-unit-") as dir:
    p=Path(dir)
    (p/"test.c").write_text(unit)
    argv=["cc","-std=c11","-Werror","-Wall","-Wextra",str(p/"test.c"),"-o",str(p/"test")]
    argv+=subprocess.check_output(["pkg-config","--cflags","--libs","glib-2.0"],text=True).split()
    subprocess.run(argv,check=True)
    subprocess.run([str(p/"test")],check=True)
assert "GX_MATCH_THRESHOLD            7" in s
assert "background contaminated by touch" in s
assert "gx_recover_capture_context (self)" in s
assert "GXFP51A0 CALIBRATION_LIFT_REQUIRED" in s
print("test_s3_contaminated_background_retry_7146: PASS (C unit 6 cases and source safety)")
PY
