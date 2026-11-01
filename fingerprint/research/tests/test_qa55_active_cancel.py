#!/usr/bin/env python3
"""Compile the actual QA55 cancel vfunc in a GLib event-loop harness."""
from pathlib import Path
import re
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
DRIVER = (ROOT / "driver/goodix51a0/goodix51a0.c").read_text()


def extract_cancel():
    match = re.search(r"static void\s*\ngx_dev_cancel\s*\(FpDevice \*dev\)\s*\{", DRIVER)
    if not match:
        raise AssertionError("missing native cancel vfunc")
    brace = match.end() - 1
    nesting = 1
    offset = brace + 1
    while nesting:
        nesting += (DRIVER[offset] == "{") - (DRIVER[offset] == "}")
        offset += 1
    return DRIVER[match.start():offset]


HARNESS = r"""
#include <gio/gio.h>
#include <stdio.h>
typedef struct FpiSsm { int state; int failed; } FpiSsm;
typedef struct FpiDeviceGoodix51A0 {
  FpiSsm *task_ssm;
  guint poll_id;
  int finger_status;
} FpiDeviceGoodix51A0;
typedef FpiDeviceGoodix51A0 FpDevice;
#define FPI_DEVICE_GOODIX51A0(dev) ((FpiDeviceGoodix51A0*)(dev))
#define GX_ST_WAIT_ON 1
#define GX_ST_CAPTURE 2
#define GX_ST_WAIT_OFF 3
#define FP_FINGER_STATUS_NONE 0
#define fp_warn(...) ((void)0)
static int completed = 0;
static int fpi_ssm_get_cur_state (FpiSsm *s) { return s->state; }
static void fpi_device_report_finger_status (FpDevice *d, int s) {d->finger_status=s;}
static void fpi_ssm_mark_failed (FpiSsm *ssm, GError *error) {
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
  ssm->failed++;
  completed++;
  g_error_free (error);
}
static gboolean dummy (gpointer data) { (void)data; return G_SOURCE_CONTINUE; }
"""
TESTS = r"""
int main (void) {
  FpDevice d = {0};
  FpiSsm s = {0};
  guint id;
  g_setenv ("GXFP_CANCEL_PENDING_POLL_QA", "1", TRUE);

  s.state = GX_ST_WAIT_ON;
  d.task_ssm = &s;
  d.poll_id = id = g_timeout_add (20000, dummy, NULL);
  gx_dev_cancel (&d);
  g_assert_cmpint (s.failed, ==, 1);
  g_assert_cmpint (d.poll_id, ==, 0);
  g_assert_null (g_main_context_find_source_by_id (NULL, id));
  d.task_ssm = NULL; /* emulate synchronous SSM completion callback */
  gx_dev_cancel (&d);
  g_assert_cmpint (completed, ==, 1);

  s.failed = 0;
  s.state = GX_ST_WAIT_OFF;
  d.task_ssm = &s;
  d.poll_id = id = g_timeout_add (20000, dummy, NULL);
  gx_dev_cancel (&d);
  g_assert_cmpint (s.failed, ==, 1);
  g_assert_null (g_main_context_find_source_by_id (NULL, id));

  s.failed = 0;
  s.state = GX_ST_CAPTURE; /* worker owns SPI, never force-complete */
  d.poll_id = 0;
  gx_dev_cancel (&d);
  g_assert_cmpint (s.failed, ==, 0);

  s.state = GX_ST_WAIT_ON;
  d.poll_id = id = g_timeout_add (20000, dummy, NULL);
  g_unsetenv ("GXFP_CANCEL_PENDING_POLL_QA");
  gx_dev_cancel (&d);
  g_assert_cmpint (s.failed, ==, 0);
  g_assert_nonnull (g_main_context_find_source_by_id (NULL, id));
  g_source_remove (id);

  g_setenv ("GXFP_CANCEL_PENDING_POLL_QA", "1", TRUE);
  d.poll_id = 0;
  gx_dev_cancel (&d);
  g_assert_cmpint (s.failed, ==, 0);
  g_assert_cmpint (completed, ==, 2);
  puts ("QA55_MAINLOOP_CANCEL_5_SCENARIOS_PASS");
  return 0;
}
"""


class QA55Cancel(unittest.TestCase):
    def test_native_cancellation_is_opt_in_and_scoped(self):
        self.assertIn("dev_class->cancel = gx_dev_cancel;", DRIVER)
        self.assertEqual(DRIVER.count("->task_ssm = ssm;"), 3)
        self.assertEqual(DRIVER.count("->task_ssm = NULL;"), 3)
        self.assertIn('g_strcmp0 (g_getenv ("GXFP_CANCEL_PENDING_POLL_QA"), "1")', DRIVER)
        self.assertIn("if (state != GX_ST_WAIT_ON && state != GX_ST_WAIT_OFF)", DRIVER)

    def test_real_source_compiles_and_responds_in_glib(self):
        with tempfile.TemporaryDirectory(prefix="gxfp-qa55-") as temp:
            source = Path(temp) / "cancel.c"
            exe = Path(temp) / "cancel"
            source.write_text(HARNESS + "\n" + extract_cancel() + "\n" + TESTS)
            flags = subprocess.check_output(
                ["pkg-config", "--cflags", "--libs", "gio-2.0"], text=True
            ).split()
            subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Werror",
                            str(source), *flags, "-o", str(exe)], check=True)
            result = subprocess.run([str(exe)], check=True, text=True,
                                    capture_output=True)
            self.assertIn("QA55_MAINLOOP_CANCEL_5_SCENARIOS_PASS", result.stdout)


if __name__ == "__main__":
    unittest.main(verbosity=2)
