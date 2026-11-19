#!/usr/bin/env python3
"""Exercise the driver contact lifecycle with synthetic FDT and image replies."""

from pathlib import Path
import shlex
import subprocess
import tempfile
import unittest


DRIVER = (Path(__file__).resolve().parents[2] / "driver/goodix51a0/goodix51a0.c").read_text()


def driver_function(name):
    # The newline after the return type distinguishes definitions from prototypes.
    definition = DRIVER.index(f"\n{name} (")
    start = DRIVER.rindex("\nstatic ", 0, definition)
    end = DRIVER.index("\n}", definition) + len("\n}")
    return DRIVER[start:end]


STUBS = r"""
#include <gio/gio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GXFP_FDT_ZONE_COUNT 6
#define GX_FDT_TOUCH_MIN_ZONES 5
#define GOODIX_FDT_ABS 340
#define GOODIX_FDT_ABS_MARGIN 24
#define GOODIX_FDT_DROP 30
#define GOODIX_IMG_PIXELS (80 * 64)
#define GX_POLL_OFF 100
#define GX_POLL_MAX 300
#define GX_POLL_MS 100
#define GX_COLD_CONTACT_READY_US G_GINT64_CONSTANT (100000)
#define GX_COLD_PARTIAL_GRACE_US G_GINT64_CONSTANT (600000)
#define GX_RECOVERY_OFF_POLLS 2
#define GX_VERIFY_MAX_ATTEMPTS 3
#define GX_ENROLL_STAGES 20
#define FP_DEVICE_RETRY_GENERAL 0
#define FP_DEVICE_RETRY_REMOVE_FINGER 4
#define FP_FINGER_STATUS_NONE 0
#define FP_FINGER_STATUS_NEEDED 1
#define FP_FINGER_STATUS_PRESENT 2

enum { GX_ST_SESSION, GX_ST_WAIT_ON, GX_ST_CAPTURE, GX_ST_WAIT_OFF, GX_ST_DONE };

typedef struct {
  gboolean capture_pacing_suppressed, capture_recovery_pending, have_fdt;
  gboolean fast_first_poll_armed, needs_release;
  int fdt_base[GXFP_FDT_ZONE_COUNT], fdt_abs;
  guint16 bg_frame[GOODIX_IMG_PIXELS];
  gint64 bg_last_refresh_us, warm_last_activity_us;
  guint poll_id;
  int probes, captures, none_reports, status, enroll_retry_stage;
  GCancellable *cancellable;
} FpiDeviceGoodix51A0, FpDevice;

typedef struct {
  gboolean verifying, identifying, match_reported;
  gboolean rearm_mcu_before_retry, rebase_before_retry;
  gboolean cold_contact_gate;
  const char *views;
  gint64 contact_since_us;
  int tries, stage, polls;
} GxTask;

typedef struct {
  FpDevice *dev;
  GxTask task;
  int state;
  GError *error;
} FpiSsm;

#define FPI_DEVICE_GOODIX51A0(dev) (dev)
#define fp_dbg(...) ((void) 0)
#define fp_info(...) do { if (FALSE) g_print (__VA_ARGS__); } while (0)
#define fp_warn(...) do { \
  if (strstr (first_argument (__VA_ARGS__, NULL), " RELEASED")) release_logs++; \
  if (strstr (first_argument (__VA_ARGS__, NULL), " READY")) ready_logs++; \
  if (FALSE) g_print (__VA_ARGS__); \
} while (0)

static const char *
first_argument (const char *format, ...)
{
  return format;
}

static int release_logs, ready_logs;
static int before_mask, after_mask, sample_mean, fail_probe;
static gboolean capture_fails;

static gint64
test_clock (void)
{
  static gint64 time_us = 1000;

  time_us += 100;
  return time_us;
}
#define g_get_monotonic_time test_clock

static int
gx_fdt_probe_ex (FpiDeviceGoodix51A0 *self, int *zones, guint8 *mask)
{
  const int probe = ++self->probes;

  if (fail_probe == -1 || probe == fail_probe)
    return -1;
  *mask = (guint8) (probe == 1 ? before_mask : after_mask);
  for (int i = 0; i < GXFP_FDT_ZONE_COUNT; i++)
    zones[i] = sample_mean;
  return 0;
}

static gboolean
gx_capture_warm_background_frame (FpiDeviceGoodix51A0 *self, guint16 *frame)
{
  self->captures++;
  g_assert_true (self->capture_pacing_suppressed);
  for (int i = 0; i < GOODIX_IMG_PIXELS; i++)
    frame[i] = 777;
  return !capture_fails;
}

static FpDevice *
fpi_ssm_get_device (FpiSsm *ssm)
{
  return ssm->dev;
}

static GxTask *
fpi_ssm_get_data (FpiSsm *ssm)
{
  return &ssm->task;
}

static gboolean
fpi_device_report_finger_status (FpDevice *dev, int status)
{
  gboolean changed = dev->status != status;

  dev->status = status;
  if (status == FP_FINGER_STATUS_NONE)
    dev->none_reports++;
  return changed;
}

static void gx_run_state (FpiSsm *ssm, FpDevice *dev);

static int
fpi_ssm_get_cur_state (FpiSsm *ssm)
{
  return ssm->state;
}

static void
fpi_ssm_jump_to_state (FpiSsm *ssm, int state)
{
  ssm->state = state;
  gx_run_state (ssm, ssm->dev);
}

static void
fpi_ssm_mark_completed (FpiSsm *ssm)
{
  g_assert_cmpint (ssm->state, ==, GX_ST_DONE);
}

static guint
test_timeout_add (guint milliseconds, GSourceFunc callback, gpointer data)
{
  g_assert_cmpuint (milliseconds, >, 0);
  g_assert_nonnull (callback);
  g_assert_nonnull (data);
  return 42;
}
#define g_timeout_add test_timeout_add

static GTaskThreadFunc gx_session_thread, gx_capture_thread;
static GAsyncReadyCallback gx_session_done, gx_capture_done;

static void
gx_run_async (FpiSsm *ssm, FpDevice *dev,
              GTaskThreadFunc thread, GAsyncReadyCallback done)
{
  /* A simulated SESSION worker is finished explicitly by its test case. */
  (void) ssm;
  (void) dev;
  (void) thread;
  (void) done;
}

static void
fpi_ssm_mark_failed (FpiSsm *ssm, GError *error)
{
  g_clear_error (&ssm->error);
  ssm->error = error;
}

static GError *
fpi_device_retry_new (int code)
{
  return g_error_new_literal (g_quark_from_static_string ("retry"), code, "retry");
}

static void
fpi_device_enroll_progress (FpDevice *dev, int stage, gpointer print, GError *error)
{
  g_assert_null (print);
  g_assert_true (g_error_matches (error, g_quark_from_static_string ("retry"),
                                  FP_DEVICE_RETRY_REMOVE_FINGER));
  dev->enroll_retry_stage = stage;
  g_error_free (error);
}

static GCancellable *
gx_action_cancellable_if_active (FpiDeviceGoodix51A0 *self)
{
  return self->cancellable;
}

static gboolean
gx_active_sleep_recovery (FpDevice *dev, FpiSsm *ssm)
{
  (void) dev;
  (void) ssm;
  return FALSE;
}
"""

SCENARIOS = r"""
static void
run_warm (const char *mode, gboolean pacing_suppressed)
{
  FpDevice dev = {
    .capture_pacing_suppressed = pacing_suppressed,
    .fdt_base = { 350, 350, 350, 350, 350, 350 },
    .fdt_abs = 326,
    .bg_last_refresh_us = 100,
    .warm_last_activity_us = 200,
    .have_fdt = TRUE,
  };
  gboolean ready;
  gboolean image_retained = TRUE;
  gboolean image_updated = TRUE;
  const int original_fdt[] = { 350, 350, 350, 350, 350, 350 };
  const int updated_fdt[] = { 355, 355, 355, 355, 355, 355 };

  for (int i = 0; i < GOODIX_IMG_PIXELS; i++)
    dev.bg_frame[i] = 123;
  ready = strcmp (mode, "fast") == 0
          ? gx_warm_fast_ready (&dev)
          : gx_warm_validate (&dev);
  for (int i = 0; i < GOODIX_IMG_PIXELS; i++)
    {
      image_retained &= dev.bg_frame[i] == 123;
      image_updated &= dev.bg_frame[i] == 777;
    }
  printf ("ready=%d probes=%d captures=%d pacing=%d image=%s refresh=%s "
          "fdt=%s floor=%d activity=%s have_fdt=%d fast_poll=%d\n",
          ready, dev.probes, dev.captures, dev.capture_pacing_suppressed,
          image_retained ? "retained" : image_updated ? "updated" : "corrupt",
          dev.bg_last_refresh_us == 100 ? "retained" : "updated",
          memcmp (dev.fdt_base, original_fdt, sizeof original_fdt) == 0 ? "retained"
            : memcmp (dev.fdt_base, updated_fdt, sizeof updated_fdt) == 0 ? "updated"
            : "corrupt",
          dev.fdt_abs, dev.warm_last_activity_us == 200 ? "retained" : "updated",
          dev.have_fdt, dev.fast_first_poll_armed);
}

static const char *state_name (int state);

static void
run_release (const char *action, gboolean rearm, gboolean complete)
{
  FpDevice dev = {
    .poll_id = 41,
    .fdt_base = { 355, 355, 355, 355, 355, 355 },
    .fdt_abs = 331,
  };
  FpiSsm ssm = { .dev = &dev, .state = GX_ST_WAIT_OFF };
  GxTask *task = &ssm.task;
  gboolean keep_polling;
  const char *error = "none";

  task->verifying = strcmp (action, "enroll") != 0;
  task->identifying = strcmp (action, "identify") == 0;
  task->tries = 1;
  task->stage = complete ? 20 : 1;
  task->match_reported = complete;
  task->rearm_mcu_before_retry = rearm;
  dev.capture_recovery_pending = strcmp (action, "recovery") == 0;
  if (strcmp (action, "cancel") == 0)
    {
      dev.cancellable = g_cancellable_new ();
      g_cancellable_cancel (dev.cancellable);
    }

  do
    {
      keep_polling = gx_poll_off (&ssm);
    }
  while (keep_polling && dev.probes <= 101);

  if (ssm.error)
    {
      if (g_error_matches (ssm.error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
        error = "cancelled";
      else if (g_error_matches (ssm.error, g_quark_from_static_string ("retry"),
                                FP_DEVICE_RETRY_REMOVE_FINGER))
        error = "remove_finger";
      else
        error = "unexpected";
    }
  printf ("state=%s probes=%d none_reports=%d release_logs=%d poll_id=%u "
          "rebase=%d tries=%d error=%s polling=%d status=%d ready=%d\n",
          state_name (ssm.state), dev.probes, dev.none_reports, release_logs,
          dev.poll_id, task->rebase_before_retry, task->tries, error,
          keep_polling, dev.status, ready_logs);
  g_clear_object (&dev.cancellable);
  g_clear_error (&ssm.error);
}

static const char *
state_name (int state)
{
  const char *names[] = { "session", "wait_on", "capture", "wait_off", "done" };

  return names[state];
}

static gboolean
wait_on_probe (FpiSsm *ssm, guint8 mask, int mean, gboolean unavailable)
{
  before_mask = after_mask = mask;
  sample_mean = mean;
  fail_probe = unavailable ? -1 : 0;
  return gx_poll_on (ssm);
}

static void
run_late_release (const char *scenario)
{
  FpDevice dev = {
    .poll_id = 41, .status = FP_FINGER_STATUS_PRESENT,
    .fdt_base = { 355, 355, 355, 355, 355, 355 },
    .fdt_abs = 331,
  };
  FpiSsm first = {
    .dev = &dev, .state = GX_ST_WAIT_OFF,
    .task = { .verifying = TRUE, .tries = 2 },
  };
  FpiSsm new_request = {
    .dev = &dev, .state = GX_ST_WAIT_ON,
    .task = { .verifying = TRUE, .identifying = TRUE },
  };
  FpiSsm *waiting = &first;
  gboolean keep_polling;
  int initial_probes, initial_state, initial_status, initial_ready_logs;
  int after_clear;
  int capture_tries;
  gboolean rebase;
  gboolean aborted = FALSE;
  const char *wait_error = "none";

  dev.capture_recovery_pending = g_str_has_prefix (scenario, "recover");
  first.task.rearm_mcu_before_retry = strcmp (scenario, "rebase") == 0;
  if (g_str_has_prefix (scenario, "enroll"))
    {
      first.task.verifying = FALSE;
      first.task.stage = 19;
      first.task.views = "kept";
    }
  before_mask = after_mask = 63;
  sample_mean = 300;
  if (strcmp (scenario, "recover-probe-error") == 0)
    fail_probe = -1;
  do
    keep_polling = gx_poll_off (&first);
  while (keep_polling && dev.probes <= 101);

  initial_probes = dev.probes;
  initial_state = first.state;
  if (dev.capture_recovery_pending)
    {
      /* Simulate the existing SESSION worker completing transport recovery. */
      dev.capture_recovery_pending = FALSE;
      fpi_ssm_jump_to_state (&first, GX_ST_WAIT_ON);
    }
  initial_status = dev.status;
  initial_ready_logs = ready_logs;

  if (g_str_has_suffix (scenario, "cancel"))
    {
      dev.cancellable = g_cancellable_new ();
      g_cancellable_cancel (dev.cancellable);
      gx_poll_on (&first);
      aborted = g_error_matches (first.error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
      g_clear_object (&dev.cancellable);
      if (first.task.verifying)
        {
          waiting = &new_request;
          gx_run_state (waiting, &dev);
        }
    }

  dev.probes = 0;
  if (strcmp (scenario, "held") == 0)
    {
      do
        keep_polling = wait_on_probe (waiting, 63, 300, FALSE);
      while (keep_polling && dev.probes <= 301);
      after_clear = waiting->state;
    }
  else if (strcmp (scenario, "boundary") == 0)
    {
      do
        keep_polling = wait_on_probe (waiting, 63, 300, FALSE);
      while (keep_polling && dev.probes < 300);
      if (keep_polling)
        keep_polling = wait_on_probe (waiting, 0, 355, FALSE);
      after_clear = waiting->state;
      if (keep_polling)
        keep_polling = wait_on_probe (waiting, 63, 300, FALSE);
    }
  else if (strcmp (scenario, "enroll-cancel") == 0)
    {
      keep_polling = FALSE;
      after_clear = waiting->state;
    }
  else
    {
      /* Failed, partial and low-ADC samples must leave the old pose active. */
      keep_polling = wait_on_probe (waiting, 0, 355, TRUE);
      if (keep_polling)
        keep_polling = wait_on_probe (waiting, 3, 300, FALSE);
      if (keep_polling)
        keep_polling = wait_on_probe (waiting, 0, 300, FALSE);
      if (keep_polling)
        keep_polling = wait_on_probe (waiting, 0, 355, FALSE);
      after_clear = waiting->state;
      if (keep_polling)
        keep_polling = wait_on_probe (waiting, 63, 300, FALSE);
    }

  capture_tries = waiting->task.tries;
  rebase = waiting->task.rebase_before_retry;
  if (waiting->error)
    wait_error = g_error_matches (waiting->error, G_IO_ERROR, G_IO_ERROR_TIMED_OUT)
                 ? "timeout"
                 : g_error_matches (waiting->error, G_IO_ERROR, G_IO_ERROR_CANCELLED)
                   ? "cancelled" : "unexpected";
  printf ("initial_probes=%d initial_state=%s initial_status=%d initial_ready=%d "
          "aborted=%d probes=%d after_clear=%s state=%s tries=%d rebase=%d "
          "ready=%d polling=%d poll_id=%u wait_error=%s stage=%d "
          "progress_stage=%d views=%s",
          initial_probes, state_name (initial_state), initial_status,
          initial_ready_logs, aborted, dev.probes, state_name (after_clear),
          state_name (waiting->state), capture_tries, rebase, ready_logs,
          keep_polling, dev.poll_id, wait_error, waiting->task.stage,
          dev.enroll_retry_stage, waiting->task.views ? waiting->task.views : "none");

  if (strcmp (scenario, "late") == 0 && waiting->state == GX_ST_CAPTURE)
    {
      /* Simulate the third scored pose completing with a terminal report. */
      waiting->task.tries++;
      waiting->task.match_reported = TRUE;
      waiting->task.polls = 0;
      waiting->state = GX_ST_WAIT_OFF;
      before_mask = after_mask = 0;
      sample_mean = 355;
      gx_poll_off (waiting);
    }
  if (strcmp (scenario, "enroll") == 0 && waiting->state == GX_ST_CAPTURE)
    {
      /* Simulate acceptance of the last needed view. */
      waiting->task.stage++;
      waiting->task.polls = 0;
      waiting->state = GX_ST_WAIT_OFF;
      before_mask = after_mask = 0;
      sample_mean = 355;
      gx_poll_off (waiting);
    }
  printf (" final_state=%s final_tries=%d final_stage=%d\n",
          state_name (waiting->state), waiting->task.tries, waiting->task.stage);
  g_clear_error (&first.error);
  g_clear_error (&new_request.error);
}

int
main (int argc, char **argv)
{
  if (argc != 9)
    return 2;
  before_mask = atoi (argv[3]);
  after_mask = atoi (argv[4]);
  sample_mean = atoi (argv[5]);
  fail_probe = atoi (argv[6]);
  capture_fails = atoi (argv[7]);
  if (strcmp (argv[1], "late_release") == 0)
    run_late_release (argv[2]);
  else if (strcmp (argv[1], "release") == 0)
    run_release (argv[2], capture_fails, atoi (argv[8]));
  else
    run_warm (argv[1], atoi (argv[8]));
  return 0;
}
"""


class ContactLifecycle(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        tmp = tempfile.TemporaryDirectory(prefix="gxfp-contact-")
        cls.addClassCleanup(tmp.cleanup)
        src = Path(tmp.name) / "contact.c"
        cls.exe = Path(tmp.name) / "contact"
        functions = (
            "gx_fdt_touch_count", "gx_fdt_touch_is_finger", "gx_fdt_mean",
            "gx_fdt_drop", "gx_cancelled", "gx_cold_contact_gate_ready",
            "gx_fdt_release_confirmed", "gx_report_ready", "gx_report_release",
            "gx_retry_rebase_after_release", "gx_poll_on", "gx_poll_first_held",
            "gx_poll_off", "gx_run_state",
            "gx_warm_fast_ready", "gx_warm_validate",
        )
        src.write_text(STUBS + "".join(map(driver_function, functions)) + SCENARIOS)
        flags = subprocess.check_output(
            ["pkg-config", "--cflags", "--libs", "gio-2.0"], text=True,
        )
        subprocess.run(
            ["cc", "-std=c11", "-Wall", "-Wextra", "-Werror",
             str(src), *shlex.split(flags), "-o", str(cls.exe)],
            check=True,
        )

    def case(self, mode, action="-", before=0, after=None, mean=355, fail_probe=0,
             flag=0, complete=0, **expected):
        if after is None:
            after = before if mode == "release" else 0
        result = subprocess.run(
            [str(self.exe), mode, action, str(before), str(after), str(mean),
             str(fail_probe), str(flag), str(complete)],
            check=True, capture_output=True, text=True, timeout=5,
        )
        observed = dict(field.split("=") for field in result.stdout.split())
        self.assertEqual({key: observed[key] for key in expected},
                         {key: str(value) for key, value in expected.items()})

    def test_warm_partial_contact_defers_background_capture(self):
        for mask in (1, 3, 7, 15):
            with self.subTest(mask=mask):
                self.case("warm", before=mask, ready=1, probes=1, captures=0,
                          image="retained", refresh="retained", fdt="retained",
                          floor=326, activity="updated", pacing=0)

    def test_warm_partial_contact_discards_captured_background(self):
        for mask in (1, 3, 7, 15):
            with self.subTest(mask=mask):
                self.case("warm", after=mask, ready=1, probes=2, captures=1,
                          image="retained", refresh="retained", fdt="retained",
                          floor=326, activity="updated", pacing=0)

    def test_warm_clear_sensor_adopts_new_background(self):
        self.case("warm", ready=1, probes=2, captures=1, image="updated",
                  refresh="updated", fdt="updated", floor=331, have_fdt=1,
                  activity="updated", pacing=0)

    def test_warm_failed_validation_preserves_reference_and_restores_pacing(self):
        for fail_probe, capture_fails, previous_pacing in (
            (1, 0, 0), (2, 0, 0), (0, 1, 0), (0, 1, 1),
        ):
            with self.subTest(fail_probe=fail_probe, capture_fails=capture_fails,
                              previous_pacing=previous_pacing):
                self.case("warm", fail_probe=fail_probe, flag=capture_fails,
                          complete=previous_pacing, ready=0, image="retained",
                          refresh="retained", fdt="retained", floor=326,
                          pacing=previous_pacing, activity="retained")

    def test_fast_ready_contact_retains_fdt_reference(self):
        for mask in (1, 3, 7, 15, 63):
            with self.subTest(mask=mask):
                self.case("fast", before=mask, ready=1, captures=0,
                          fdt="retained", floor=326, activity="updated")

    def test_fast_ready_clear_sensor_refreshes_only_fdt(self):
        self.case("fast", before=0x80, ready=1, captures=0, fdt="updated",
                  floor=331, have_fdt=1, image="retained", refresh="retained")

    def test_fast_ready_full_held_hint_and_transport_fallback(self):
        self.case("fast", before=63, mean=300, ready=1, fast_poll=1, fdt="retained")
        self.case("fast", fail_probe=1, ready=0, fast_poll=0, fdt="retained",
                  activity="retained")

    def test_release_timeout_never_invents_another_auth_pose(self):
        for action, mask, fail_probe, rearm in (
            ("verify", 63, 0, 1), ("identify", 63, 0, 0),
            ("verify", 0, -1, 1), ("identify", 0, -1, 0),
        ):
            with self.subTest(action=action, mask=mask, fail_probe=fail_probe):
                self.case("release", action, before=mask, fail_probe=fail_probe,
                          flag=rearm, probes=101, state="wait_on", none_reports=0,
                          release_logs=0, poll_id=42, rebase=0, tries=1,
                          error="none", polling=0, status=2, ready=0)

    def test_release_partial_mask_is_not_clear_even_when_mean_is_high(self):
        self.case("release", "verify", before=1, probes=101, state="wait_on",
                  release_logs=0, error="none", status=2, ready=0)

    def test_release_actual_clear_retries_with_next_timer_preserved(self):
        self.case("release", "identify", before=0x80, probes=1, state="wait_on",
                  release_logs=1, none_reports=1, rebase=0, poll_id=42,
                  error="none", polling=0)

    def test_release_waits_for_partial_contact_to_clear(self):
        self.case("release", "verify", before=7, after=0, probes=2, state="wait_on",
                  release_logs=1, none_reports=1, poll_id=42, error="none")

    def test_release_actual_clear_can_rebase_for_retry(self):
        self.case("release", "verify", flag=1, probes=1, state="session",
                  release_logs=1, none_reports=1, rebase=1, poll_id=0,
                  error="none")

    def test_release_transport_recovery_is_bounded_without_rebase(self):
        self.case("release", "recovery", fail_probe=-1, probes=3, state="session",
                  rebase=0, release_logs=0, tries=1, error="none", poll_id=0)
        self.case("release", "recovery", probes=1, state="session", rebase=0,
                  release_logs=1, none_reports=1, error="none")

    def test_late_release_preserves_attempt_budget_and_then_allows_third_pose(self):
        self.case("late_release", "late", initial_probes=101, initial_state="wait_on",
                  initial_status=2, initial_ready=0, after_clear="wait_on", probes=5,
                  state="capture", tries=2, ready=1, wait_error="none",
                  final_state="done", final_tries=3)

    def test_late_release_can_trigger_rebase_after_clear(self):
        self.case("late_release", "rebase", initial_state="wait_on", initial_ready=0,
                  after_clear="session", probes=4, state="session", tries=2,
                  rebase=1, ready=0, wait_error="none")

    def test_release_at_poll_limit_allows_next_pose(self):
        self.case("late_release", "boundary", initial_state="wait_on",
                  probes=302, after_clear="wait_on", state="capture", tries=2,
                  ready=1, wait_error="none")

    def test_late_release_timeout_ends_auth_without_retry(self):
        self.case("late_release", "held", initial_state="wait_on", initial_status=2,
                  probes=301, state="wait_on", tries=2, ready=0, polling=0, poll_id=0,
                  wait_error="timeout")

    def test_release_requirement_survives_cancellation_and_fresh_identify(self):
        self.case("late_release", "cancel", initial_state="wait_on", initial_status=2,
                  aborted=1, probes=5, after_clear="wait_on", state="capture",
                  tries=0, ready=1, wait_error="none")

    def test_transport_recovery_waits_for_real_release_after_rebuilding(self):
        for action in ("recover-held", "recover-probe-error"):
            with self.subTest(action=action):
                self.case("late_release", action, initial_probes=3,
                          initial_state="session", initial_status=2, initial_ready=0,
                          after_clear="wait_on", probes=5, state="capture", tries=2,
                          ready=1, wait_error="none")

    def test_release_timeout_preserves_completed_actions(self):
        for action in ("verify", "identify", "enroll"):
            with self.subTest(action=action):
                self.case("release", action, before=63, complete=1, probes=101,
                          state="done", rebase=0, release_logs=0, error="none")

    def test_release_enroll_can_continue_after_clear(self):
        self.case("release", "enroll", probes=1, state="wait_on", poll_id=42,
                  error="none")

    def test_enroll_late_release_keeps_preceding_views_and_stage(self):
        self.case("late_release", "enroll", initial_state="wait_on", initial_status=2,
                  after_clear="wait_on", probes=5, state="capture", views="kept",
                  stage=19, progress_stage=19, wait_error="none", final_state="done",
                  final_stage=20)

    def test_enroll_late_release_wait_remains_cancellable(self):
        self.case("late_release", "enroll-cancel", initial_state="wait_on",
                  initial_status=2, aborted=1, probes=0, views="kept", stage=19,
                  progress_stage=19, poll_id=0, wait_error="cancelled")

    def test_release_cancellation_stops_before_probing(self):
        self.case("release", "cancel", probes=0, state="wait_off", poll_id=0,
                  release_logs=0, none_reports=1, error="cancelled")


if __name__ == "__main__":
    unittest.main()
