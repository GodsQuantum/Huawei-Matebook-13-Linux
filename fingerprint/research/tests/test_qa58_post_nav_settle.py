#!/usr/bin/env python3
"""QA58: compile the real C helper and constrain cleanup-only opt-in safely."""
from pathlib import Path
import subprocess
import tempfile
import unittest
root = Path(__file__).resolve().parents[2]
src = (root / "driver/goodix51a0/goodix51a0.c").read_text()
start = src.index("#define GX_POST_NAV_SETTLE_QA_US 50000u")
end = src.index("\n/* The 51x7 reference", start)
helper = src[start:end]
code = r"""
#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <string.h>
typedef unsigned int guint;
typedef unsigned char guint8;
static const char *g_getenv(const char *x) { return getenv(x); }
static int g_strcmp0(const char *a, const char *b) {
    if (!a) return b ? -1 : 0;
    if (!b) return 1;
    return strcmp(a,b);
}
""" + helper + r"""
int main(int argc, char **argv) {
  if (argc != 5) return 9;
  if (atoi(argv[1])) setenv("GXFP_POST_NAV_SETTLE_QA","1",1);
  else unsetenv("GXFP_POST_NAV_SETTLE_QA");
  guint got = gx_post_nav_settle_us((guint8)strtoul(argv[2],NULL,0),
                                      (guint8)strtoul(argv[3],NULL,0));
  return got == (guint)strtoul(argv[4],NULL,0) ? 0 : 1;
}
"""

class QA58(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tmp = tempfile.TemporaryDirectory(prefix="gxfp58-")
        d = Path(cls.tmp.name)
        (d / "test.c").write_text(code)
        cls.exe = d / "test"
        subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Werror",
                        "-pedantic", str(d / "test.c"), "-o", str(cls.exe)],
                       check=True, capture_output=True)

    @classmethod
    def tearDownClass(cls):
        cls.tmp.cleanup()

    def check(self, enabled, prev, cur, expected):
        args = [str(self.exe), str(int(enabled)), hex(prev), hex(cur), str(expected)]
        self.assertEqual(subprocess.run(args).returncode, 0, args)

    def test_default_disabled_and_phase_exclusive(self):
        for prev, cur in [(0x50,0x32),(0x32,0x20),(0x34,0x20),
                          (0x20,0x50),(0x50,0x36),(0x00,0x32)]:
            self.check(False,prev,cur,0)

    def test_enabled_only_post_nav_then_down(self):
        self.check(True,0x50,0x32,50000)
        for prev,cur in [(0x20,0x32),(0x00,0x32),(0x50,0x20),
                         (0x50,0x36),(0x34,0x32)]:
            self.check(True,prev,cur,0)

    def test_cleanup_only_and_strict_security_unchanged(self):
        cleanup=src[src.index("static gboolean\ngx_send_capture_cleanup ("):
                    src.index("\n\n/* Captures one full image")]
        capture=src[src.index("static gboolean\ngx_send_capture_recipe ("):
                    src.index("\n\n/* Takes exactly one")]
        self.assertEqual(cleanup.count("gx_post_nav_settle_us ("),1)
        self.assertNotIn("gx_post_nav_settle_us (",capture)
        self.assertIn("recipe.steps[i - 1].inner[0]",cleanup)
        self.assertIn("self->s3_presleep_deadline_us",cleanup)
        self.assertIn("gx_send_plain_drain (self, packet->inner, packet->inner_len",cleanup)
        self.assertLess(cleanup.index("g_usleep (settle_us)"),
                        cleanup.index("gx_send_plain_drain (self, packet->inner, packet->inner_len"))
        self.assertIn("FDT_DOWN_PROOF incomplete before GET_IMAGE",src)
        self.assertIn("fdt_data_seen",src)
        self.assertIn("GX_MATCH_THRESHOLD",src)
        self.assertIn("GXFP_POSTAUTH_CLEANUP_DEFER_QA",src)
        self.assertIn("GXFP_DECOUPLE_ACK_PACING_QA",src)

if __name__=="__main__":
    unittest.main(verbosity=2)
