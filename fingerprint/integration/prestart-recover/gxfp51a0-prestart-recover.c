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
#include <stdio.h>
#include <string.h>
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

  /* Exact target-specific reset already validated on ST411/14115.  Keep the
   * device unbound for the whole pulse so host-driver state and MCU state cross
   * the same cold boundary. */
  puts ("GXFP51A0 PRESTART_RECOVERY step=gpio264-reset-300-600");
  if (gx51_reset_gpio264 () < 0)
    {
      reset_rc = 1;
      fprintf (stderr, "GXFP51A0 PRESTART_RECOVERY GPIO264 reset failed: %s\n",
               strerror (errno));
    }

  /* Historical successful GXFP51A0 recovery used a one-second quiet interval
   * before rebinding.  Do not shorten this while the target is still being
   * validated on multiple machines. */
  if (sleep_ms (1000) < 0)
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
