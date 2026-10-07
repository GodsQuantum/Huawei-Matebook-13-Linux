/* SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * GXFP51A0 pre-enumeration recovery.
 *
 * Deep Goodix Milan SPI desynchronisation is not always cleared by a GPIO
 * reset or by closing/reopening /dev/spidev.  Hardware evidence from GXFP5187
 * and GXFP51A0 shows that the spidev kernel binding itself must sometimes be
 * torn down and recreated.  Doing that from a live libfprint FpDevice would
 * create a udev hot-remove underneath the object, so this helper runs as
 * fprintd ExecStartPre, before libfprint enumerates the reader.
 *
 * Only the target child spi-GXFP51A0:00 is touched.  The PXA2xx controller is
 * never unbound and no firmware operation is performed.
 */
#define _GNU_SOURCE
#include "gx51_transport.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <linux/gpio.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define GX_DEV_NAME "spi-GXFP51A0:00"
#define GX_SYS_DEV "/sys/bus/spi/devices/" GX_DEV_NAME
#define GX_SPIDEV_DRIVER "/sys/bus/spi/drivers/spidev"
#define GX_DRIVER_LINK GX_SYS_DEV "/driver"
#define GX_DRIVER_OVERRIDE GX_SYS_DEV "/driver_override"
#define GX_BIND GX_SPIDEV_DRIVER "/bind"
#define GX_UNBIND GX_SPIDEV_DRIVER "/unbind"

static int
sleep_ms (long ms)
{
  struct timespec ts = {
    .tv_sec = ms / 1000,
    .tv_nsec = (ms % 1000) * 1000000L,
  };

  while (nanosleep (&ts, &ts) < 0)
    if (errno != EINTR)
      return -1;
  return 0;
}

static int
write_sysfs (const char *path, const char *value)
{
  int fd;
  size_t len = strlen (value);
  ssize_t n;

  fd = open (path, O_WRONLY | O_CLOEXEC);
  if (fd < 0)
    {
      fprintf (stderr, "GXFP51A0 PRESTART_RECOVERY write-open failed: %s: %s\n",
               path, strerror (errno));
      return -1;
    }

  do
    n = write (fd, value, len);
  while (n < 0 && errno == EINTR);

  if (n != (ssize_t) len)
    {
      int saved = errno;
      close (fd);
      errno = saved;
      fprintf (stderr, "GXFP51A0 PRESTART_RECOVERY write failed: %s: %s\n",
               path, n < 0 ? strerror (errno) : "short write");
      return -1;
    }

  if (close (fd) < 0)
    {
      fprintf (stderr, "GXFP51A0 PRESTART_RECOVERY close failed: %s: %s\n",
               path, strerror (errno));
      return -1;
    }
  return 0;
}

static int
open_int34bb_gpiochip (void)
{
  for (int n = 0; n < 16; n++)
    {
      char path[32];
      struct gpiochip_info info = { 0 };
      int fd;

      if (snprintf (path, sizeof path, "/dev/gpiochip%d", n) < 0)
        continue;

      fd = open (path, O_RDWR | O_CLOEXEC);
      if (fd < 0)
        continue;

      if (ioctl (fd, GPIO_GET_CHIPINFO_IOCTL, &info) == 0 &&
          strncmp (info.label, "INT34BB", 7) == 0 &&
          info.lines > GX51_RESET_LINE)
        return fd;

      close (fd);
    }

  errno = ENODEV;
  return -1;
}

/* rel71.24 experiment: ST411 reset only.
 *
 * Our established GXFP51A0/ST411 polarity is active-HIGH on GPIO264.  Unlike
 * the historical 300/600 reset used by the driver, deep-lock recovery here
 * tests the short-pulse/release semantics measured on working Goodix siblings:
 *
 *   request OUTPUT explicitly inactive LOW
 *   HIGH 10 ms (assert)
 *   LOW       (release)
 *   release the GPIO line request completely
 *   150 ms settle with no request held
 *
 * This helper is pre-enumeration only; the libfprint driver remains unchanged.
 */
static int
reset_gpio264_short_pulse (void)
{
  struct gpio_v2_line_request req = { 0 };
  struct gpio_v2_line_values val = { 0 };
  int chip = open_int34bb_gpiochip ();

  if (chip < 0)
    return -1;

  req.num_lines = 1;
  req.offsets[0] = GX51_RESET_LINE;
  req.config.flags = GPIO_V2_LINE_FLAG_OUTPUT;
  req.config.num_attrs = 1;
  req.config.attrs[0].attr.id = GPIO_V2_LINE_ATTR_ID_OUTPUT_VALUES;
  req.config.attrs[0].attr.values = 0; /* explicit inactive LOW at request */
  req.config.attrs[0].mask = 1;
  strncpy (req.consumer, "goodix51a0-prestart", sizeof req.consumer - 1);

  if (ioctl (chip, GPIO_V2_GET_LINE_IOCTL, &req) < 0 || req.fd < 0)
    {
      int saved = errno;
      close (chip);
      errno = saved;
      return -1;
    }

  val.mask = 1;
  val.bits = 1; /* HIGH: active reset on this ST411 */
  if (ioctl (req.fd, GPIO_V2_LINE_SET_VALUES_IOCTL, &val) < 0)
    goto fail;
  if (sleep_ms (10) < 0)
    goto fail;

  val.bits = 0; /* LOW: MCU runs */
  if (ioctl (req.fd, GPIO_V2_LINE_SET_VALUES_IOCTL, &val) < 0)
    goto fail;

  /* Complete release is part of this experiment. */
  close (req.fd);
  close (chip);

  return sleep_ms (150);

fail:
  {
    int saved = errno;
    val.mask = 1;
    val.bits = 0;
    (void) ioctl (req.fd, GPIO_V2_LINE_SET_VALUES_IOCTL, &val);
    close (req.fd);
    close (chip);
    errno = saved;
    return -1;
  }
}

static int
bound_driver_is_spidev (void)
{
  char buf[PATH_MAX];
  ssize_t n;
  const char *base;

  n = readlink (GX_DRIVER_LINK, buf, sizeof buf - 1);
  if (n < 0)
    {
      if (errno == ENOENT)
        return 0;
      fprintf (stderr, "GXFP51A0 PRESTART_RECOVERY readlink failed: %s\n",
               strerror (errno));
      return -1;
    }

  buf[n] = '\0';
  base = strrchr (buf, '/');
  base = base ? base + 1 : buf;
  return strcmp (base, "spidev") == 0 ? 1 : -2;
}

static int
wait_driver_state (int want_spidev, long timeout_ms)
{
  for (long waited = 0; waited <= timeout_ms; waited += 20)
    {
      int state = bound_driver_is_spidev ();

      if (state < -1)
        return -1;
      if (want_spidev ? state == 1 : state == 0)
        return 0;
      if (state < 0)
        return -1;
      if (sleep_ms (20) < 0)
        return -1;
    }

  errno = ETIMEDOUT;
  return -1;
}

int
main (void)
{
  int state;
  int reset_rc = 0;
  int bind_rc = 0;

  if (access (GX_SYS_DEV, F_OK) < 0)
    {
      if (errno == ENOENT)
        {
          /* fprintd may serve unrelated readers too.  Absence of GXFP51A0 is
           * not an error and must not prevent the generic daemon from starting. */
          puts ("GXFP51A0 PRESTART_RECOVERY=SKIP target-not-present");
          return 0;
        }
      fprintf (stderr, "GXFP51A0 PRESTART_RECOVERY target check failed: %s\n",
               strerror (errno));
      return 1;
    }

  if (access (GX_SPIDEV_DRIVER, F_OK) < 0)
    {
      fprintf (stderr, "GXFP51A0 PRESTART_RECOVERY spidev driver unavailable: %s\n",
               strerror (errno));
      return 1;
    }

  state = bound_driver_is_spidev ();
  if (state == -2)
    {
      fprintf (stderr,
               "GXFP51A0 PRESTART_RECOVERY refusing to unbind non-spidev driver\n");
      return 1;
    }
  if (state < 0)
    return 1;

  if (state == 1)
    {
      puts ("GXFP51A0 PRESTART_RECOVERY step=unbind-spidev");
      if (write_sysfs (GX_UNBIND, GX_DEV_NAME) < 0 ||
          wait_driver_state (0, 1000) < 0)
        {
          fprintf (stderr, "GXFP51A0 PRESTART_RECOVERY unbind failed: %s\n",
                   strerror (errno));
          return 1;
        }
    }
  else
    puts ("GXFP51A0 PRESTART_RECOVERY step=already-unbound");

  /* Deep-lock recovery on the Milan SPI family is materially more reliable
   * when host state is allowed to go fully quiescent after unbind, before the
   * sensor reset is asserted. Keep this delay outside the GPIO helper so the
   * target stays completely detached for the whole interval. */
  puts ("GXFP51A0 PRESTART_RECOVERY step=unbound-quiet-1000");
  if (sleep_ms (1000) < 0)
    {
      fprintf (stderr, "GXFP51A0 PRESTART_RECOVERY pre-reset quiet failed: %s\n",
               strerror (errno));
      return 1;
    }

  /* rel71.24 helper-only experiment: keep the target detached while issuing
   * a short active-HIGH GPIO264 pulse, then release the request completely. */
  puts ("GXFP51A0 PRESTART_RECOVERY step=gpio264-short-pulse-high10-release-settle150");
  if (reset_gpio264_short_pulse () < 0)
    {
      reset_rc = 1;
      fprintf (stderr, "GXFP51A0 PRESTART_RECOVERY GPIO264 reset failed: %s\n",
               strerror (errno));
    }

  /* Deep-lock references use a longer detached settle after the reset.
   * Keep two full seconds before rebinding so both MCU and host-driver state
   * are quiescent before the spidev child is recreated. */
  puts ("GXFP51A0 PRESTART_RECOVERY step=post-reset-quiet-2000");
  if (sleep_ms (2000) < 0)
    {
      reset_rc = 1;
      fprintf (stderr, "GXFP51A0 PRESTART_RECOVERY settle sleep failed: %s\n",
               strerror (errno));
    }

  puts ("GXFP51A0 PRESTART_RECOVERY step=rebind-spidev");
  if (write_sysfs (GX_DRIVER_OVERRIDE, "spidev") < 0)
    bind_rc = 1;

  state = bound_driver_is_spidev ();
  if (state == 0)
    {
      if (write_sysfs (GX_BIND, GX_DEV_NAME) < 0)
        bind_rc = 1;
    }
  else if (state != 1)
    bind_rc = 1;

  if (!bind_rc && wait_driver_state (1, 1500) < 0)
    {
      bind_rc = 1;
      fprintf (stderr, "GXFP51A0 PRESTART_RECOVERY rebind did not settle: %s\n",
               strerror (errno));
    }

  if (!bind_rc)
    {
      puts ("GXFP51A0 PRESTART_RECOVERY step=post-rebind-quiet-1000");
      if (sleep_ms (1000) < 0)
        {
          bind_rc = 1;
          fprintf (stderr,
                   "GXFP51A0 PRESTART_RECOVERY post-rebind quiet failed: %s\n",
                   strerror (errno));
        }
    }

  /* Always try to restore the binding even when GPIO reset failed, but never
   * report success if any required recovery step failed. */
  if (reset_rc || bind_rc)
    {
      fprintf (stderr,
               "GXFP51A0 PRESTART_RECOVERY=FAILED reset=%d rebind=%d\n",
               reset_rc, bind_rc);
      return 1;
    }

  puts ("GXFP51A0 PRESTART_RECOVERY=READY unbind+GPIO264+rebind");
  return 0;
}
