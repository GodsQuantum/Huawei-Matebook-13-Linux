/*
 * Goodix GXFP51A0 SPI (TLS-PSK) driver for libfprint
 *
 * Copyright (C) 2026 Benjamin Allègre (https://github.com/Sigfrodr)
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * Driver candidate for the GXFP51A0 SPI fingerprint sensor used in Huawei
 * MateBook systems. The chain is: split Milan SPI framing, target-specific
 * ChicagoHS configuration, TLS-PSK, and an 80x64 image path (capture
 * integration still under validation).
 * (six bytes carry four interleaved pixels) calibrated by subtracting a
 * background frame.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <https://www.gnu.org/licenses/>.
 */

#define FP_COMPONENT "goodix51a0"
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <errno.h>
#include <glib/gstdio.h>
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <linux/spi/spidev.h>
#include <linux/gpio.h>
#include <openssl/crypto.h>

#include "goodix51a0.h"
#include "goodix_sift.h"
#include "goodix_tls.h"

#include "gx51_transport.h"
#include "gx51_target.h"
#include "gx51_factory_pmk.h"
#include "gx51_image.h"
#include "gx51_capture_recipe.h"

G_STATIC_ASSERT (GOODIX_PSK_LEN == GXFP_FACTORY_PMK_LEN);


/* Frames averaged for background and finger: a noise/latency trade-off. */
#define GOODIX_BG_FRAMES     1
#define GOODIX_FINGER_FRAMES 3
/* Finger detection threshold on the standard deviation of the
 * background-minus-frame difference. */
#define GOODIX_FINGER_STD    150.0
/* Headroom above the ~10.6 kB target image record and the 22 kB regression case. */
#define GOODIX_RX_MAX        24000
#define GXFP_PMK_ACQUIRE_ATTEMPTS 4
#define GX_TARGET_ACK_ATTEMPTS 2
#define GX_TARGET_ACK_IRQ_TIMEOUT_MS 100
#define GX_PMK_CACHE_DIR  "/var/lib/fprint"
#define GX_PMK_CACHE_FILE "/var/lib/fprint/.goodix51a0-pmk"

struct _FpiDeviceGoodix51A0
{
  FpDevice parent;

  int           spi_fd;
  int           irq_fd;
  FpiSsm       *task_ssm;

  /* TLS stack. Note the sensor is the client and we are the server. */
  GxTls        *tls;
  gboolean      tls_up;

  guint8        psk[GOODIX_PSK_LEN];
  gboolean      psk_ready;
  gboolean      psk_from_cache;
  struct gxfp_target_calibration target_cal;
  gboolean      have_target_cal;

  /* Reassembly buffer for TLS records, drained by the BIO recv callback. */
  guint8        tls_rx[GOODIX_RX_MAX];
  int           tls_rxlen, tls_rxpos;

  guint16      *bg_frame;    /* averaged background, NULL until calibrated */
  /* A previously clean, empty-sensor ImageBase may be re-used for one
   * EARLY-FINGER verified capture immediately following short S3.  Never
   * persist it to disk or use it for enrolment/identification bypass. */
  guint16      *s3_saved_bg;
  int           s3_saved_fdt[GXFP_FDT_ZONE_COUNT];
  int           s3_saved_abs;
  gint64        s3_saved_at_boot_us;
  gint64        s3_saved_bg_age_us;

  guint         poll_id;     /* finger-detection timeout source */
  int           poll_count;  /* poll iterations, bounded to avoid hanging */
  GPtrArray    *enroll_feats;/* descriptor sets accumulated during enrolment */
  int           fdt_base[GXFP_FDT_ZONE_COUNT]; /* FDT baseline (finger absent) */
  gboolean      have_fdt;    /* is fdt_base populated? */
  int           fdt_abs;     /* per-unit absolute floor, derived from baseline */
  int           timing_scale;/* session-local protocol-delay multiplier in % */
  int           capture_gap_scale; /* session-local capture-step gap % */
  guint         capture_clean_streak; /* consecutive complete frames without GET_IMAGE retry */
  gboolean      capture_retry_seen; /* current frame needed a GET_IMAGE/TLS retry */
  gboolean      capture_pacing_suppressed; /* lifecycle loss is not pacing evidence */
  gboolean      capture_recovery_pending; /* transport failed; rebuild between presses */
  gboolean      bg_dirty;    /* background taken with a finger down */
  gboolean      production_ready; /* TLS + fresh background + FDT prepared during open */
  gboolean      warm_valid;       /* TLS/background/FDT retained across fp_device close */
  gint64        warm_last_activity_us; /* monotonic time of last validated sensor activity */
  gint64        bg_last_refresh_us; /* monotonic time of last accepted finger-off ImageBase */
  gint64        warm_sleep_delta_us; /* CLOCK_BOOTTIME-MONOTONIC when warm state was armed */
  gboolean      warm_sleep_clock_valid; /* baseline validity; zero is a legitimate pre-first-suspend value */
  gboolean      force_cold_reset; /* suspend/lifecycle invalidation: never reuse stale sensor state */
  gboolean      s3_first_tls_pending; /* use full recovery promptly after a real S3 failure */
  gboolean      s3_rel61_tls_fallback; /* after one detached recovery, allow historic complete TLS retry sequence */
  guint         cold_verify_placement_budget; /* first two auth transactions of this daemon only */
  gint64        s3_presleep_deadline_us; /* QA-only hard deadline for extra presleep image */
};

G_DECLARE_FINAL_TYPE (FpiDeviceGoodix51A0, fpi_device_goodix51a0, FPI,
                      DEVICE_GOODIX51A0, FpDevice)
G_DEFINE_TYPE (FpiDeviceGoodix51A0, fpi_device_goodix51a0, FP_TYPE_DEVICE)

static guint gx_capture_gap_us (FpiDeviceGoodix51A0 *self);
static void gx_capture_transport_desync (FpiDeviceGoodix51A0 *self);
static void gx_capture_pacing_success (FpiDeviceGoodix51A0 *self);
static void gx_protocol_timing_miss (FpiDeviceGoodix51A0 *self,
                                     const gchar          *source);
static gboolean gx_warm_crossed_sleep (FpiDeviceGoodix51A0 *self);
static gboolean gx_warm_validate (FpiDeviceGoodix51A0 *self);
static void gx_warm_abandon (FpiDeviceGoodix51A0 *self);
static gboolean gx_cold_prepare (FpiDeviceGoodix51A0 *self);
static void gx_transport_close (FpiDeviceGoodix51A0 *self);
static gboolean gx_transport_open (FpDevice *dev, GError **error);

/* fpi_device_get_cancellable() requires a live libfprint action.
 * Idle S3 resume prewarm must not call it with ACTION_NONE. */
static GCancellable *
gx_action_cancellable_if_active (FpiDeviceGoodix51A0 *self)
{
  FpDevice *dev = FP_DEVICE (self);
  if (fpi_device_get_current_action (dev) == FPI_DEVICE_ACTION_NONE)
    return NULL;
  return fpi_device_get_cancellable (dev);
}

static const FpIdEntry goodix51a0_id_table[] = {
  { .udev_types = FPI_DEVICE_UDEV_SUBTYPE_SPIDEV, .spi_acpi_id = "GXFP51A0" },
  { .udev_types = 0 }
};

/* Fixed startup commands, byte-for-byte as validated against the sensor. */
static const guint8 GX_AMORCE[] = { 0, 5, 0, 0, 0, 0, 0, 0xA5 };
static const guint8 GX_ENABLE[] = { 0x96, 0x03, 0x00, 0x01, 0x00, 0x10 };
static const guint8 GX_REQTLS[] = { 0xd0, 0x03, 0x00, 0x00, 0x00, 0xd7 };
static const guint8 GX_TLS_OK[] = { 0xd4, 0x03, 0x00, 0x00, 0x00, 0xd3 };

#define GX_TLS_IRQ_POLL_MS 20
#define GX_TLS_IRQ_POLLS 200

static gboolean gx_read_fw_version_once (FpiDeviceGoodix51A0 *self,
                                           gchar *out,
                                           gsize cap);
#ifdef GXFP51A0_DEVELOPER
static void gx_dump_capture (FpiDeviceGoodix51A0 *self,
                             const guint16 *px);
#endif

/* ------------------------------------------------------------------ */
/*  SPI transport: one frame is exactly one transfer                   */
/* ------------------------------------------------------------------ */

/* Exact GXFP51A0 Windows 1.1.141.36 WakeupMCU (gfspi.dll SHA-256
 * 4fc5956220cc7bd86d002437e9cae5508d724763a430e4994ba7ce64144a6d59,
 * function 0x1800413e4): one raw four-byte SpbPeripheralWrite
 * {0x0f,0x00,0x00,0x0e}, then Sleep(5).  This is deliberately NOT a Milan
 * protocol frame and expects no sensor ACK. */
static gboolean
gx_wakeup_mcu (FpiDeviceGoodix51A0 *self)
{
  static const guint8 wake[4] = { 0x0f, 0x00, 0x00, 0x0e };
  struct spi_ioc_transfer xfer = { 0 };

  xfer.tx_buf = (unsigned long) wake;
  xfer.len = sizeof wake;
  if (gx51_spi_message_fd (self->spi_fd, &xfer) < 0)
    {
      fp_warn ("GXFP51A0 AUTH_TRACE WakeupMCU raw SPI write failed");
      return FALSE;
    }

  g_usleep (5000);
  fp_warn ("GXFP51A0 AUTH_TRACE WakeupMCU raw SPI write complete");
  return TRUE;
}

static gboolean
gx_write_frame (FpiDeviceGoodix51A0 *self, guint8 type,
                const guint8 *body, gsize n)
{
  struct gx51_outer hdr;
  struct spi_ioc_transfer xfer = { 0 };

  if (n == 0 || n > 0xffff ||
      gx51_make_outer (type, n, &hdr) < 0)
    return FALSE;

  xfer.tx_buf = (unsigned long) &hdr;
  xfer.len = sizeof hdr;
  if (gx51_spi_message_fd (self->spi_fd, &xfer) < 0)
    return FALSE;

  g_usleep (2000);

  memset (&xfer, 0, sizeof xfer);
  xfer.tx_buf = (unsigned long) body;
  xfer.len = n;
  return gx51_spi_message_fd (self->spi_fd, &xfer) == 0;
}


/* Reads one frame: a 4-byte header then the body. Returns the body length,
 * or -1 on failure. */
static int
gx_read_frame (FpiDeviceGoodix51A0 *self, guint8 *out_type,
               guint8 *rx, gsize rx_cap)
{
  guint8 hdr[4] = { 0 };
  struct spi_ioc_transfer xfer = { 0 };
  guint16 n;

  if (self->irq_fd < 0 ||
      gx51_wait_irq_gpio48 (self->irq_fd, 1200) < 0)
    return -1;

  xfer.rx_buf = (unsigned long) hdr;
  xfer.len = sizeof hdr;
  if (gx51_spi_message_fd (self->spi_fd, &xfer) < 0)
    return -1;

  if (hdr[0] == 0xff && hdr[1] == 0xff &&
      hdr[2] == 0xff && hdr[3] == 0xff)
    return -1;

  if ((guint8) (hdr[0] + hdr[1] + hdr[2]) != hdr[3])
    return -1;

  if (hdr[0] != GOODIX_PKT_PLAIN && hdr[0] != GOODIX_PKT_TLS)
    return -1;

  *out_type = hdr[0];
  n = hdr[1] | (hdr[2] << 8);
  if (n == 0 || n > rx_cap)
    return -1;

  for (gsize off = 0; off < n; )
    {
      gsize chunk = gx51_read_chunk_size ((gsize) n - off);

      memset (&xfer, 0, sizeof xfer);
      xfer.rx_buf = (unsigned long) (rx + off);
      xfer.len = (guint32) chunk;
      if (gx51_spi_message_fd (self->spi_fd, &xfer) < 0)
        return -1;
      off += chunk;
    }

  return n;
}


/* Sends the preamble (A0), then a plaintext command (A0). */
static gboolean
gx_send_plain_raw (FpiDeviceGoodix51A0 *self, const guint8 *body, gsize n)
{
  if (!gx_write_frame (self, GOODIX_PKT_PLAIN, GX_AMORCE, sizeof GX_AMORCE))
    return FALSE;
  g_usleep (8000);
  return gx_write_frame (self, GOODIX_PKT_PLAIN, body, n);
}

/* 2020 MateBook 13 QA only: distinguish delayed FDT-down data from a
 * genuinely missing second reply. The tested production default remains
 * 100ms; allow only exact 200/300ms values to avoid unbounded waits.
 * This never bypasses the mandatory ACK + second FDT report and never
 * permits GET_IMAGE to run with an incomplete transport response. */
#ifndef GX_FDT_DOWN_SECOND_REPLY_TIMEOUT_MS
#define GX_FDT_DOWN_SECOND_REPLY_TIMEOUT_MS 100
#endif
#ifndef GX_DRAIN_IRQ_POLL_MS
#define GX_DRAIN_IRQ_POLL_MS 10
#endif
#define GX_FDT_DOWN_SECOND_REPLY_POLLS (GX_FDT_DOWN_SECOND_REPLY_TIMEOUT_MS / GX_DRAIN_IRQ_POLL_MS)
G_STATIC_ASSERT (GX_FDT_DOWN_SECOND_REPLY_TIMEOUT_MS % GX_DRAIN_IRQ_POLL_MS == 0);
G_STATIC_ASSERT (GX_FDT_DOWN_SECOND_REPLY_POLLS == 10);

static int
gx_fdt_down_second_reply_poll_limit (void)
{
  const gchar *qa_ms = g_getenv ("GXFP_FDT_DOWN_SECOND_REPLY_WINDOW_QA_MS");
  if (g_strcmp0 (qa_ms, "200") == 0)
    return 200 / GX_DRAIN_IRQ_POLL_MS;
  if (g_strcmp0 (qa_ms, "300") == 0)
    return 300 / GX_DRAIN_IRQ_POLL_MS;
  return GX_FDT_DOWN_SECOND_REPLY_POLLS;
}

/* Sends a cleartext command and drains every plain response. A TLS record
 * arriving instead is stashed for the BIO layer to pick up. */
static gboolean
gx_send_plain_drain (FpiDeviceGoodix51A0 *self, const guint8 *body, gsize n,
                     gboolean *out_ack_seen, gboolean *out_tls_seen)
{
/* IRQ-silence polls, 10 ms apart, that count as silence after a response.
 * This is the dominant cost of a capture: each command in the sequence
 * pays it. Cut it too short and frame boundaries desynchronise, leaving the
 * sensor wedged in a state only a hard reset plus an spidev rebind recovers.
 *
 * Measured over 6 to 8 captures per setting:
 *   6 -> 1319 ms, no failures       (original value)
 *   4 -> 1136 ms, no failures       <- kept
 *   3 -> 1062 ms, no failures
 *   2 -> total failure, sensor wedged
 * We keep 4 rather than 3: shipping the value right next to the cliff would
 * leave no margin for load or temperature variation. */
#ifndef GX_DRAIN_SILENCE
#define GX_DRAIN_SILENCE 4
#endif
#ifndef GX_DRAIN_IRQ_POLL_MS
#define GX_DRAIN_IRQ_POLL_MS 10
#endif
/* Windows completes FDT-down asynchronously: first ACK 0x32, then a separate
 * 20-byte FDT/touch report.  The generic post-reply silence budget is only
 * 40 ms, while the already-validated explicit FDT probe allows 100 ms for its
 * second response.  Give FDT-down the same second-response window before
 * advancing to GET_IMAGE. */
#ifndef GX_GET_IMAGE_ATTEMPTS
#define GX_GET_IMAGE_ATTEMPTS 2
#endif
/*
 * Exact Windows GXFP51A0/ST411/14115 WBDI trace calls image SetMode (0x20)
 * with a 1000 ms ACK timeout.  Do not declare a command swallowed after the
 * old ~120 ms no-evidence window: on reference MateBook 13 that caused nearly every first
 * biometric GET_IMAGE to be replayed.  Replaying remains allowed exactly once,
 * but only after the full Windows-equivalent ACK window produced neither a
 * clear ACK nor a TLS image record.
 */
#ifndef GX_GET_IMAGE_ACK_TIMEOUT_MS
#define GX_GET_IMAGE_ACK_TIMEOUT_MS 1000
#endif
#define GX_GET_IMAGE_NO_REPLY_POLLS   (GX_GET_IMAGE_ACK_TIMEOUT_MS / GX_DRAIN_IRQ_POLL_MS)
G_STATIC_ASSERT (GX_GET_IMAGE_ACK_TIMEOUT_MS % GX_DRAIN_IRQ_POLL_MS == 0);
G_STATIC_ASSERT (GX_GET_IMAGE_NO_REPLY_POLLS == 100);

  static guint8 scratch[GOODIX_RX_MAX];
  int expected_plain_replies = 0;
  int attempts = body[0] == 0x20u ? GX_GET_IMAGE_ATTEMPTS : 1;
  int attempt;

  /* Reply counts observed stable across the GXFP51A0 capture path and matching
   * the command semantics. Commands not listed here retain silence-based
   * draining. GET_IMAGE remains special because its ACK/TLS ordering varies. */
  switch (body[0])
    {
    /* Windows ST411/14115 emits two plain responses for FDT-down before
     * image capture: the 0x32 ACK, then the 20-byte FDT data/touch report.
     * Leaving the second response pending contaminates the IRQ/response queue
     * immediately before GET_IMAGE and makes the first 0x20 command appear
     * swallowed. Drain both before advancing the capture recipe. */
    case 0x32u: expected_plain_replies = 2; break; /* FDT-down ACK + data */
    case 0x36u: expected_plain_replies = 2; break; /* FDT manual/data */
    case 0x50u: expected_plain_replies = 2; break; /* NAV */
    case 0x80u: expected_plain_replies = 1; break; /* register write */
    case 0x82u: expected_plain_replies = 2; break; /* register read */
    case 0xaeu: expected_plain_replies = 2; break; /* MCU state */
    default: break;
    }

  if (out_ack_seen)
    *out_ack_seen = FALSE;
  if (out_tls_seen)
    *out_tls_seen = FALSE;

  for (attempt = 1; attempt <= attempts; attempt++)
    {
      if (self->s3_presleep_deadline_us > 0 &&
          g_get_monotonic_time () >= self->s3_presleep_deadline_us)
        return FALSE;
      guint8 ty;
      gboolean ack_seen = FALSE;
      gboolean tls_seen = FALSE;
      int no_reply_miss_limit =
        body[0] == 0x20u ? GX_GET_IMAGE_NO_REPLY_POLLS :
        body[0] == 0xaeu ? 12 : 25;
      int r, got = 0, i, misses = 0;
      int post_reply_miss_limit =
        body[0] == 0x32u ? gx_fdt_down_second_reply_poll_limit () : GX_DRAIN_SILENCE;
      gint64 first_reply_us = 0;
      gint64 second_reply_us = 0;

      /* Capture commands are sent directly. The recipe carries explicit NOPs
       * at the positions observed on GXFP51A0; do not prepend one here. */
      if (!gx_write_frame (self, GOODIX_PKT_PLAIN, body, n))
        return FALSE;

      /* Windows sends capture-group NOP without waiting for an ACK.
       * Waiting for silence here costs ~250 ms because NOP intentionally has
       * no response.  The outer capture recipe still keeps its conservative
       * inter-command gap, so this removes only a known-empty drain. */
      if (body[0] == 0x00u)
        {
          fp_dbg ("drain cmd=00: no ACK expected; skip silence wait");
          return TRUE;
        }

      g_usleep (15000);

      for (i = 0; i < 120; i++)
        {
          if (self->s3_presleep_deadline_us > 0 &&
              g_get_monotonic_time () >= self->s3_presleep_deadline_us)
            return FALSE;
          errno = 0;
          if (gx51_wait_irq_gpio48 (self->irq_fd, GX_DRAIN_IRQ_POLL_MS) < 0)
            {
              if (errno != ETIMEDOUT)
                return FALSE;

              misses++;
              if (got && misses >= post_reply_miss_limit)
                break;
              if (!got && misses >= no_reply_miss_limit)
                break;
              continue;
            }

          r = gx_read_frame (self, &ty, scratch, sizeof scratch);
          if (r > 0 && ty == GOODIX_PKT_PLAIN)
            {
              guint8 ack_status = 0;

              got++;
              if (body[0] == 0x32u && got == 1)
                first_reply_us = g_get_monotonic_time ();
              if (body[0] == 0x32u && got == 2)
                second_reply_us = g_get_monotonic_time ();
              misses = 0;
              if (gxfp_parse_ack (scratch, r, body[0], &ack_status) &&
                  gxfp_ack_status_success (ack_status))
                ack_seen = TRUE;
              if (expected_plain_replies > 0 && got >= expected_plain_replies)
                {
                  fp_dbg ("drain cmd=%02x: expected %d replies received; finish",
                          body[0], expected_plain_replies);
                  break;
                }
              g_usleep (5000);
              continue;
            }
          if (r > 0 && ty == GOODIX_PKT_TLS)
            {
              memcpy (self->tls_rx, scratch, r);
              self->tls_rxlen = r;
              self->tls_rxpos = 0;
              tls_seen = TRUE;
              fp_dbg ("drain cmd=%02x -> stashed TLS record, %d bytes",
                      body[0], r);
              break;
            }

          misses++;
          if (got && misses >= post_reply_miss_limit)
            break;
          if (!got && misses >= no_reply_miss_limit)
            break;
        }

      fp_dbg ("drain cmd=%02x: %d cleartext reply/replies ack=%d tls=%d attempt=%d/%d",
              body[0], got, ack_seen, tls_seen, attempt, attempts);
      if (body[0] == 0x32u)
        fp_info ("GXFP51A0 FDT_DOWN_DRAIN replies=%d expected=%d ack=%d "
                 "second_window_ms=%d second_reply_delta_ms=%ld",
                 got, expected_plain_replies, ack_seen,
                 post_reply_miss_limit * GX_DRAIN_IRQ_POLL_MS,
                 first_reply_us > 0
                 ? (long) (((second_reply_us > 0 ? second_reply_us :
                               g_get_monotonic_time ()) - first_reply_us) / 1000)
                 : -1L);

      if (out_ack_seen && ack_seen)
        *out_ack_seen = TRUE;
      if (out_tls_seen && tls_seen)
        *out_tls_seen = TRUE;

      /* Windows returns a 0x32 ACK *and* a separate finger/FDT report
       * before the following GET_IMAGE.  Historically this drain accepted
       * an incomplete FDT-down as success, then sent GET_IMAGE into a sensor
       * that might not have armed a frame, reproducing the post-S3 ACK with
       * no subsequent TLS image.  Opt-in: require both plain replies and
       * a validated ACK.  An incomplete 0x32 may already have executed:
       * NEVER replay it or the next image request; recover the whole
       * transport with the existing bounded strict TLS path. */
      if (body[0] == 0x32u &&
          g_strcmp0 (g_getenv ("GXFP_FDT_DOWN_PROOF_QA"), "1") == 0 &&
          (got < expected_plain_replies || !ack_seen))
        {
          fp_warn ("GXFP51A0 FDT_DOWN_PROOF incomplete before GET_IMAGE: "
                   "replies=%d expected=%d ack=%d; bounded session recovery",
                   got, expected_plain_replies, ack_seen ? 1 : 0);
          gx_capture_transport_desync (self);
          return FALSE;
        }

      if (body[0] != 0x20u)
        return TRUE;

      /* Windows and the validated Linux background path both receive a
       * cleartext ACK for GET_IMAGE before the oversized TLS record. If neither
       * ACK nor TLS arrived, the command was swallowed. Replaying the identical
       * GET_IMAGE once is safe because FDT remains armed. Never replay once a
       * TLS record is already present: that would advance the client record
       * stream twice and desynchronise AES-GCM sequence numbers. */
      if (tls_seen || ack_seen)
        return TRUE;

      if (attempt < attempts)
        {
          self->capture_retry_seen = TRUE;
          fp_warn ("GXFP51A0 GET_IMAGE had no ACK/TLS after %d ms ACK window; "
                   "retrying attempt=%d/%d",
                   GX_GET_IMAGE_ACK_TIMEOUT_MS, attempt + 1, attempts);
          g_usleep (8000 * self->timing_scale / 100);
        }
    }

  fp_warn ("GXFP51A0 GET_IMAGE failed: no ACK/TLS after %d attempts",
           attempts);
  gx_capture_transport_desync (self);
  return FALSE;
}

/* ------------------------------------------------------------------ */
/*  Transport glue: TLS records travel inside 0xB0 SPI frames          */
/* ------------------------------------------------------------------ */

static int
gx_bio_send (gpointer ctx, const guint8 *b, gsize l)
{
  FpiDeviceGoodix51A0 *self = ctx;

  gsize off = 0;

  /* One TLS record per 0xB0 frame. OpenSSL batches several records into a
   * single write — the ServerHello group comes out as one 121-byte call — but
   * the sensor expects exactly one record per frame and silently ignores a
   * frame carrying two. Split on the record headers rather than forwarding the
   * buffer as it arrives. */
  while (off + 5 <= l)
    {
      gsize rec = 5 + (((gsize) b[off + 3] << 8) | b[off + 4]);

      if (off + rec > l)
        break;                        /* partial record: should not happen */
      if (!gx_write_frame (self, GOODIX_PKT_TLS, b + off, rec))
        return -1;
      off += rec;
    }
  return (int) l;
}

static int
gx_bio_recv (gpointer ctx, guint8 *b, gsize l)
{
  FpiDeviceGoodix51A0 *self = ctx;
  int avail, take, i;
  guint8 ty;

  if (self->tls_rxpos >= self->tls_rxlen)
    {
      /* Buffer empty: pull fresh TLS frames, skipping cleartext ones. The
       * sensor takes a while to produce the image, hence the generous poll. */
      int r = -1;
      self->tls_rxlen = self->tls_rxpos = 0;
      for (i = 0; i < GX_TLS_IRQ_POLLS; i++)
        {
          /* Do not call gx_read_frame() while the line is quiet: its generic
           * 1200 ms wait would turn this intended 20 ms poll loop into a
           * multi-minute block.  TLS handshake records observed on reference MateBook
           * normally arrive in ~10-20 ms, while 4 s remains generous for a
           * genuinely delayed record. */
          errno = 0;
          if (gx51_wait_irq_gpio48 (self->irq_fd, GX_TLS_IRQ_POLL_MS) < 0)
            {
              if (errno != ETIMEDOUT)
                return -1;
              continue;
            }

          r = gx_read_frame (self, &ty, self->tls_rx, GOODIX_RX_MAX);
          if (r > 0 && ty == GOODIX_PKT_TLS)
            {
              fp_dbg ("bio_recv: TLS frame %d bytes (attempt %d)", r, i);
              break;
            }
          r = -1;
        }
      if (r <= 0)
        {
          fp_dbg ("bio_recv: no TLS frame (timed out)");
          return -1;
        }
      self->tls_rxlen = r;
      self->tls_rxpos = 0;
    }
  avail = self->tls_rxlen - self->tls_rxpos;
  take = (int) l < avail ? (int) l : avail;
  memcpy (b, self->tls_rx + self->tls_rxpos, take);
  self->tls_rxpos += take;
  return take;
}

/* ------------------------------------------------------------------ */
/*  Image decoding: six bytes carry four pixels, in interleaved order  */
/* ------------------------------------------------------------------ */


/* ------------------------------------------------------------------ */
/*  Init, handshake and frame capture                                  */
/* ------------------------------------------------------------------ */

/* Hardware reset. GPIO264 is active-HIGH reset on GXFP51A0: HIGH holds the
 * MCU in reset and LOW lets it run. First-contact hardware confirmation uses
 * HIGH for 300 ms, then LOW with a 600 ms settle; final state remains LOW. */
static void
gx_gpio_reset (FpiDeviceGoodix51A0 *self)
{
  /* A cold reset is most reliable while no SPI file descriptor is open.
   * rel71.16 proved that opening spidev first and only then toggling GPIO264
   * can leave ST411 in a half-open TLS state across fprintd restarts. */
  fp_warn ("GXFP51A0 RESET_TRACE GPIO264 300/600 boundary (spi_fd=%d)",
           self ? self->spi_fd : -1);
  if (gx51_reset_gpio264 () < 0)
    fp_warn ("GXFP51A0: reviewed GPIO264 reset failed");
}




/* A real S3 transition is different from a protocol-defined Windows reset.
 * Use rel71.24's short MCU pulse only while transport is ALREADY detached.
 * Do not reintroduce rel71.21's blanket SPI close/reset/reopen changes. */
static void
gx_reset_detached_s3_boundary (FpiDeviceGoodix51A0 *self)
{
  if (self->s3_first_tls_pending && !self->s3_rel61_tls_fallback &&
      self->spi_fd < 0 && self->irq_fd < 0)
    {
      fp_warn ("GXFP51A0 NATIVE_S3_SHORT_RESET detached GPIO264 HIGH10/LOW150");
      if (gx51_reset_gpio264_short () == 0)
        return;
      fp_warn ("GXFP51A0 NATIVE_S3_SHORT_RESET failed; falling back to reviewed long reset");
    }

  gx_gpio_reset (self);
}

static gboolean
gx_target_read_body (FpiDeviceGoodix51A0 *self,
                     guint8                 *body,
                     gsize                   cap,
                     int                    *out_len)
{
  guint8 type = 0;
  int n = gx_read_frame (self, &type, body, cap);

  if (n <= 0 || type != GOODIX_PKT_PLAIN)
    return FALSE;
  if (out_len)
    *out_len = n;
  return TRUE;
}

static gboolean
gx_target_read_response (FpiDeviceGoodix51A0 *self,
                         guint8               *body,
                         gsize                 cap,
                         int                  *out_len)
{
  /* Proven on reference MateBook: response-bearing commands need the ACK IRQ cycle to
   * deassert before the response is armed. The validated probes use 8 ms. */
  g_usleep (8000 * self->timing_scale / 100);
  return gx_target_read_body (self, body, cap, out_len);
}

static gboolean
gx_target_send_ack (FpiDeviceGoodix51A0       *self,
                    const struct gxfp_target_packet *packet,
                    guint8                           command,
                    guint8                          *status)
{
  guint8 rx[128];
  guint8 type = 0;
  guint8 ack_status = 0;
  int attempt;

  for (attempt = 1; attempt <= GX_TARGET_ACK_ATTEMPTS; attempt++)
    {
      int n;

      /* GPIO48 is level-high.  Never start a new command while a previous
       * response still owns the line. */
      if (gx51_wait_irq_gpio48_low (self->irq_fd, 50) < 0)
        {
          fp_warn ("GXFP51A0 target ACK diagnostic: pre-write-irq-high cmd=0x%02x",
                   command);
          return FALSE;
        }

      if (!gx_write_frame (self, GOODIX_PKT_PLAIN,
                           packet->inner, packet->inner_len))
        {
          fp_warn ("GXFP51A0 target ACK diagnostic: write-failed cmd=0x%02x",
                   command);
          return FALSE;
        }

      /* Distinguish a completely swallowed command (no IRQ edge/level at all)
       * from a protocol error.  Only the former is safe to retry: register
       * writes and idle/config commands are idempotent when replayed with the
       * same bytes.  If any packet arrives, parsing/status errors are final. */
      errno = 0;
      if (gx51_wait_irq_gpio48 (self->irq_fd, GX_TARGET_ACK_IRQ_TIMEOUT_MS) < 0)
        {
          if (errno == ETIMEDOUT && attempt < GX_TARGET_ACK_ATTEMPTS)
            {
              fp_warn ("GXFP51A0 target ACK diagnostic: no-irq retry cmd=0x%02x attempt=%d/%d",
                       command, attempt, GX_TARGET_ACK_ATTEMPTS);
              gx_protocol_timing_miss (self, "target-ack");
              g_usleep (8000 * self->timing_scale / 100);
              continue;
            }

          fp_warn ("GXFP51A0 target ACK diagnostic: ack-read-failed cmd=0x%02x",
                   command);
          return FALSE;
        }

      n = gx_read_frame (self, &type, rx, sizeof rx);
      if (n <= 0)
        {
          fp_warn ("GXFP51A0 target ACK diagnostic: ack-read-failed cmd=0x%02x",
                   command);
          return FALSE;
        }
      if (type != GOODIX_PKT_PLAIN)
        {
          fp_warn ("GXFP51A0 target ACK diagnostic: ack-type-failed cmd=0x%02x type=0x%02x",
                   command, type);
          return FALSE;
        }
      if (!gxfp_parse_ack (rx, n, command, &ack_status))
        {
          fp_warn ("GXFP51A0 target ACK diagnostic: ack-parse-failed cmd=0x%02x len=%d first=0x%02x",
                   command, n, n > 0 ? rx[0] : 0);
          return FALSE;
        }
      if (!gxfp_ack_status_success (ack_status))
        {
          fp_warn ("GXFP51A0 target ACK diagnostic: status-failed cmd=0x%02x status=0x%02x",
                   command, ack_status);
          return FALSE;
        }

      if (status)
        *status = ack_status;
      return TRUE;
    }

  return FALSE;
}

static bool
gx_factory_staging_read_cb (void *user, uint32_t selector,
                            uint32_t request_len, uint8_t *out,
                            size_t out_cap, size_t *out_len)
{
  FpiDeviceGoodix51A0 *self = user;
  struct gxfp_target_packet packet;
  guint8 rx[GXFP_MEM_READ_MAX + 16];
  guint8 discard[GXFP_MEM_READ_MAX];
  guint8 type = 0, status = 0;
  int n;

  if (!out || !out_len || request_len != GXFP_FACTORY_BODY_LEN ||
      out_cap < GXFP_FACTORY_STAGING_LEN ||
      !gxfp_build_mem_read (selector, request_len, &packet) ||
      !gx_write_frame (self, GOODIX_PKT_PLAIN,
                       packet.inner, packet.inner_len))
    return false;

  n = gx_read_frame (self, &type, rx, sizeof rx);
  if (n <= 0 || type != GOODIX_PKT_PLAIN ||
      !gxfp_parse_ack (rx, n, 0xf2, &status) ||
      !gxfp_ack_status_success (status))
    return false;

  for (unsigned int i = 0; i < 3; i++)
    {
      enum gxfp_mem_read_result result;

      g_usleep ((i == 0 ? 8000 : 5000) * self->timing_scale / 100);
      n = gx_read_frame (self, &type, rx, sizeof rx);
      if (n <= 0 || type != GOODIX_PKT_PLAIN)
        return false;
      if (gxfp_14115_parse_rejected_staging_response (
            rx, n, selector, request_len, out, out_cap, out_len))
        return *out_len == GXFP_FACTORY_STAGING_LEN;

      result = gxfp_classify_mem_read_response (
        rx, n, selector, request_len, discard);
      if (result != GXFP_MEM_READ_ECHO_ONLY)
        return false;
    }
  return false;
}

static bool
gx_factory_e4_sanity (FpiDeviceGoodix51A0 *self)
{
  struct gxfp_target_packet packet;
  guint8 rx[128], value[32];
  uint32_t dtype = 0;
  int n = 0;
  bool ok;

  ok = gxfp_build_factory_hash_read (&packet) &&
       gx_target_send_ack (self, &packet, 0xe4, NULL) &&
       gx_target_read_response (self, rx, sizeof rx, &n) &&
       gxfp_parse_factory_hash_response (rx, n, &dtype, value) &&
       dtype == 0x0000aaaau;
  OPENSSL_cleanse (value, sizeof value);
  return ok;
}

static void
gx_pmk_clear (FpiDeviceGoodix51A0 *self)
{
  OPENSSL_cleanse (self->psk, sizeof self->psk);
  self->psk_ready = FALSE;
  self->psk_from_cache = FALSE;
}


static gboolean
gx_pmk_cache_load (FpiDeviceGoodix51A0 *self)
{
  struct stat st;
  g_autofree gchar *data = NULL;
  gsize len = 0;
  g_autoptr(GError) error = NULL;

  if (g_stat (GX_PMK_CACHE_FILE, &st) != 0)
    return FALSE;

  if (!S_ISREG (st.st_mode) ||
      st.st_uid != geteuid () ||
      (st.st_mode & 0077) != 0)
    {
      fp_warn ("GXFP51A0 PMK cache rejected: owner/permissions are unsafe");
      return FALSE;
    }

  if (!g_file_get_contents (GX_PMK_CACHE_FILE, &data, &len, &error))
    {
      fp_warn ("GXFP51A0 PMK cache read failed: %s", error->message);
      return FALSE;
    }

  if (len != GOODIX_PSK_LEN)
    {
      fp_warn ("GXFP51A0 PMK cache rejected: invalid length");
      OPENSSL_cleanse (data, len);
      return FALSE;
    }

  memcpy (self->psk, data, GOODIX_PSK_LEN);
  OPENSSL_cleanse (data, len);
  self->psk_ready = TRUE;
  self->psk_from_cache = TRUE;
  fp_info ("GXFP51A0 PMK cache candidate loaded; TLS validation pending");
  return TRUE;
}


static gboolean
gx_pmk_cache_save (FpiDeviceGoodix51A0 *self)
{
  g_autoptr(GError) error = NULL;

  if (!self->psk_ready || !self->tls_up)
    return FALSE;

  if (!g_file_test (GX_PMK_CACHE_DIR, G_FILE_TEST_IS_DIR) &&
      g_mkdir_with_parents (GX_PMK_CACHE_DIR, 0700) != 0)
    {
      fp_warn ("GXFP51A0 PMK cache directory creation failed: %s",
               g_strerror (errno));
      return FALSE;
    }

  if (!g_file_set_contents_full (
        GX_PMK_CACHE_FILE,
        (const gchar *) self->psk,
        GOODIX_PSK_LEN,
        G_FILE_SET_CONTENTS_CONSISTENT | G_FILE_SET_CONTENTS_DURABLE,
        0600,
        &error))
    {
      fp_warn ("GXFP51A0 PMK cache save failed: %s", error->message);
      return FALSE;
    }

  if (g_chmod (GX_PMK_CACHE_FILE, 0600) != 0)
    {
      fp_warn ("GXFP51A0 PMK cache chmod failed: %s", g_strerror (errno));
      (void) g_unlink (GX_PMK_CACHE_FILE);
      return FALSE;
    }

  fp_info ("GXFP51A0 PMK cache saved after live TLS validation");
  return TRUE;
}


static gboolean
gx_factory_load_staging_pmk (FpiDeviceGoodix51A0 *self)
{
  guint8 pmk[GXFP_FACTORY_PMK_LEN];
  gboolean ok = FALSE;

  /* On 14115 the rejected F2 path can expose one boot-staged factory record.
   * A second selector is not required at runtime: the first-byte brute force
   * must produce exactly one marker=0x000d,len=0x30 candidate, and that
   * candidate is then validated by the real TLS handshake.  Keeping the
   * two-selector consensus code as offline research avoids coupling runtime
   * success to the IRQ/reply behavior of a second rejected F2 command. */
  if (!gxfp_factory_load_pmk_from_single_staging (
        gx_factory_staging_read_cb, self, pmk))
    {
      fp_warn ("GXFP51A0 PMK single-staging candidate invalid for this boot session");
      goto out;
    }

  /* Run E4 only after extraction: any earlier command can reuse staging. */
  if (!gx_factory_e4_sanity (self))
    {
      fp_warn ("GXFP51A0 PMK candidate recovered but E4 sanity failed");
      goto out;
    }

  memcpy (self->psk, pmk, sizeof pmk);
  self->psk_ready = TRUE;
  self->psk_from_cache = FALSE;
  fp_info ("GXFP51A0 PMK recovered from single rejected-read boot staging; TLS validation pending");
  ok = TRUE;

out:
  if (!ok)
    {
      gx_pmk_clear (self);
    }
  OPENSSL_cleanse (pmk, sizeof pmk);
  return ok;
}


static gboolean
gx_factory_acquire_staging_pmk (FpiDeviceGoodix51A0 *self, gboolean allow_cache)
{
  int attempt;

  gboolean stage_qa =
    g_strcmp0 (g_getenv ("GXFP_RESUME_STAGE_QA"), "1") == 0;
  if (allow_cache && gx_pmk_cache_load (self))
    {
      if (stage_qa)
        fp_warn ("GXFP51A0 RESUME_STAGE_QA stage=pmk_cache hit=1");
      return TRUE;
    }
  if (stage_qa)
    fp_warn ("GXFP51A0 RESUME_STAGE_QA stage=pmk_cache hit=0 "
             "cache_allowed=%d", allow_cache ? 1 : 0);

  for (attempt = 1; attempt <= GXFP_PMK_ACQUIRE_ATTEMPTS; attempt++)
    {
      if (stage_qa)
        fp_warn ("GXFP51A0 RESUME_STAGE_QA reset_reason=pmk_staging "
                 "attempt=%d", attempt);
      gx_gpio_reset (self);
      fp_info ("GXFP51A0 PMK staging attempt %d/%d after hardware reset",
               attempt, GXFP_PMK_ACQUIRE_ATTEMPTS);

      if (gx_factory_load_staging_pmk (self))
        {
          if (attempt > 1)
            fp_info ("GXFP51A0 PMK staging recovered on attempt %d", attempt);
          return TRUE;
        }

      if (attempt < GXFP_PMK_ACQUIRE_ATTEMPTS)
        g_usleep (50000);
    }

  fp_warn ("GXFP51A0 PMK staging acquisition exhausted after %d boot sessions",
           GXFP_PMK_ACQUIRE_ATTEMPTS);
  return FALSE;
}

static gboolean
gx_read_fw_version_stage2e (FpiDeviceGoodix51A0 *self, gchar *out, gsize cap)
{
  static const guint8 a8[] = {
    GOODIX_CMD_FW_VERSION, 0x03, 0x00, 0x00, 0x00, 0xFF
  };
  guint8 rx[128], type = 0, status = 0;
  int n = 0;

  /* Exact post-reset boundary proven by reference MateBook Stage2E:
   * A8 -> ACK -> 8 ms -> A8 response.  No preceding NOP here. */
  if (!gx_write_frame (self, GOODIX_PKT_PLAIN, a8, sizeof a8))
    {
      fp_warn ("GXFP51A0 Stage2E A8 write failed");
      return FALSE;
    }

  n = gx_read_frame (self, &type, rx, sizeof rx);
  if (n <= 0 || type != GOODIX_PKT_PLAIN ||
      !gxfp_parse_ack (rx, n, GOODIX_CMD_FW_VERSION, &status) ||
      !gxfp_ack_status_success (status))
    {
      fp_warn ("GXFP51A0 Stage2E A8 ACK failed");
      return FALSE;
    }
  fp_info ("GXFP51A0 Stage2E A8 ACK accepted");

  g_usleep (8000 * self->timing_scale / 100);

  n = gx_read_frame (self, &type, rx, sizeof rx);
  if (n <= 3 || type != GOODIX_PKT_PLAIN ||
      rx[0] != GOODIX_CMD_FW_VERSION)
    {
      fp_warn ("GXFP51A0 Stage2E A8 response failed");
      return FALSE;
    }

  g_strlcpy (out, (const gchar *) rx + 3,
             MIN ((gsize) (n - 3) + 1, cap));
  fp_info ("GXFP51A0 Stage2E A8 response accepted");
  return TRUE;
}


static gboolean
gx_target_soft_reset (FpiDeviceGoodix51A0 *self)
{
  struct gxfp_target_packet packet;
  guint8 rx[128];
  uint32_t code = 0;
  int n = 0;

  if (!gxfp_build_soft_reset (&packet))
    {
      fp_warn ("GXFP51A0 A2 diagnostic: build-failed");
      return FALSE;
    }
  if (!gx_target_send_ack (self, &packet, 0xa2, NULL))
    {
      fp_warn ("GXFP51A0 A2 diagnostic: ack-failed");
      return FALSE;
    }
  fp_info ("GXFP51A0 A2 ACK accepted");

  if (!gx_target_read_response (self, rx, sizeof rx, &n))
    {
      fp_warn ("GXFP51A0 A2 diagnostic: response-read-failed");
      return FALSE;
    }
  if (!gxfp_parse_soft_reset_response (rx, n, &code))
    {
      fp_warn ("GXFP51A0 A2 diagnostic: response-parse-failed len=%d", n);
      return FALSE;
    }
  if (code != 0x010008u)
    {
      fp_warn ("GXFP51A0 A2 diagnostic: unexpected-code=0x%06x", code);
      return FALSE;
    }

  fp_info ("GXFP51A0 A2 reset code accepted: 0x%06x", code);
  return TRUE;
}

static gboolean
gx_target_configure (FpiDeviceGoodix51A0 *self)
{
  struct gxfp_target_packet packet;
  struct gxfp_target_calibration cal;
  guint8 rx[512];
  guint8 otp[64];
  guint8 config[GXFP_CONFIG_LEN];
  guint8 status = 0;
  uint16_t chip_id = 0;
  int n = 0;

  if (!gx_target_soft_reset (self))
    {
      fp_warn ("GXFP51A0 target init failed: soft-reset-1");
      return FALSE;
    }

  if (!gxfp_build_chip_id (&packet) ||
      !gx_target_send_ack (self, &packet, 0x82, NULL) ||
      !gx_target_read_response (self, rx, sizeof rx, &n) ||
      !gxfp_parse_chip_id_response (rx, n, &chip_id) ||
      chip_id != 0x2504u)
    {
      fp_warn ("GXFP51A0 target init failed: chip-id");
      return FALSE;
    }
  fp_info ("GXFP51A0 chip-id accepted: 0x%04x", chip_id);

  if (!gxfp_build_read_otp (&packet))
    {
      fp_warn ("GXFP51A0 target init failed: otp-build");
      return FALSE;
    }
  if (!gx_target_send_ack (self, &packet, 0xa6, NULL))
    {
      fp_warn ("GXFP51A0 target init failed: otp-ack");
      return FALSE;
    }
  fp_info ("GXFP51A0 OTP ACK accepted");
  if (!gx_target_read_response (self, rx, sizeof rx, &n))
    {
      fp_warn ("GXFP51A0 target init failed: otp-response-read");
      return FALSE;
    }
  if (!gxfp_parse_otp_response (rx, n, otp))
    {
      fp_warn ("GXFP51A0 target init failed: otp-response-parse len=%d", n);
      return FALSE;
    }
  if (!gxfp_derive_calibration (otp, &cal))
    {
      fp_warn ("GXFP51A0 target init failed: otp-validation");
      return FALSE;
    }
  fp_info ("GXFP51A0 OTP validated: len=%d tcode=%u fdt_delta=%u",
           n, cal.tcode, cal.fdt_delta);

  memcpy (config, GXFP_TARGET_BASE_CONFIG, sizeof config);
  if (!gxfp_patch_config (config, &cal) ||
      !gxfp_config_checksum_valid (config))
    {
      fp_warn ("GXFP51A0 target init failed: config-patch");
      return FALSE;
    }

  if (!gx_target_soft_reset (self))
    {
      fp_warn ("GXFP51A0 target init failed: soft-reset-2");
      return FALSE;
    }

  if (!gxfp_build_idle (&packet) ||
      !gx_target_send_ack (self, &packet, 0x70, &status))
    {
      fp_warn ("GXFP51A0 target init failed: idle");
      return FALSE;
    }

  if (!gxfp_build_reg_write (0x0220u, cal.dac_main, &packet) ||
      !gx_target_send_ack (self, &packet, 0x80, NULL))
    {
      fp_warn ("GXFP51A0 target init failed: dac-main");
      return FALSE;
    }
  if (!gxfp_build_reg_write (0x0236u, cal.dac1, &packet) ||
      !gx_target_send_ack (self, &packet, 0x80, NULL))
    {
      fp_warn ("GXFP51A0 target init failed: dac1");
      return FALSE;
    }
  if (!gxfp_build_reg_write (0x0238u, cal.dac2, &packet) ||
      !gx_target_send_ack (self, &packet, 0x80, NULL))
    {
      fp_warn ("GXFP51A0 target init failed: dac2");
      return FALSE;
    }
  if (!gxfp_build_reg_write (0x023au, cal.dac3, &packet) ||
      !gx_target_send_ack (self, &packet, 0x80, NULL))
    {
      fp_warn ("GXFP51A0 target init failed: dac3");
      return FALSE;
    }

  if (!gxfp_build_upload_config (config, &packet) ||
      !gx_target_send_ack (self, &packet, 0x90, NULL))
    {
      fp_warn ("GXFP51A0 target init failed: config-ack");
      return FALSE;
    }
  if (!gx_target_read_response (self, rx, sizeof rx, &n) ||
      !gxfp_parse_config_response (rx, n, &status) || status != 0x01u)
    {
      fp_warn ("GXFP51A0 target init failed: config-response");
      return FALSE;
    }

  self->target_cal = cal;
  self->have_target_cal = TRUE;
  fp_info ("GXFP51A0 target config accepted: chip=0x%04x tcode=%u "
           "fdt_delta=%u dac=0x%03x/%02x/%02x/%02x",
           chip_id, cal.tcode, cal.fdt_delta, cal.dac_main,
           cal.dac1, cal.dac2, cal.dac3);
  return TRUE;
}


/* reference MateBook now validates the 14115 rejected-read staging provider. Genuine
 * private-flash/RAM F2 remains closed; the reset is required so extraction
 * happens before another command can reuse the boot staging area. */
static gboolean
gx_upload_config_and_reqtls (FpiDeviceGoodix51A0 *self, gboolean allow_cache)
{
  guint8 ack[128], status = 0, type = 0;
  int n;

  if (!self->psk_ready)
    {
      gchar fw[64] = { 0 };

      if (!gx_factory_acquire_staging_pmk (self, allow_cache))
        return FALSE;

      /* The reviewed full MCU reset is essential AFTER actual F2/E4
       * factory boot-staging extraction: its RAM state is transient and
       * must not reach target initialization.
       *
       * By contrast, an already validated 48-byte PMK read from the secure,
       * owner-only local CACHE did NOT send any staging command to the MCU.
       * In that case the current native S3/S4 initial reset, DriverState,
       * FW A8 and bounded TLS preparation have already established the
       * fresh boundary. Resetting the MCU a SECOND time solely because the
       * host loaded a file added ~900ms and another A8 cycle on real S4.
       *
       * The cache-only optimization stays explicitly QA opt-in until repeated
       * hardware tests. Failed TLS/config still runs the original bounded
       * GPIO recovery; TLS-GCM and the real matcher cannot be bypassed. */
      gboolean cached_pmk_fast =
        self->psk_from_cache &&
        g_strcmp0 (g_getenv ("GXFP_PMK_CACHE_SKIP_RESET_QA"), "1") == 0;
      if (cached_pmk_fast)
        {
          fp_warn ("GXFP51A0 cached PMK QA: skipping unnecessary post-stage "
                   "reset after secure cache load; full TLS validation next");
          if (g_strcmp0 (g_getenv ("GXFP_RESUME_STAGE_QA"), "1") == 0)
            fp_warn ("GXFP51A0 RESUME_STAGE_QA stage=pmk_cached_skip_post_reset");
        }
      if (!cached_pmk_fast)
        {
          fp_info ("GXFP51A0 PMK retained; performing reviewed post-PMK reset");
          if (g_strcmp0 (g_getenv ("GXFP_RESUME_STAGE_QA"), "1") == 0)
            fp_warn ("GXFP51A0 RESUME_STAGE_QA reset_reason=pmk_post_stage");
          gx_gpio_reset (self);
          if (!gx_read_fw_version_stage2e (self, fw, sizeof fw) ||
              g_strcmp0 (fw, "GF_ST411SEC_APP_14115") != 0)
            {
              fp_warn ("GXFP51A0 target init failed: post-PMK reset/A8");
              return FALSE;
            }
          fp_info ("GXFP51A0 post-PMK reset/A8 confirmed: %s", fw);
        }
    }

  if (!gx_target_configure (self))
    {
      fp_warn ("GXFP51A0: target initialization/configuration failed");
      return FALSE;
    }

  if (!gx_send_plain_raw (self, GX_REQTLS, sizeof GX_REQTLS))
    return FALSE;
  n = gx_read_frame (self, &type, ack, sizeof ack);
  if (n <= 0 || type != GOODIX_PKT_PLAIN ||
      !gxfp_parse_ack (ack, n, 0xd0, &status) ||
      !gxfp_ack_status_success (status))
    {
      fp_warn ("GXFP51A0: D0 TLS request was not accepted");
      return FALSE;
    }

  return TRUE;
}


/* Establishes the TLS-PSK channel, with us as the server. */
static gboolean
gx_tls_handshake (FpiDeviceGoodix51A0 *self)
{
  g_autoptr(GError) err = NULL;
  guint8 ack[128], status = 0, type = 0;
  int n;

  if (!self->psk_ready)
    return FALSE;
  self->tls_rxlen = self->tls_rxpos = 0;
  g_clear_pointer (&self->tls, gx_tls_free);
  self->tls = gx_tls_new (self->psk, GOODIX_PSK_LEN, GOODIX_TLS_IDENTITY,
                          gx_bio_send, gx_bio_recv, self);
  if (!self->tls)
    {
      fp_warn ("cannot set up the TLS channel");
      return FALSE;
    }
  if (!gx_tls_handshake_run (self->tls, &err))
    {
      fp_warn ("%s", err->message);
      return FALSE;
    }

  /* Windows and the validated Stage3C path both confirm the completed
   * handshake back to the MCU with NOP -> D4. */
  if (!gx_send_plain_raw (self, GX_TLS_OK, sizeof GX_TLS_OK))
    {
      fp_warn ("GXFP51A0: failed to send TLS established D4");
      return FALSE;
    }
  n = gx_read_frame (self, &type, ack, sizeof ack);
  if (n <= 0 || type != GOODIX_PKT_PLAIN ||
      !gxfp_parse_ack (ack, n, 0xd4, &status) ||
      !gxfp_ack_status_success (status))
    {
      fp_warn ("GXFP51A0: TLS established D4 was not accepted");
      return FALSE;
    }

  fp_info ("GXFP51A0 TLS established ACK D4 accepted");
  fp_info ("GXFP51A0 PMK candidate validated by live TLS handshake");
  fp_info ("TLS-PSK session up (%s)", gx_tls_ciphersuite (self->tls));
  self->tls_up = TRUE;
  if (self->psk_from_cache)
    fp_info ("GXFP51A0 cached PMK validated by live TLS handshake");
  else
    (void) gx_pmk_cache_save (self);
  return TRUE;
}


static void
gx_tls_teardown (FpiDeviceGoodix51A0 *self)
{
  if (self->tls_up)
    {
      gx_tls_close (self->tls);
      self->tls_up = FALSE;
    }
  g_clear_pointer (&self->tls, gx_tls_free);
}

/* Exact 14115/51x7 capture recipes, as validated by the Linux imaging path.
 * 30 ms remains the default on the reference 2021 unit. Slower controllers can
 * learn a capture-only pacing multiplier after a real GET_IMAGE transport
 * desync; this is deliberately separate from TLS/init timing. */
#ifndef GX_SEQ_GAP_US
#define GX_SEQ_GAP_US 30000
#endif

typedef enum
{
  GX_CAPTURE_FINGER,
  GX_CAPTURE_BACKGROUND_COLD,
  GX_CAPTURE_BACKGROUND_WARM
} GxCaptureKind;

static gboolean
gx_send_capture_recipe (FpiDeviceGoodix51A0 *self, GxCaptureKind kind)
{
  struct gxfp_capture_recipe recipe;
  gboolean ok;
  gsize i;

  if (kind == GX_CAPTURE_BACKGROUND_COLD)
    ok = self->have_target_cal &&
         gxfp_build_background_capture_recipe (&self->target_cal, &recipe);
  else if (kind == GX_CAPTURE_BACKGROUND_WARM)
    ok = gxfp_build_warm_background_capture_recipe (&recipe);
  else
    ok = gxfp_build_finger_capture_recipe (&recipe);
  if (!ok)
    return FALSE;

  for (i = 0; i < recipe.count; i++)
    {
      const struct gxfp_target_packet *packet = &recipe.steps[i];
      gint64 t0 = g_get_monotonic_time ();
      if (self->s3_presleep_deadline_us > 0 &&
          t0 >= self->s3_presleep_deadline_us)
        return FALSE;

      if (!gx_send_plain_drain (self, packet->inner, packet->inner_len, NULL, NULL))
        return FALSE;
      fp_dbg ("chrono cmd=%02x drain=%ld us", packet->inner[0],
              (long) (g_get_monotonic_time () - t0));
      g_usleep (gx_capture_gap_us (self));
    }
  return TRUE;
}


/* Takes exactly one complete sensor->host TLS transport frame. A frame may
 * already have been stashed by gx_send_plain_drain(); otherwise wait for it. */
static int
gx_take_tls_frame (FpiDeviceGoodix51A0 *self, guint8 *rec, gsize cap)
{
  int raw, k;
  int irq_high = 0, read_error = 0, plain_frame = 0;

  if (self->tls_rxlen > self->tls_rxpos)
    {
      raw = self->tls_rxlen - self->tls_rxpos;
      if ((gsize) raw > cap)
        return -1;
      memcpy (rec, self->tls_rx + self->tls_rxpos, raw);
      self->tls_rxlen = self->tls_rxpos = 0;
      return raw;
    }

  for (k = 0; k < GX_TLS_IRQ_POLLS; k++)
    {
      guint8 ty;
      if (self->s3_presleep_deadline_us > 0 &&
          g_get_monotonic_time () >= self->s3_presleep_deadline_us)
        return -1;

      errno = 0;
      if (gx51_wait_irq_gpio48 (self->irq_fd, GX_TLS_IRQ_POLL_MS) < 0)
        {
          if (errno != ETIMEDOUT)
            return -1;
          continue;
        }

      irq_high++;
      raw = gx_read_frame (self, &ty, rec, cap);
      if (raw > 0 && ty == GOODIX_PKT_TLS)
        return raw;
      if (raw <= 0)
        read_error++;
      else if (ty == GOODIX_PKT_PLAIN)
        plain_frame++;
    }
  /* Only counts and frame classes: no raw frames, fingerprint pixels,
   * TLS records, keys or personally identifying data are logged.  On
   * genuine S3 this separates an IRQ that never rises from an invalid
   * SPI frame or a delayed plain response after GET_IMAGE ACK. */
  fp_warn ("GXFP51A0 TLS_FRAME_TIMEOUT_DIAG irq_high=%d "
           "read_error=%d plain_frame=%d polls=%d",
           irq_high, read_error, plain_frame, GX_TLS_IRQ_POLLS);
  return -1;
}


static int
gx_fail_get_image_after_tls_timeout (FpiDeviceGoodix51A0 *self)
{
  /* Reaching this point means gx_send_plain_drain() already observed evidence
   * that GET_IMAGE was accepted (normally its cleartext ACK), then no TLS image
   * arrived during the full bounded TLS wait.  The command may still complete
   * late.  Replaying it here is unsafe: two accepted GET_IMAGE commands can
   * leave two application records in flight, so the next semantic capture can
   * consume a stale record even when AES-GCM sequence authentication itself
   * still succeeds.  The only unambiguous recovery boundary is a fresh MCU/TLS
   * session.  By contrast, gx_send_plain_drain() still retries once when
   * neither ACK nor TLS was observed, because there is no evidence that command
   * reached the sensor. */
  fp_warn ("GXFP51A0 GET_IMAGE was accepted but TLS image timed out; "
           "not replaying accepted command; forcing full session recovery");

  if (!self->capture_recovery_pending)
    gx_capture_transport_desync (self);
  return -1;
}


/* The 51x7 reference requires an FDT-up image, NAV and final FDT-down after a
 * finger frame. The intermediate image record must be AUTHENTICATED/decrypted,
 * not merely discarded: AES-GCM record sequence numbers advance for every
 * sensor record, and skipping it would desynchronise the following capture. */
static gboolean
gx_send_capture_cleanup (FpiDeviceGoodix51A0 *self)
{
  struct gxfp_capture_recipe recipe;
  g_autofree guint8 *rec = g_malloc0 (GOODIX_RX_MAX);
  g_autofree guint8 *plain = g_malloc0 (GOODIX_RX_MAX);
  gboolean ok = FALSE;
  gboolean perf_qa =
    g_strcmp0 (g_getenv ("GXFP_CAPTURE_PERF_QA"), "1") == 0;
  gsize i;

  if (!gxfp_build_capture_cleanup_recipe (&recipe))
    goto out;

  for (i = 0; i < recipe.count; i++)
    {
      const struct gxfp_target_packet *packet = &recipe.steps[i];
      gint64 t_step = perf_qa ? g_get_monotonic_time () : 0;
      if (self->s3_presleep_deadline_us > 0 &&
          g_get_monotonic_time () >= self->s3_presleep_deadline_us)
        goto out;

      if (!gx_send_plain_drain (self, packet->inner, packet->inner_len, NULL, NULL))
        goto out;

      if (packet->inner[0] == 0x20u)
        {
          int raw = gx_take_tls_frame (self, rec, GOODIX_RX_MAX);
          gssize got;

          if (raw < 0)
            raw = gx_fail_get_image_after_tls_timeout (self);

          got = raw > 0
                  ? gx_tls_decrypt_record (self->tls, rec, (gsize) raw,
                                           plain, GOODIX_RX_MAX)
                  : -1;
          if (got != (gssize) GXFP_IMAGE_PLAINTEXT_LEN)
            {
              fp_warn ("capture cleanup image record failed (%d raw, %ld plain)",
                       raw, (long) got);
              if (!self->capture_recovery_pending)
                gx_capture_transport_desync (self);
              goto out;
            }
        }

      if (perf_qa)
        fp_warn ("GXFP51A0 CAPTURE_PERF_QA cleanup_step=%lu cmd=0x%02x "
                 "processing_ms=%ld",
                 (unsigned long) i, packet->inner[0],
                 (long) ((g_get_monotonic_time () - t_step) / 1000));
      g_usleep (gx_capture_gap_us (self));
    }

  ok = TRUE;
out:
  OPENSSL_cleanse (rec, GOODIX_RX_MAX);
  OPENSSL_cleanse (plain, GOODIX_RX_MAX);
  return ok;
}


/* Captures one full image, assuming a TLS session is already up, and fills
 * px[GOODIX_IMG_PIXELS] with 12-bit samples. */
static gboolean
gx_capture_frame_ex (FpiDeviceGoodix51A0 *self,
                     guint16             *px,
                     GxCaptureKind        kind,
                     gboolean             cleanup)
{
  gboolean background = kind != GX_CAPTURE_FINGER;
  g_autofree guint8 *img = g_malloc (GOODIX_RX_MAX);
  int total = 0;

  self->capture_retry_seen = FALSE;
  gint64 tA = g_get_monotonic_time ();
  if (!gx_send_capture_recipe (self, kind))
    return FALSE;
  fp_dbg ("timing: capture sequence %ld us",
           (long) (g_get_monotonic_time () - tA));

  /* The image arrives as one complete sensor application-data record. Keep
   * explicit record decryption here because capture drains/stashes the already
   * framed transport record and the client-read sequence is tracked explicitly.
   * The record has usually already been picked up while draining the capture
   * sequence; otherwise wait for it. */
  {
    g_autofree guint8 *rec = g_malloc (GOODIX_RX_MAX);
    int raw = gx_take_tls_frame (self, rec, GOODIX_RX_MAX);
    gssize got;

    if (raw < 0)
      raw = gx_fail_get_image_after_tls_timeout (self);

    got = raw > 0 ? gx_tls_decrypt_record (self->tls, rec, (gsize) raw,
                                           img, GOODIX_RX_MAX) : -1;
    if (got < 0)
      {
        fp_warn ("cannot authenticate/decrypt the image record (%d raw bytes); "
                 "forcing full session recovery", raw);
        if (!self->capture_recovery_pending)
          gx_capture_transport_desync (self);
        return FALSE;
      }
    total = (int) got;
  }
  fp_dbg ("timing: whole capture %ld us (image %d bytes)",
           (long) (g_get_monotonic_time () - tA), total);
  if (total != (int) GXFP_IMAGE_PLAINTEXT_LEN)
    {
      fp_warn ("unexpected GXFP51A0 image plaintext: %d bytes (expected %u)",
               total, (unsigned) GXFP_IMAGE_PLAINTEXT_LEN);
      return FALSE;
    }
  if (!gxfp_decode_image_plaintext (img, (gsize) total, px))
    {
      fp_warn ("cannot decode GXFP51A0 88x80 transport raster");
      return FALSE;
    }

  /* Preserve a successfully decoded finger frame before cleanup.  The cleanup
   * is required to keep TLS record sequencing healthy for subsequent captures,
   * but a cleanup failure must not destroy the diagnostic evidence from the
   * frame we already authenticated and decoded.  gx_dump_capture() is inert
   * unless the private dump directory exists. */
#ifdef GXFP51A0_DEVELOPER
  if (!background)
    gx_dump_capture (self, px);
#endif

  if (!background && cleanup)
    {
      if (!gx_send_capture_cleanup (self))
        {
          fp_warn ("GXFP51A0 post-capture cleanup failed");
          return FALSE;
        }
      gx_capture_pacing_success (self);
    }
  return TRUE;
}

static gboolean
gx_capture_frame (FpiDeviceGoodix51A0 *self, guint16 *px, gboolean background)
{
  return gx_capture_frame_ex (self, px,
                              background ? GX_CAPTURE_BACKGROUND_COLD
                                         : GX_CAPTURE_FINGER,
                              TRUE);
}

static gboolean
gx_capture_warm_background_frame (FpiDeviceGoodix51A0 *self, guint16 *px)
{
  return gx_capture_frame_ex (self, px, GX_CAPTURE_BACKGROUND_WARM, TRUE);
}


static int
gx_cmp_dbl (const void *a, const void *b)
{
  double x = *(const double *) a - *(const double *) b;
  return (x > 0) - (x < 0);
}






/* Moyenne n trames en un tableau de pixels. */
static gboolean
gx_capture_avg (FpiDeviceGoodix51A0 *self, int nframes, guint16 *avg)
{
  g_autofree guint32 *acc = g_malloc0 (sizeof (guint32) * GOODIX_IMG_PIXELS);
  guint16 px[GOODIX_IMG_PIXELS];
  int got = 0, f, i;

  for (f = 0; f < nframes; f++)
    {
      if (!gx_capture_frame (self, px, TRUE))
        {
          if (self->capture_recovery_pending)
            return FALSE;
          continue;
        }
      for (i = 0; i < GOODIX_IMG_PIXELS; i++)
        acc[i] += px[i];
      got++;
    }
  if (!got)
    return FALSE;
  for (i = 0; i < GOODIX_IMG_PIXELS; i++)
    avg[i] = acc[i] / got;
  return TRUE;
}

/* ------------------------------------------------------------------ */
/*  Device life cycle: open / close                                    */
/* ------------------------------------------------------------------ */



/* Native finger detection: six 16-bit values, one per sensor zone, in about
 * 33 ms — some 70 times faster than capturing an image. A finger pulls those
 * values DOWN by roughly 100 counts (around 335/371/361 idle against
 * 229/269/275 with a finger), which is what makes cheap polling possible. */
#define GX_FDT_PROBE_ATTEMPTS 3
#define GX_FDT_IRQ_TIMEOUT_MS 100

static int
gx_fdt_probe_ex (FpiDeviceGoodix51A0 *self,
                 int                 *out_zones,
                 guint8              *out_touchflag)
{
  struct gxfp_target_packet packet;
  guint8 rx[256], ty, ack_status, touchflag;
  guint16 zones[GXFP_FDT_ZONE_COUNT];
  int attempt, n, k;

  if (!gxfp_build_fdt_probe (&packet))
    return -1;

  for (attempt = 1; attempt <= GX_FDT_PROBE_ATTEMPTS; attempt++)
    {
      const gchar *stage = "send";

      /* FDT manual is idempotent and Windows reissues it repeatedly while
       * building the baseline. A missing IRQ here is therefore recoverable:
       * retry the complete command, but never spin indefinitely. */
      if (!gx_send_plain_raw (self, packet.inner, packet.inner_len))
        return -1;

      g_usleep (15000 * self->timing_scale / 100);

      stage = "ack";
      errno = 0;
      if (gx51_wait_irq_gpio48 (self->irq_fd, GX_FDT_IRQ_TIMEOUT_MS) < 0)
        goto retry;

      n = gx_read_frame (self, &ty, rx, sizeof rx);
      if (n <= 0 || ty != GOODIX_PKT_PLAIN ||
          !gxfp_parse_ack (rx, (size_t) n, 0x36u, &ack_status) ||
          !gxfp_ack_status_success (ack_status))
        goto retry;

      g_usleep (8000 * self->timing_scale / 100);

      stage = "response";
      errno = 0;
      if (gx51_wait_irq_gpio48 (self->irq_fd, GX_FDT_IRQ_TIMEOUT_MS) < 0)
        goto retry;

      n = gx_read_frame (self, &ty, rx, sizeof rx);
      if (n <= 0 || ty != GOODIX_PKT_PLAIN ||
          !gxfp_parse_fdt_response (rx, (size_t) n, &touchflag, zones))
        goto retry;

      for (k = 0; k < (int) GXFP_FDT_ZONE_COUNT; k++)
        out_zones[k] = (int) zones[k];
      if (out_touchflag)
        *out_touchflag = touchflag;

      return 0;

retry:
      fp_warn ("GXFP51A0 FDT probe retry: stage=%s attempt=%d/%d",
               stage, attempt, GX_FDT_PROBE_ATTEMPTS);
      if (attempt == 1)
        gx_protocol_timing_miss (self, stage);
      if (attempt < GX_FDT_PROBE_ATTEMPTS)
        {
          /* Give a late/stale transaction a small quiesce window before
           * reissuing the same idempotent FDT-manual command. */
          g_usleep (10000 * self->timing_scale / 100);
        }
    }

  return -1;
}

static guint
gx_fdt_touch_count (guint8 touchflag)
{
  guint count = 0;

  for (guint8 zones = touchflag & 0x3fu; zones; zones >>= 1)
    count += zones & 1u;
  return count;
}

#define GX_FDT_TOUCH_MIN_ZONES 5

static gboolean
gx_fdt_touch_is_finger (guint8 touchflag)
{
  return gx_fdt_touch_count (touchflag) >= GX_FDT_TOUCH_MIN_ZONES;
}

/* Mean of the six exact-target FDT zones: an absolute test needing no baseline. */
static int
gx_fdt_mean (const int *v)
{
  int k, s = 0;
  for (k = 0; k < (int) GXFP_FDT_ZONE_COUNT; k++)
    s += v[k];
  return s / (int) GXFP_FDT_ZONE_COUNT;
}

/* Total drop against the baseline; above the threshold means a finger. */
static int
gx_fdt_drop (const int *base, const int *cur)
{
  int k, d = 0;
  for (k = 0; k < (int) GXFP_FDT_ZONE_COUNT; k++)
    d += base[k] - cur[k];
  return d / (int) GXFP_FDT_ZONE_COUNT;
}

/* ------------------------------------------------------------------ */
/*  Enrolment and verification, using local descriptors instead of the */
/*  NBIS pipeline.                                                     */
/*                                                                     */
/*  FpImageDevice mandates NBIS (minutiae plus bozorth3), which this    */
/*  tiny active area cannot reliably feed the stock NBIS pipeline.     */
/*  far below what reliable matching needs, and bozorth3 scored zero    */
/*  every time. Hence deriving straight from FpDevice and implementing  */
/*  enrol and verify against the descriptor matcher in goodix_sift.c.   */
/* ------------------------------------------------------------------ */

/* SIGFM score gates.
 *
 * These are candidate values, not security claims.  The release is only
 * promotable after a labelled genuine/impostor corpus shows a clear margin.
 * External small-sensor SIGFM drivers use runtime gates around 100-150; start
 * at the conservative low end until this exact 80x64 target is measured. */
#define GX_MATCH_THRESHOLD            7
#define GX_MIN_CAPTURE_KEYPOINTS     25

/* rel71.30 candidate: conservative per-view photometric rescue.
 *
 * The primary SIGFM acceptance threshold remains 7.  Only a weak-but-usable
 * per-view score in [3,6] may be promoted, and only when the already-enrolled
 * normalized raster independently clears every strict alignment gate below.
 * This is not score fusion and does not add another biometric draw: one
 * physical pose still produces exactly one authentication decision.
 *
 * Gate A from the local aggregate screen remains selected for rel71.30:
 *   genuine enrolled-view pairs rescued: 5
 *   inter-finger enrolled-view pairs rescued: 0 / 1200
 * The primary candidate's best SIGFM score must still be at least 4, so a
 * photometric match can never promote a completely weak gallery result.
 * Historical inter-finger maxima were ZNCC=.634 and agreement=.800 versus
 * the rescue minima .750/.845.  This remains local validation evidence,
 * not a population FAR claim, so rel71.30 stays private until field-tested. */
#define GX_PIXEL_RESCUE_CANDIDATE_BEST_MIN 4
#define GX_PIXEL_RESCUE_SCORE_MIN       3
#define GX_PIXEL_RESCUE_SCORE_MAX       6
#define GX_PIXEL_RESCUE_MIN_OVERLAP  1500
#define GX_PIXEL_RESCUE_MIN_ZNCC_MILLI 750
#define GX_PIXEL_RESCUE_MIN_AGREE_PERMILLE 845

/* One user-visible press is one biometric sample.  Hidden second captures made
 * the previous enrollment slow and did not add controlled diversity. */
#define GX_ENROLL_STAGES             20
#define GX_VIEWS_PER_STAGE            1
#define GX_ENROLL_VIEWS              GX_ENROLL_STAGES

/* Enrollment topology policy for a tiny partial sensor.
 *
 * The first five accepted views form a redundant anchor around the user's
 * natural placement. Later views may extend coverage, but must remain connected
 * to the accepted template graph. This prevents a valid-looking yet effectively
 * isolated capture from consuming one of the fixed 20 enrollment slots.
 *
 * Authentication threshold remains independent at 7; these much lower scores
 * are used only while the user is deliberately enrolling the same finger. */
#define GX_ENROLL_ANCHOR_STAGES              5
#define GX_ENROLL_ANCHOR_MIN_SCORE           3
#define GX_ENROLL_EXTENSION_MIN_SCORE        3
#define GX_ENROLL_EXTENSION_WEAK_SCORE       2
#define GX_ENROLL_EXTENSION_WEAK_LINKS       2

/* Reads the firmware version, which also proves the SPI dialogue works. */
static gboolean
gx_wait_plain_ack_for (FpiDeviceGoodix51A0 *self, guint8 command)
{
  guint8 rx[128], type = 0;
  int n = gx_read_frame (self, &type, rx, sizeof rx);

  return n >= 5 &&
         type == GOODIX_PKT_PLAIN &&
         rx[0] == 0xB0 &&
         rx[3] == command;
}

static gboolean
gx_driverstate_install_windows (FpiDeviceGoodix51A0 *self)
{
  int wrapper, attempt;

  /* Windows GF3658 first-contact: NOP -> 5 ms -> DriverState Install. */
  if (!gx_write_frame (self, GOODIX_PKT_PLAIN,
                       GX_AMORCE, sizeof GX_AMORCE))
    return FALSE;

  g_usleep (5000);

  for (wrapper = 0; wrapper < 2; wrapper++)
    for (attempt = 0; attempt < 2; attempt++)
      {
        if (!gx_write_frame (self, GOODIX_PKT_PLAIN,
                             GX_ENABLE, sizeof GX_ENABLE))
          return FALSE;
        if (gx_wait_plain_ack_for (self, 0x96))
          return TRUE;
      }

  return FALSE;
}

static gboolean
gx_read_fw_version_once (FpiDeviceGoodix51A0 *self, gchar *out, gsize cap)
{
  static const guint8 nop[] = {
    0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA5
  };
  static const guint8 a8[] = {
    GOODIX_CMD_FW_VERSION, 0x03, 0x00, 0x00, 0x00, 0xFF
  };
  guint8 rx[128], cached[128];
  guint8 ty = 0, cached_ty = 0;
  int n, cached_n = 0;
  int send_index, read_index;
  gboolean acked = FALSE;

  if (!gx_write_frame (self, GOODIX_PKT_PLAIN, nop, sizeof nop))
    return FALSE;
  g_usleep (5000);

  for (send_index = 0; send_index < 2 && !acked; send_index++)
    {
      cached_n = 0;
      cached_ty = 0;

      if (!gx_write_frame (self, GOODIX_PKT_PLAIN, a8, sizeof a8))
        return FALSE;

      for (read_index = 0; read_index < 2; read_index++)
        {
          n = gx_read_frame (self, &ty, rx, sizeof rx);
          if (n <= 0)
            break;
          if (ty != GOODIX_PKT_PLAIN)
            continue;

          if (n >= 5 && rx[0] == 0xB0 && rx[3] == GOODIX_CMD_FW_VERSION)
            {
              acked = TRUE;
              break;
            }

          if (n > 3 && rx[0] != 0xB0)
            {
              memcpy (cached, rx, n);
              cached_n = n;
              cached_ty = ty;
            }
        }
    }

  if (!acked)
    return FALSE;

  if (cached_n > 3 && cached_ty == GOODIX_PKT_PLAIN)
    {
      g_strlcpy (out, (const gchar *) cached + 3,
                 MIN ((gsize) (cached_n - 3) + 1, cap));
      return TRUE;
    }

  n = gx_read_frame (self, &ty, rx, sizeof rx);
  if (n <= 3 || ty != GOODIX_PKT_PLAIN || rx[0] == 0xB0)
    return FALSE;

  g_strlcpy (out, (const gchar *) rx + 3,
             MIN ((gsize) (n - 3) + 1, cap));
  return TRUE;
}

static gboolean
gx_read_fw_version (FpiDeviceGoodix51A0 *self, gchar *out, gsize cap)
{
  int attempt;

  /* GetEvkVersionWithRetry: three initial tries. The caller owns the
   * reviewed HardResetMcu fallback and one final try. */
  for (attempt = 0; attempt < 3; attempt++)
    if (gx_read_fw_version_once (self, out, cap))
      return TRUE;

  return FALSE;
}


/* ------------------------------------------------------------------ */
/*  Session: TLS setup, background frame, detection baseline           */
/* ------------------------------------------------------------------ */

/* Runtime timing adaptation.
 *
 * rel24-rel40 persisted protocol/capture timing under /var/lib/fprint.  That
 * looked useful for slow controllers, but deep-S3/prewarm transport loss could
 * be mis-attributed as a speed problem and ratchet 100 -> 150 -> ... -> 300
 * across reboots.  rel42 deliberately keeps both scales process-local.
 *
 * Every fresh/cold lifecycle starts at the Windows-validated nominal timing.
 * A real runtime failure may loosen timings for the current fprintd lifetime,
 * while clean captures decay back toward nominal.  No lifecycle/prewarm event
 * can poison a future boot through persistent tuning state. */
#define GX_TIMING_SCALE_MIN 100
#define GX_TIMING_SCALE_MAX 300
#define GX_TIMING_SCALE_STEP 50

#define GX_CAPTURE_SCALE_MIN 100
#define GX_CAPTURE_SCALE_MAX 300
#define GX_CAPTURE_SCALE_STEP 50
#define GX_CAPTURE_CLEAN_DECAY_STREAK 8

/* Protocol timing is a controller/session characteristic, not biometric
 * evidence. Start every fresh fprintd process at Windows-nominal 100%, then
 * widen only after an observed missed ACK/FDT response. Keep the result in RAM
 * for the daemon lifetime so a recovery does not relearn the same controller.
 * Nothing is persisted across daemon restarts or boots. */
static void
gx_protocol_timing_miss (FpiDeviceGoodix51A0 *self,
                         const gchar          *source)
{
  int previous = MAX (self->timing_scale, GX_TIMING_SCALE_MIN);

  if (previous >= GX_TIMING_SCALE_MAX)
    return;

  self->timing_scale =
    MIN (previous + GX_TIMING_SCALE_STEP, GX_TIMING_SCALE_MAX);

  fp_warn ("GXFP51A0 protocol timing auto-calibration: %d%% -> %d%% "
           "after %s miss; session-local only",
           previous, self->timing_scale, source ? source : "protocol");
}

static guint
gx_capture_gap_us (FpiDeviceGoodix51A0 *self)
{
  int scale = CLAMP (self->capture_gap_scale,
                     GX_CAPTURE_SCALE_MIN, GX_CAPTURE_SCALE_MAX);

  return (guint) ((guint64) GX_SEQ_GAP_US * (guint) scale / 100u);
}

static void
gx_capture_transport_desync (FpiDeviceGoodix51A0 *self)
{
  int previous = MAX (self->capture_gap_scale, GX_CAPTURE_SCALE_MIN);

  self->capture_clean_streak = 0;
  self->capture_recovery_pending = TRUE;

  /* Lifecycle/prewarm failures say nothing about the steady-state capture
   * gap. They request a session rebuild only and must never change pacing. */
  if (self->capture_pacing_suppressed)
    {
      fp_warn ("GXFP51A0 lifecycle recovery desync: pacing remains %d%% "
               "(%u us gap); full MCU/session recovery required",
               previous, gx_capture_gap_us (self));
      return;
    }

  self->capture_gap_scale =
    MIN (previous + GX_CAPTURE_SCALE_STEP, GX_CAPTURE_SCALE_MAX);

  fp_warn ("GXFP51A0 session capture desync: pacing %d%% -> %d%% "
           "(%u us gap); full MCU/session recovery required; not persisted",
           previous, self->capture_gap_scale, gx_capture_gap_us (self));
}

static void
gx_capture_pacing_success (FpiDeviceGoodix51A0 *self)
{
  if (self->capture_retry_seen)
    {
      int previous = MAX (self->capture_gap_scale, GX_CAPTURE_SCALE_MIN);
      int protocol_floor =
        CLAMP (self->timing_scale - GX_CAPTURE_SCALE_STEP,
               GX_CAPTURE_SCALE_MIN, 250);
      int target = MIN (GX_CAPTURE_SCALE_MAX,
                        MAX (previous + GX_CAPTURE_SCALE_STEP, protocol_floor));

      self->capture_clean_streak = 0;

      /* A successfully decoded real finger frame that needed the transport
       * retry is direct evidence that nominal capture pacing was too tight for
       * this session.  Do not wait for three failed user presses: calibrate the
       * NEXT physical press immediately.  The already-observed protocol timing
       * provides a conservative floor (300% protocol -> 250% capture on the
       * reference MateBook), while controllers with nominal protocol timing
       * still move only one 50-point step.  Nothing is persisted. */
      if (target > previous)
        {
          self->capture_gap_scale = target;
          fp_info ("GXFP51A0 session capture pacing calibrated by retry-assisted "
                   "finger frame: %d%% -> %d%% (%u us gap, protocol=%d%%); "
                   "not persisted",
                   previous, self->capture_gap_scale,
                   gx_capture_gap_us (self), self->timing_scale);
        }

      self->capture_retry_seen = FALSE;
      self->warm_last_activity_us = g_get_monotonic_time ();
      return;
    }


  if (self->capture_gap_scale > GX_CAPTURE_SCALE_MIN)
    {
      self->capture_clean_streak++;
      if (self->capture_clean_streak >= GX_CAPTURE_CLEAN_DECAY_STREAK)
        {
          int previous = self->capture_gap_scale;

          self->capture_gap_scale =
            MAX (GX_CAPTURE_SCALE_MIN,
                 previous - GX_CAPTURE_SCALE_STEP);
          self->capture_clean_streak = 0;
          fp_info ("GXFP51A0 session capture pacing decayed after %d clean "
                   "captures: %d%% -> %d%% (%u us gap)",
                   GX_CAPTURE_CLEAN_DECAY_STREAK, previous,
                   self->capture_gap_scale, gx_capture_gap_us (self));
        }
    }
  else
    self->capture_clean_streak = 0;

  self->capture_retry_seen = FALSE;
  self->warm_last_activity_us = g_get_monotonic_time ();
}

/* Establishes the TLS channel. This is the expensive step, about half a
 * second, and it does not depend on when the finger arrives — so it can be
 * done once and kept. */
static gboolean
gx_tls_session (FpiDeviceGoodix51A0 *self)
{
  int att;
  gboolean diagnostic = g_getenv ("GXFP_DIAGNOSTIC_ONESHOT") != NULL;
  gboolean capture_diagnostic = g_getenv ("GXFP_DIAGNOSTIC_CAPTURE_ONCE") != NULL;
  int max_attempts = capture_diagnostic ? 2 : (diagnostic ? 1 : 5);

  if (self->tls_up)
    return TRUE;

  for (att = 1; att <= max_attempts && !self->tls_up; att++)
    {
      GCancellable *c = gx_action_cancellable_if_active (self);

      if (c && g_cancellable_is_cancelled (c))
        {
          fp_info ("TLS setup cancelled by the caller");
          break;
        }

      gboolean stage_qa =
        g_strcmp0 (g_getenv ("GXFP_RESUME_STAGE_QA"), "1") == 0;
      gint64 qa_start = stage_qa ? g_get_monotonic_time () : 0;
      gboolean config_ok = gx_upload_config_and_reqtls (self, TRUE);
      gint64 qa_config = stage_qa ? g_get_monotonic_time () : 0;
      gboolean handshake_ok = config_ok && gx_tls_handshake (self);
      if (stage_qa)
        fp_warn ("GXFP51A0 RESUME_STAGE_QA stage=tls_attempt attempt=%d "
                 "config_ok=%d handshake_ok=%d pmk_cache=%d "
                 "config_ms=%ld handshake_ms=%ld",
                 att, config_ok ? 1 : 0, handshake_ok ? 1 : 0,
                 self->psk_from_cache ? 1 : 0,
                 (long) ((qa_config - qa_start) / 1000),
                 (long) ((g_get_monotonic_time () - qa_config) / 1000));
      if (handshake_ok)
        break;

      gx_tls_teardown (self);

      /* On the very first TLS failure following real S3, stop repeating
       * protocol resets through the same SPI lifetime. Let the enclosing
       * gx_prepare_capture_context() perform its reviewed full recovery:
       * close SPI/IRQ -> GPIO264 reset -> reopen -> firmware/A8 -> retry.
       * rel71.21 proved that changing EVERY protocol reset was harmful.
       * This exception is restricted to a proven S3 boundary. */
      if (self->s3_first_tls_pending && !self->s3_rel61_tls_fallback)
        {
          fp_warn ("GXFP51A0 NATIVE_S3_TLS_EARLY_RECOVER first failed handshake; full detached recovery before legacy retries");
          break;
        }

      /* A PMK that reached D4 in an earlier session is durable trusted state.
       * A transport/configuration/handshake failure later does NOT prove that
       * the key changed, so never unlink the cache on a transient failure. */
      if (self->psk_from_cache && diagnostic)
        {
          if (att >= max_attempts)
            {
              fp_warn ("GXFP51A0 cached PMK retained after failed session; diagnostic mode will not fall back to staging");
              break;
            }
          fp_warn ("GXFP51A0 cached PMK retained after failed diagnostic TLS attempt; retrying clean target init");
        }

      /* Self-healing on desync. The protocol delays are tuned to the author's
       * unit and sit right at the edge; a different SPI controller can be
       * slower and lose sync. Adaptation is session-local and never persisted. */
      gx_protocol_timing_miss (self, "tls-handshake");

      {
        gchar fw[64] = { 0 };

        /* A true S3 has already invalidated the MCU TLS session. The legacy
         * rel61 retry reset is safe on normal/awake transport, but our
         * observed post-S3 fallback reset on an OPEN spidev fd (spi_fd=12)
         * repeatedly failed TLS. Restrict detached retries to this proven
         * S3 recovery phase and keep ordinary LOCK retry behavior intact.
         *
         * No unbind/rebind under a live FpDevice: only its SPI/IRQ file
         * descriptors are closed, as in gx_recover_capture_context(). */
        if (self->s3_first_tls_pending && self->s3_rel61_tls_fallback)
          {
            g_autoptr(GError) reopen_error = NULL;

            fp_warn ("GXFP51A0 NATIVE_S3_TLS_DETACHED_RETRY closing SPI/IRQ before GPIO264 reset");
            gx_transport_close (self);
            if (stage_qa)
              fp_warn ("GXFP51A0 RESUME_STAGE_QA reset_reason=tls_detached_retry");
            gx_gpio_reset (self);
            if (!gx_transport_open (FP_DEVICE (self), &reopen_error))
              {
                fp_warn ("GXFP51A0 S3 TLS retry transport reopen failed: %s",
                         reopen_error ? reopen_error->message : "unknown");
                break;
              }
          }
        else
          {
            /* normal in-process TLS retry remains byte-for-byte the
             * historical rel61 behavior, avoiding rel71.21 regression. */
            if (stage_qa)
              fp_warn ("GXFP51A0 RESUME_STAGE_QA reset_reason=tls_in_process_retry");
            gx_gpio_reset (self);
          }

        if (!gx_read_fw_version_stage2e (self, fw, sizeof fw) ||
            g_strcmp0 (fw, "GF_ST411SEC_APP_14115") != 0)
          {
            fp_warn ("GXFP51A0 TLS retry reset/A8 failed");
            break;
          }
        fp_info ("GXFP51A0 TLS retry reset/A8 confirmed: %s", fw);
      }
    }

  /* Production-only stale-cache recovery. Keep the validated cache file on
   * disk while probing fresh boot staging. If the newly recovered candidate
   * reaches D4, gx_pmk_cache_save() atomically replaces the old file. If it
   * does not, the last-known-good cache remains available for a later boot. */
  if (!self->tls_up && !diagnostic && self->psk_from_cache &&
      (!self->s3_first_tls_pending || self->s3_rel61_tls_fallback))
    {
      fp_warn ("GXFP51A0 cached PMK could not establish TLS; trying fresh staging without deleting validated cache");
      gx_pmk_clear (self);

      if (gx_upload_config_and_reqtls (self, FALSE) &&
          gx_tls_handshake (self))
        {
          fp_info ("GXFP51A0 fresh staging replaced stale cache after live TLS validation");
        }
      else
        {
          gx_tls_teardown (self);
          fp_warn ("GXFP51A0 fresh staging fallback failed; validated cache file retained");
        }
    }

  if (!self->tls_up)
    fp_warn ("no TLS session after %d/%d attempts; if this persists the sensor "
             "needs a full recovery (long reset plus an spidev rebind)",
             att - 1, max_attempts);
  else if (self->timing_scale > GX_TIMING_SCALE_MIN)
    fp_info ("GXFP51A0 session protocol timing settled at %d%%; not persisted",
             self->timing_scale);

  return self->tls_up;
}

/* Refreshes the background frame and the detection baseline.
 *
 * This must happen JUST BEFORE each operation, unlike the TLS setup above: the
 * sensor's baseline level drifts, and the whole chain rests on the
 * background-minus-finger difference. A background captured too early yields a
 * distorted image and collapses the score — measured at 11 to 28 matches with
 * a fresh background against 2 to 4 with one a few tens of seconds old. */

/* How long to wait for the finger to be lifted before taking the background. */
#define GX_BG_WAIT_MS 2000

/* One-shot diagnostic UX: give the operator time to react to the explicit
 * "sensor clear" prompt before measuring anything. Then reject a calibration
 * whose mean moved too far from the just-observed no-finger anchor. */
#define GX_DIAG_CLEAR_GRACE_SEC 3
#define GX_DIAG_BASELINE_MAX_DRIFT 20
#define GX_DIAG_BASELINE_ATTEMPTS 4
#define GX_CLEAR_WAIT_MS 15000

/* The absolute FDT test used before ImageBase exists is STRICTER than
 * biometric finger detection. A 1..4-zone touch can be a partial placement:
 * do not learn it as no-finger merely because verify requires 5/6 zones.
 * Importantly, clean means BOTH zero active zones and undepressed ADC values.
 *
 * A single clean FDT sample is not a stable lift. On real S3 at 00:45:55..19
 * the user repeatedly removed and returned their finger during the 1-second
 * background capture; every image was rejected. Arm the background only after
 * confirmed dwell. Longer dwell after a contaminated capture prevents a
 * repeated, expensive image on every very brief lift.
 *
 * Do not accept the FDT-up IRQ of the related GDIX51C0 as a drop-in ST411
 * command until its different response framing has been bench validated. */
#define GX_CALIB_CLEAN_DWELL_US   G_GINT64_CONSTANT(260000)
#define GX_CALIB_PREIMAGE_DWELL_US G_GINT64_CONSTANT(140000)
#define GX_CALIB_RETOUCH_DWELL_US G_GINT64_CONSTANT(650000)
#define GX_CALIB_POLL_US         (50 * 1000)

static gboolean
gx_fdt_clear_stable_sample (guint8 touchflag,
                            int mean,
                            int anchor_mean,
                            gint64 now_us,
                            gint64 dwell_us,
                            gint64 *clean_since_us)
{
  if ((touchflag & 0x3fu) != 0 ||
      mean < GOODIX_FDT_ABS ||
      (anchor_mean >= 0 &&
       anchor_mean - mean > GX_DIAG_BASELINE_MAX_DRIFT))
    {
      *clean_since_us = 0;
      return FALSE;
    }

  if (*clean_since_us == 0)
    *clean_since_us = now_us;

  return now_us - *clean_since_us >= dwell_us;
}

/* Only called inside a genuine libfprint OPEN/VERIFY action with a valid
 * cancellable. Unlike rejected rel71.39, never invoke this from idle resume. */
static gboolean
gx_wait_stable_sensor_clear (FpiDeviceGoodix51A0 *self,
                             int anchor_mean,
                             int *out_mean,
                             gint64 dwell_us)
{
  gint64 deadline_us = g_get_monotonic_time () +
                       (gint64) GX_CLEAR_WAIT_MS * 1000;
  gint64 clean_since_us = 0;
  int cur[GXFP_FDT_ZONE_COUNT];
  gboolean warned = FALSE;

  while (g_get_monotonic_time () <= deadline_us)
    {
      guint8 touchflag = 0xff;
      gint64 now_us;
      int mean;

      GCancellable *c = gx_action_cancellable_if_active (self);
      if (c && g_cancellable_is_cancelled (c))
        return FALSE;

      if (gx_fdt_probe_ex (self, cur, &touchflag) == 0)
        {
          mean = gx_fdt_mean (cur);
          now_us = g_get_monotonic_time ();
          if (gx_fdt_clear_stable_sample (touchflag, mean, anchor_mean,
                                          now_us, dwell_us,
                                          &clean_since_us))
            {
              if (out_mean)
                *out_mean = mean;
              if (warned)
                fp_info ("GXFP51A0 CALIBRATION_CLEAR_STABLE mean=%d anchor=%d "
                         "dwell_ms=%d", mean, anchor_mean,
                         (int) (dwell_us / 1000));
              return TRUE;
            }

          if (!warned &&
              ((touchflag & 0x3fu) != 0 || mean < GOODIX_FDT_ABS ||
               (anchor_mean >= 0 &&
                anchor_mean - mean > GX_DIAG_BASELINE_MAX_DRIFT)))
            {
              fp_warn ("GXFP51A0 CALIBRATION_LIFT_REQUIRED keep finger OFF "
                       "until ImageBase is ready; mean=%d anchor=%d "
                       "touch=0x%02x", mean, anchor_mean, touchflag);
              warned = TRUE;
            }
        }
      else
        clean_since_us = 0;

      g_usleep (GX_CALIB_POLL_US);
    }

  fp_warn ("GXFP51A0 calibration could not find a stable clean sensor "
           "within %d ms (not a TLS/image failure)", GX_CLEAR_WAIT_MS);
  return FALSE;
}

static gboolean
gx_wait_clean_anchor (FpiDeviceGoodix51A0 *self, int *out_mean)
{
  gboolean ok = gx_wait_stable_sensor_clear (self, -1, out_mean,
                                              GX_CALIB_CLEAN_DWELL_US);
  if (ok)
    fp_info ("GXFP51A0 clean calibration anchor: mean=%d", *out_mean);
  return ok;
}

static gboolean
gx_wait_sensor_clear_stable (FpiDeviceGoodix51A0 *self,
                             int off_anchor_mean,
                             int *out_mean,
                             gint64 dwell_us)
{
  return gx_wait_stable_sensor_clear (self, off_anchor_mean, out_mean,
                                      dwell_us);
}

static gboolean
gx_wait_sensor_clear (FpiDeviceGoodix51A0 *self,
                      int off_anchor_mean,
                      int *out_mean)
{
  return gx_wait_sensor_clear_stable (self, off_anchor_mean, out_mean,
                                     GX_CALIB_PREIMAGE_DWELL_US);
}

static gboolean
gx_diag_press_countdown (FpiDeviceGoodix51A0 *self, int off_anchor_mean)
{
  int cur[GXFP_FDT_ZONE_COUNT];
  int q;

  for (;;)
    {
      gboolean restart = FALSE;

      fp_info ("GXFP51A0 CAPTURE_DIAGNOSTIC_GET_READY: keep finger OFF until zero");
      for (q = 3; q > 0; q--)
        {
          fp_info ("GXFP51A0 CAPTURE_DIAGNOSTIC_PRESS_IN_%d", q);
          g_usleep (G_USEC_PER_SEC);

          guint8 touchflag = 0;

          if (gx_fdt_probe_ex (self, cur, &touchflag) == 0)
            {
              int mean = gx_fdt_mean (cur);

              if (gx_fdt_touch_is_finger (touchflag) ||
                  off_anchor_mean - mean > GX_DIAG_BASELINE_MAX_DRIFT)
                {
                  fp_warn ("GXFP51A0 CAPTURE_DIAGNOSTIC_COUNTDOWN_RESTART: REMOVE_FINGER mean=%d anchor=%d",
                           mean, off_anchor_mean);
                  if (!gx_wait_sensor_clear (self, off_anchor_mean, NULL))
                    return FALSE;
                  restart = TRUE;
                  break;
                }
            }
        }

      if (restart)
        continue;

      fp_info ("GXFP51A0 CAPTURE_DIAGNOSTIC_PRESS_NOW: PRESS_AND_HOLD_NOW");
      return TRUE;
    }
}

/* Only reference a previously verified finger-OFF calibration during a very
 * short S3 wake. Keeping it across a suspend never keeps the old TLS session:
 * TLS and sensor configuration are rebuilt separately before this function.
 * CLOCK_BOOTTIME includes time actually asleep, unlike monotonic time. */
/* A clean reference is only captured from an authenticated finger-off
 * baseline, and independently revalidated by LIVE FDT immediately before
 * issuing the successful S3 sleep command. This allows realistic laptop
 * sleep periods without treating old background as a fresh calibration. */
#define GX_S3_CLEAN_REFERENCE_MAX_US (G_GINT64_CONSTANT(12) * 60 * 60 * G_USEC_PER_SEC)
#define GX_S3_CLEAN_REFERENCE_PRE_AGE_MAX_US (G_GINT64_CONSTANT(2) * 60 * 60 * G_USEC_PER_SEC)

static gint64
gx_s3_boottime_us (void)
{
  struct timespec ts;
  if (clock_gettime (CLOCK_BOOTTIME, &ts) != 0)
    return -1;
  return (gint64) ts.tv_sec * G_USEC_PER_SEC + ts.tv_nsec / 1000;
}

static void
gx_s3_discard_clean_bg (FpiDeviceGoodix51A0 *self)
{
  g_clear_pointer (&self->s3_saved_bg, g_free);
  self->s3_saved_at_boot_us = 0;
  self->s3_saved_bg_age_us = 0;
}

static void
gx_s3_snapshot_clean_bg (FpiDeviceGoodix51A0 *self)
{
  gint64 now = g_get_monotonic_time ();
  gint64 boot = gx_s3_boottime_us ();
  gint64 age = self->bg_last_refresh_us > 0 ?
               now - self->bg_last_refresh_us : G_MAXINT64;

  gboolean rolling =
    g_strcmp0 (g_getenv ("GXFP_HELD_FINGER_ROLLING_BG_QA"), "1") == 0;

  /* A clean backup is separate from transport/TLS reliability. If a previous
   * image cleanup broke the TLS stream, keep the last independently proven
   * clean background: later S3/S4 cannot create one with the finger already
   * on the glass. Non-QA behavior still discards as before. */
  /* The only accepted backgrounds were captured with a fully clear sensor
   * AND a valid subsequent FDT baseline. Never retain pending, dirty,
   * missing or old references from an unfinished biometric operation. */
  if ((!self->warm_valid && !rolling) ||
      !self->bg_frame || !self->have_fdt ||
      self->bg_dirty || !self->tls_up || age < 0 ||
      age > GX_S3_CLEAN_REFERENCE_PRE_AGE_MAX_US || boot <= 0 ||
      gx_fdt_mean (self->fdt_base) < GOODIX_FDT_ABS)
    {
      if (!rolling || !self->s3_saved_bg)
        gx_s3_discard_clean_bg (self);
      return;
    }

  gx_s3_discard_clean_bg (self);
  self->s3_saved_bg =
    g_memdup2 (self->bg_frame, sizeof (guint16) * GOODIX_IMG_PIXELS);
  memcpy (self->s3_saved_fdt, self->fdt_base,
          sizeof self->s3_saved_fdt);
  self->s3_saved_abs = self->fdt_abs;
  self->s3_saved_at_boot_us = boot;
  self->s3_saved_bg_age_us = age;
  fp_warn ("GXFP51A0 NATIVE_S3_CLEAN_REFERENCE_SAVED age_ms=%d RAM-only",
           (int) (age / 1000));
}

static gboolean
gx_s3_try_early_finger_reference (FpiDeviceGoodix51A0 *self)
{
  int cur[GXFP_FDT_ZONE_COUNT];
  guint8 touchflag = 0;
  gint64 boot = gx_s3_boottime_us ();
  gint64 elapsed;
  gint64 saved_age;
  int mean;

  if (!self->s3_saved_bg || boot <= 0 ||
      self->s3_saved_at_boot_us <= 0)
    return FALSE;

  elapsed = boot - self->s3_saved_at_boot_us;
  saved_age = self->s3_saved_bg_age_us;
  if (elapsed < 0 || saved_age < 0 ||
      elapsed + saved_age > GX_S3_CLEAN_REFERENCE_MAX_US)
    {
      gx_s3_discard_clean_bg (self);
      return FALSE;
    }

  if (gx_fdt_probe_ex (self, cur, &touchflag) != 0)
    return FALSE;

  mean = gx_fdt_mean (cur);
  /* Only a deliberate FULL 6/6 finger contact is allowed to use the
   * previously clean reference. For partial/absent contact, recalibrate
   * normally. This does NOT relax quality/matcher thresholds, which still
   * require actual fingerprint evidence after the image is captured. */
  if (mean >= GOODIX_FDT_ABS - 30 ||
      mean >= self->s3_saved_abs)
    return FALSE;

  /* On real natural S3 the FIRST arriving finger frequently activates
   * 0x3c (four zones), then evolves to full 0x3f. Refusing even to
   * consider a proven-clean, S3-saved ImageBase trapped the greeter in
   * repeated 15-second finger-OFF calibrations (37.6 seconds recorded).
   *
   * Keep the original 6/6 gate unless QA is explicitly opted in.
   * A 4-5 zone contact may reuse the saved, age-bounded, finger-OFF
   * reference ONLY with two independent FDT measurements and a >=50 ADC
   * mean drop against the preserved finger-OFF FDT baseline. This is
   * hardware preparation, NOT biometric acceptance: the single real
   * captured image still has to pass full quality, SIGFM and strict
   * threshold=7 with no extra draw or template change. */
  {
    guint mask = touchflag & 0x3fu;
    guint n = 0;
    gboolean partial_qa;

    for (guint bits = mask; bits; bits >>= 1)
      n += bits & 1u;
    partial_qa =
      g_strcmp0 (g_getenv ("GXFP_S3_EARLY_PARTIAL_QA"), "1") == 0 &&
      self->s3_first_tls_pending && n >= 4 && n < 6 &&
      gx_fdt_mean (self->s3_saved_fdt) - mean >= 50;
    if (mask != 0x3fu && !partial_qa)
      return FALSE;
    if (partial_qa)
      {
        int confirm[GXFP_FDT_ZONE_COUNT];
        guint8 confirm_touch = 0;
        guint confirm_n = 0;
        guint confirm_mask;

        g_usleep (GX_CALIB_POLL_US);
        if (gx_fdt_probe_ex (self, confirm, &confirm_touch) != 0)
          return FALSE;

        confirm_mask = confirm_touch & 0x3fu;
        for (guint bits = confirm_mask; bits; bits >>= 1)
          confirm_n += bits & 1u;
        mean = gx_fdt_mean (confirm);
        if (confirm_n < 4 ||
            mean >= GOODIX_FDT_ABS - 30 ||
            mean >= self->s3_saved_abs ||
            gx_fdt_mean (self->s3_saved_fdt) - mean < 50)
          return FALSE;
        touchflag = confirm_touch;
        fp_warn ("GXFP51A0 S3_EARLY_PARTIAL_QA confirmed 2 FDT "
                 "samples, zones=%u mean=%d; trusted saved background "
                 "still requires strict biometric match",
                 confirm_n, mean);
      }
  }

  if (!self->bg_frame)
    self->bg_frame = g_malloc (sizeof (guint16) * GOODIX_IMG_PIXELS);
  memcpy (self->bg_frame, self->s3_saved_bg,
          sizeof (guint16) * GOODIX_IMG_PIXELS);
  memcpy (self->fdt_base, self->s3_saved_fdt,
          sizeof self->fdt_base);
  self->fdt_abs = self->s3_saved_abs;
  self->have_fdt = TRUE;
  self->bg_dirty = FALSE;
  /* Preserve the ORIGINAL clean-reference age. When the finger is lifted,
   * warm rebase will acquire a fresh background rather than pretending a
   * recycled image was newly calibrated. */
  /* Monotonic time excludes deep sleep; a valid 8 h suspend can exceed
   * current active uptime. Never store negative timestamps. This original
   * calibration is treated as stale and refreshed once the finger lifts. */
  self->bg_last_refresh_us =
    MAX (G_GINT64_CONSTANT(1),
         g_get_monotonic_time () - (saved_age + elapsed));
  fp_warn ("GXFP51A0 NATIVE_S3_EARLY_FINGER_VERIFIED_REFERENCE "
           "live_touch=0x%02x mean=%d prev_abs=%d age_ms=%d; "
           "normal strict image/matcher gates still required",
           touchflag, mean, self->fdt_abs,
           (int) ((saved_age + elapsed) / 1000));
  /* Normally this is a single-use S3 snapshot. QA-only rolling cache keeps
   * a known clean ImageBase RAM-only across successive wake/transport errors
   * until the next independently clean background replaces it or 12h expire.
   * The next encrypted finger frame STILL has to pass full quality + 7 score. */
  if (g_strcmp0 (g_getenv ("GXFP_HELD_FINGER_ROLLING_BG_QA"), "1") != 0)
    gx_s3_discard_clean_bg (self);
  return TRUE;
}

static gboolean
gx_prepare_capture_context_once (FpiDeviceGoodix51A0 *self,
                                 gboolean capture_diagnostic)
{
  int q, k;
  int off_anchor_mean = -1;

  if (!gx_tls_session (self))
    return FALSE;


  /* The sensor drives GPIO48 level-high while a response is pending. The
   * first background capture must not race the tail of the TLS handshake:
   * wait for a proven idle-low boundary, then give the MCU a short quiesce
   * window before starting the capture recipe. This only runs on cold/context
   * preparation; the normal warm Claim path never pays this delay. */
  if (gx51_wait_irq_gpio48_low (self->irq_fd, 250) < 0)
    {
      fp_warn ("GXFP51A0 TLS settled but IRQ did not return low before calibration");
      return FALSE;
    }
  g_usleep (100 * 1000);

  if (capture_diagnostic)
    {
      fp_info ("GXFP51A0 CAPTURE_DIAGNOSTIC_PREPARING_BACKGROUND: KEEP_FINGER_OFF_SENSOR");
      fp_info ("GXFP51A0 CAPTURE_DIAGNOSTIC_CLEAR_SENSOR: REMOVE_FINGER_NOW");
      for (q = GX_DIAG_CLEAR_GRACE_SEC; q > 0; q--)
        {
          fp_info ("GXFP51A0 CAPTURE_DIAGNOSTIC_CALIBRATION_STARTS_IN_%d", q);
          g_usleep (G_USEC_PER_SEC);
        }
    }

  self->bg_dirty = FALSE;

  /* If the user already rests a FULL finger on the sensor at S3 wake, no
   * unoccupied reference image can be collected right now. A short-lived
   * copy of the LAST proven-clean calibration from before S3 may be used
   * solely to let the ordinary quality and matcher gates process that first
   * real finger. Longer sleep or partial contact retains cold calibration. */
  if (!capture_diagnostic && gx_s3_try_early_finger_reference (self))
    return TRUE;

  /* The first post-S4 FDT request can be lost once (ACK retry observed).
   * Without another chance we enter a long finger-OFF calibration loop,
   * even though a clean RAM-only background exists. In QA mode retry only
   * the read-only FDT proof; do not acquire/accept a touched ImageBase. */
  if (!capture_diagnostic &&
      g_strcmp0 (g_getenv ("GXFP_HELD_FINGER_ROLLING_BG_QA"), "1") == 0 &&
      self->s3_saved_bg)
    {
      for (guint proof_attempt = 0; proof_attempt < 2; proof_attempt++)
        {
          g_usleep (35 * 1000);
          if (gx_s3_try_early_finger_reference (self))
            {
              fp_warn ("GXFP51A0 HELD_FINGER_QA early contact "
                       "proved on retry=%u without lift", proof_attempt + 1);
              return TRUE;
            }
        }
    }

  /* Both the background and the detection baseline are only meaningful with
   * the finger LIFTED. A finger left resting on the sensor when the operation
   * starts ends up in the background, the difference then cancels out, and the
   * capture is worthless — the observed symptom being verifications collapsing
   * from about 100 matches to 6 with no change in how the finger was placed.
   *
   * So take the background, ask the sensor whether it saw a finger, and redo it
   * if so. Background first, detection second, matching the order the rest of
   * the driver uses.
   *
   * The presence test here is the ABSOLUTE one, the only usable choice, since
   * the relative baseline is precisely what we are about to establish. */
  if (!self->bg_frame)
    self->bg_frame = g_malloc (sizeof (guint16) * GOODIX_IMG_PIXELS);

  {
    int cur[GXFP_FDT_ZONE_COUNT];
    gint64 t0 = g_get_monotonic_time ();

    /* Production and diagnostics use the same invariant: calibration starts
     * only from a proven finger-off sample. The previous production path used
     * a 2 s timeout and then accepted a contaminated background anyway; on
     * reference MateBook that learned idle≈211 while the true finger-off level is≈353,
     * making every subsequent press invisible. Never do that. */
    if (!gx_wait_clean_anchor (self, &off_anchor_mean))
      return FALSE;

    if (capture_diagnostic)
      fp_info ("GXFP51A0 CAPTURE_DIAGNOSTIC_CLEAR_SENSOR_ANCHOR: mean=%d",
               off_anchor_mean);

    for (;;)
      {
        int clear_mean = off_anchor_mean;
        int mean;

        if (!gx_wait_sensor_clear (self, off_anchor_mean, &clear_mean))
          return FALSE;
        if (clear_mean > off_anchor_mean)
          off_anchor_mean = clear_mean;

        if (!gx_capture_avg (self, GOODIX_BG_FRAMES, self->bg_frame))
          return FALSE;

        /* A finger may land during the ~2 s background capture. Reject that
         * frame and wait for release rather than baking the finger into the
         * reference image. */
        guint8 touchflag = 0;

        if (gx_fdt_probe_ex (self, cur, &touchflag) != 0)
          return FALSE;

        mean = gx_fdt_mean (cur);
        if (mean > off_anchor_mean)
          off_anchor_mean = mean;

        if ((touchflag & 0x3fu) == 0 &&
            mean >= GOODIX_FDT_ABS &&
            off_anchor_mean - mean <= GX_DIAG_BASELINE_MAX_DRIFT)
          break;

        fp_warn ("GXFP51A0 background contaminated by touch: mean=%d anchor=%d touch=0x%02x; discarding",
                 mean, off_anchor_mean, touchflag);

        /* LAB ONLY, default off: the validated pre-S4 clean RAM background
         * may have been available before this image capture, but the sensor
         * was clear on our first early-reference check and the user placed
         * a finger DURING the background capture.  The original sequence
         * then spent repeated 15s cycles waiting for an empty sensor.
         *
         * Re-evaluate the existing age-bounded, independently validated
         * clean S3/S4 snapshot only on explicit opt-in.  This helper
         * rejects absent/partial finger and stale saved calibration, then
         * restores the old verified finger-OFF ImageBase, never the frame
         * contaminated above.  The next real TLS image, full image-quality
         * tests and unchanged matcher threshold 7 remain mandatory.
         *
         * If no trusted snapshot/full-contact proof exists, preserve the
         * old strict finger-off wait and rejection. No GPIO or PAM change. */
        if (g_strcmp0 (g_getenv ("GXFP_S4_MIDCAPTURE_REUSE_QA"), "1") == 0 &&
            gx_s3_try_early_finger_reference (self))
          {
            fp_warn ("GXFP51A0 S4_MIDCAPTURE_SAVED_REFERENCE strict prior "
                     "background restored after rejecting touched image");
            return TRUE;
          }

        /* Wait for a genuinely STABLE lift before spending another whole
         * image cycle; a quick release/re-press was the S3 failure pattern. */
        if (!gx_wait_sensor_clear_stable (self, off_anchor_mean, NULL,
                                          GX_CALIB_RETOUCH_DWELL_US))
          return FALSE;
      }

    self->bg_last_refresh_us = g_get_monotonic_time ();
    fp_info ("session ready in %d ms; fresh ImageBase armed",
             (int) ((self->bg_last_refresh_us - t0) / 1000));
  }

  /* Detection baseline. In one-shot diagnostic mode, validate it against the
   * no-finger anchor. If the user touched while those four probes were being
   * collected, discard them and retry instead of learning the finger as idle. */
  {
    int attempt;

    self->have_fdt = FALSE;
    for (attempt = 1; attempt <= GX_DIAG_BASELINE_ATTEMPTS; attempt++)
      {
        int t[GXFP_FDT_ZONE_COUNT];
        int acc[GXFP_FDT_ZONE_COUNT] = { 0 };
        int nb = 0;
        int baseline_mean;

        for (q = 0; q < 4; q++)
          {
            guint8 touchflag = 0;

            if (gx_fdt_probe_ex (self, t, &touchflag) == 0 &&
                (touchflag & 0x3fu) == 0 &&
                gx_fdt_mean (t) >= GOODIX_FDT_ABS)
              {
                for (k = 0; k < (int) GXFP_FDT_ZONE_COUNT; k++)
                  acc[k] += t[k];
                nb++;
              }
          }

        if (!nb)
          continue;

        for (k = 0; k < (int) GXFP_FDT_ZONE_COUNT; k++)
          self->fdt_base[k] = acc[k] / nb;
        baseline_mean = gx_fdt_mean (self->fdt_base);

        if (off_anchor_mean >= 0 &&
            off_anchor_mean - baseline_mean > GX_DIAG_BASELINE_MAX_DRIFT)
          {
              fp_warn ("GXFP51A0 FDT baseline contaminated by touch: mean=%d anchor=%d attempt=%d/%d",
                     baseline_mean, off_anchor_mean, attempt,
                     GX_DIAG_BASELINE_ATTEMPTS);
            if (!gx_wait_sensor_clear_stable (self, off_anchor_mean, NULL,
                                              GX_CALIB_RETOUCH_DWELL_US))
              return FALSE;
            continue;
          }

        if (baseline_mean > off_anchor_mean)
          off_anchor_mean = baseline_mean;

        self->have_fdt = TRUE;
        self->fdt_abs = baseline_mean - GOODIX_FDT_ABS_MARGIN;
        fp_info ("FDT auto-calibrated: idle mean=%d -> floor=%d (drop>%d)",
                 baseline_mean, self->fdt_abs, GOODIX_FDT_DROP);
        if (capture_diagnostic)
          fp_info ("GXFP51A0 CAPTURE_DIAGNOSTIC_BASELINE_STABLE: mean=%d anchor=%d",
                   baseline_mean, off_anchor_mean);
        break;
      }
  }

  if (capture_diagnostic && self->have_fdt &&
      !gx_diag_press_countdown (self, off_anchor_mean))
    return FALSE;

  if (self->have_fdt)
    {
      if (g_strcmp0 (g_getenv ("GXFP_HELD_FINGER_ROLLING_BG_QA"), "1") == 0)
        {
          /* The background was captured with no finger, and the following
           * four FDT baselines individually proved no-contact. Copy it
           * BEFORE any later biometric/TLS cleanup can fail. */
          gx_s3_snapshot_clean_bg (self);
        }
      else
        gx_s3_discard_clean_bg (self);
    }
  return self->have_fdt;
}

#define GX_PREPARE_ATTEMPTS 2
#define GX_BG_FRESH_MAX_US (G_GINT64_CONSTANT(20) * G_USEC_PER_SEC)
G_STATIC_ASSERT (GX_BG_FRESH_MAX_US == G_GINT64_CONSTANT(20000000));

static gint64
gx_background_age_us (FpiDeviceGoodix51A0 *self)
{
  gint64 now;

  if (!self->bg_frame || self->bg_last_refresh_us <= 0)
    return G_MAXINT64;
  now = g_get_monotonic_time ();
  return MAX (G_GINT64_CONSTANT(0), now - self->bg_last_refresh_us);
}

static gboolean
gx_background_is_fresh (FpiDeviceGoodix51A0 *self)
{
  return gx_background_age_us (self) <= GX_BG_FRESH_MAX_US;
}

static void
gx_invalidate_capture_context (FpiDeviceGoodix51A0 *self)
{
  self->production_ready = FALSE;
  self->warm_valid = FALSE;
  self->have_fdt = FALSE;
  self->bg_dirty = FALSE;
  self->bg_last_refresh_us = 0;
  self->tls_rxlen = 0;
  self->tls_rxpos = 0;
}

static gboolean
gx_recover_capture_context (FpiDeviceGoodix51A0 *self)
{
  gchar fw[64] = { 0 };
  g_autoptr(GError) reopen_error = NULL;

  gx_invalidate_capture_context (self);
  gx_tls_teardown (self);

  /* A timed-out TLS/image transaction can leave ST411 state coupled to the
   * currently open spidev file.  Do not reset through an already-open SPI
   * transport.  Close both SPI/IRQ handles, pulse the reviewed GPIO264 reset,
   * then open a completely fresh transport before proving the A8 boundary.
   *
   * This is deliberately NOT a sysfs unbind/rebind and needs no kernel,
   * distro, udev or desktop-specific helper.  It is the narrowest native
   * userspace equivalent to a cold device boundary and is safe to use inside
   * a live libfprint operation. */
  gx_transport_close (self);
  if (self->s3_first_tls_pending)
    {
      fp_info ("GXFP51A0 NATIVE_S3_RECOVERY detached quiet before GPIO reset");
      g_usleep (G_USEC_PER_SEC);
    }
  /* A first S3 wake may legitimately use the proven 10 ms fast reset.
   * But if a full TLS/ImageBase session has already failed, repeating that
   * SAME short pulse does not advance recovery. The historical rel59/61
   * recovery only became dependable after a full, SPI-detached MCU reset.
   *
   * Keep protocol-defined resets inside TLS untouched (rel71.21 proved that
   * detaching SPI there breaks the sensor). This escalation applies ONLY to
   * the outer, confirmed S3 whole-session recovery with SPI/IRQ CLOSED.
   * Never replay an already-ACKed GET_IMAGE in the degraded TLS session. */
  if (self->s3_first_tls_pending)
    {
      fp_warn ("GXFP51A0 NATIVE_S3_LONG_RECOVERY full GPIO264 HIGH300/LOW600 "
               "after failed TLS/ImageBase; SPI/IRQ detached");
      gx_gpio_reset (self);
    }
  else
    gx_reset_detached_s3_boundary (self);

  if (self->s3_first_tls_pending)
    g_usleep (G_USEC_PER_SEC);

  if (!gx_transport_open (FP_DEVICE (self), &reopen_error))
    {
      fp_warn ("GXFP51A0 capture-context recovery could not reopen transport: %s",
               reopen_error ? reopen_error->message : "unknown error");
      return FALSE;
    }

  fp_warn ("GXFP51A0 RESET_TRACE transport reopened after detached GPIO264 reset");

  if (!gx_read_fw_version_stage2e (self, fw, sizeof fw) ||
      g_strcmp0 (fw, "GF_ST411SEC_APP_14115") != 0)
    {
      fp_warn ("GXFP51A0 capture-context recovery could not re-establish the firmware boundary");
      return FALSE;
    }

  fp_info ("GXFP51A0 capture-context recovery reset/A8 confirmed: %s", fw);
  /* The first quick S3 attempt failed; the native detached recovery is now
   * complete. The old rel59/61 code recovered real S3 with multiple normal
   * Goodix TLS protocol retries and, when appropriate, fresh staging.
   * Restore those retries for the NEXT attempt without clearing the S3
   * marker until actual image+FDT production readiness. */
  if (self->s3_first_tls_pending && !self->s3_rel61_tls_fallback)
    {
      self->s3_rel61_tls_fallback = TRUE;
      fp_warn ("GXFP51A0 NATIVE_S3_REL61_FALLBACK detached reset confirmed; next TLS uses historical complete bounded retries");
    }
  return TRUE;
}

static gboolean
gx_prepare_capture_context (FpiDeviceGoodix51A0 *self,
                            gboolean capture_diagnostic)
{
  int attempt;
  gboolean previous_pacing_suppression;
  gboolean ok = FALSE;

  /* Diagnostics are intentionally single-shot so protocol failures remain
   * visible to the research harness.  Production operations get a bounded
   * whole-session retry; each retry starts after a hard reset because a
   * missing TLS image after ACK may have advanced unknown sensor state. */
  if (capture_diagnostic ||
      g_getenv ("GXFP_DIAGNOSTIC_ONESHOT") ||
      g_getenv ("GXFP_DIAGNOSTIC_TLS_ONLY"))
    return gx_prepare_capture_context_once (self, capture_diagnostic);

  /* Context preparation is lifecycle/calibration work, not biometric
   * performance evidence.  A GET_IMAGE miss while building background/FDT
   * must request recovery but must never slow the capture gap used by the
   * first real finger.  Preserve an already-adapted scale from an earlier
   * genuine biometric capture; only suppress further adaptation here. */
  previous_pacing_suppression = self->capture_pacing_suppressed;
  self->capture_pacing_suppressed = TRUE;

  for (attempt = 1; attempt <= GX_PREPARE_ATTEMPTS; attempt++)
    {
      GCancellable *c = gx_action_cancellable_if_active (self);

      if (c && g_cancellable_is_cancelled (c))
        goto out;

      if (gx_prepare_capture_context_once (self, FALSE))
        {
          self->capture_recovery_pending = FALSE;
          ok = TRUE;
          goto out;
        }

      fp_warn ("GXFP51A0 capture-context preparation failed attempt=%d/%d",
               attempt, GX_PREPARE_ATTEMPTS);

      if (!gx_recover_capture_context (self))
        break;
    }

out:
  self->capture_pacing_suppressed = previous_pacing_suppression;
  return ok;
}

static gboolean
gx_session_start (FpiDeviceGoodix51A0 *self)
{
  gboolean capture_diagnostic =
    g_getenv ("GXFP_DIAGNOSTIC_CAPTURE_ONCE") != NULL;

  /* A Verify/Identify operation may already be open when the machine enters
   * deep S3. In that case fprintd does not necessarily close/reopen the device,
   * so gx_dev_open() never gets a chance to invalidate stale TLS/FDT state.
   * Recover natively inside the existing operation, before any post-resume FDT
   * or GET_IMAGE command can hit the dead MCU session. */
  if (self->force_cold_reset)
    {
      fp_warn ("GXFP51A0 active resume recovery: rebuilding cold sensor "
               "context before continuing authentication");
      gx_warm_abandon (self);
      self->capture_recovery_pending = FALSE;
      self->capture_gap_scale = 0;
      self->capture_clean_streak = 0;
      self->capture_retry_seen = FALSE;
      self->capture_pacing_suppressed = TRUE;

      /* Preserve the rel71.48 S3 park/TLS fallback, but never reset an
       * ST411 session through an open SPI/IRQ transport. The established
       * recovery path detaches the fds and verifies the firmware boundary. */
      if (!gx_recover_capture_context (self))
        return FALSE;

      if (!gx_cold_prepare (self) || !gx_wakeup_mcu (self))
        return FALSE;

      self->force_cold_reset = FALSE;
      return TRUE;
    }

  if (self->capture_recovery_pending)
    {
      fp_info ("GXFP51A0 rebuilding capture context after transport desync");
      if (!gx_recover_capture_context (self))
        return FALSE;
    }

  /* Diagnostics deliberately retain their interactive preparation so the
   * research harness can observe each boundary. Normal fprintd clients are
   * prepared during device open/Claim, before EnrollStart or VerifyStart is
   * returned to the desktop. */
  if (capture_diagnostic || g_getenv ("GXFP_DIAGNOSTIC_ONESHOT") ||
      g_getenv ("GXFP_DIAGNOSTIC_TLS_ONLY"))
    {
      if (!gx_prepare_capture_context (self, capture_diagnostic))
        return FALSE;
      return g_getenv ("GXFP_DIAGNOSTIC_TLS_ONLY") != NULL ||
             gx_wakeup_mcu (self);
    }

  if (self->production_ready && self->tls_up && self->have_fdt &&
      self->bg_frame)
    {
      /* FAST_READY proves that the retained TLS/FDT transport is alive, but
       * ImageBase drifts much faster than the TLS lifecycle. reference MateBook 13 traces
       * show that a background only tens of seconds old can collapse genuine
       * scores from double digits to 2-4. Keep Claim sub-second, then refresh
       * the imaging reference here on the existing SESSION worker before
       * biometric READY. If the user is already touching the sensor,
       * gx_warm_validate() safely defers the refresh and the bounded reposition
       * path remains available. */
      if (!gx_background_is_fresh (self))
        {
          gint64 age_us = gx_background_age_us (self);
          gint64 previous_refresh = self->bg_last_refresh_us;

          fp_warn ("GXFP51A0 AUTH_TRACE ImageBase age=%d ms exceeds %d ms; "
                   "refreshing before biometric READY",
                   age_us == G_MAXINT64 ? -1 : (int) (age_us / 1000),
                   (int) (GX_BG_FRESH_MAX_US / 1000));

          if (!gx_warm_validate (self))
            {
              fp_warn ("GXFP51A0 AUTH_TRACE stale ImageBase refresh failed; "
                       "rebuilding cold capture context");
              gx_warm_abandon (self);
              if (!gx_recover_capture_context (self) ||
                  !gx_cold_prepare (self))
                return FALSE;
            }
          else if (self->bg_last_refresh_us > previous_refresh)
            fp_warn ("GXFP51A0 AUTH_TRACE fresh ImageBase ready before biometric READY");
          else
            fp_warn ("GXFP51A0 AUTH_TRACE ImageBase refresh deferred by touch; "
                     "using retained reference for this pose");
        }

      fp_info ("GXFP51A0 reusing capture context prepared during device open");
      return gx_wakeup_mcu (self);
    }

  /* Defensive fallback for non-fprintd clients that may reach an operation
   * without a normal Claim/Open lifecycle. */
  if (!gx_prepare_capture_context (self, FALSE))
    return FALSE;

  self->capture_recovery_pending = FALSE;
  self->production_ready = TRUE;
  return gx_wakeup_mcu (self);
}



/* Pre-processing: subtract the background, then remove row and column
 * banding with a median. */

static double *
gx_preprocess (FpiDeviceGoodix51A0 *self, const guint16 *px)
{
  const int n = GOODIX_IMG_PIXELS;
  double *out = g_new (double, n);
  g_autofree double *sorted = g_new (double, n);
  double pos, frac, p54;
  int lo, hi;

  /* Exact-target GXFP51A0 imaging: frame-background has a drifting DC
   * component.  Recenter at the ~54th percentile, clip negatives, and keep
   * one physical frame per press.  The matcher wrapper performs the final
   * percentile stretch and unsharp mask. */
  for (int i = 0; i < n; i++)
    {
      out[i] = (double) px[i] - self->bg_frame[i];
      sorted[i] = out[i];
    }

  qsort (sorted, n, sizeof *sorted, gx_cmp_dbl);
  pos = 0.54 * (double) (n - 1);
  lo = (int) floor (pos);
  hi = (int) ceil (pos);
  frac = pos - lo;
  p54 = sorted[lo] * (1.0 - frac) + sorted[hi] * frac;

  for (int i = 0; i < n; i++)
    out[i] = MAX (0.0, out[i] - p54);

  return out;
}

/* Is a finger actually down? Costs about 33 ms. */
static gboolean
gx_finger_present (FpiDeviceGoodix51A0 *self)
{
  int cur[GXFP_FDT_ZONE_COUNT];
  guint8 touchflag = 0;

  if (gx_fdt_probe_ex (self, cur, &touchflag) != 0)
    return FALSE;
  return gx_fdt_touch_is_finger (touchflag) ||
         gx_fdt_drop (self->fdt_base, cur) > GOODIX_FDT_DROP ||
         gx_fdt_mean (cur) < self->fdt_abs;
}

/* Windows OnRetryCaptureIMG keeps the current physical press alive:
 * FDT-manual first confirms that the finger is still down, then another
 * image command 0x20 is issued without waiting for a lift/new FDT-down IRQ.
 * This is deliberately a new biometric image, not the transport-level
 * resend performed inside gx_send_plain_drain(). */
static gboolean
gx_capture_retry_same_press_frame (FpiDeviceGoodix51A0 *self,
                                   guint16             *px,
                                   gboolean            *finger_still_down)
{
  struct gxfp_target_packet packet;
  g_autofree guint8 *rec = g_malloc0 (GOODIX_RX_MAX);
  g_autofree guint8 *plain = g_malloc0 (GOODIX_RX_MAX);
  int cur[GXFP_FDT_ZONE_COUNT];
  guint8 touchflag = 0;
  int raw;
  gssize got;

  g_return_val_if_fail (finger_still_down != NULL, FALSE);
  *finger_still_down = FALSE;

  if (gx_fdt_probe_ex (self, cur, &touchflag) != 0)
    {
      fp_warn ("same-press RetryCaptureIMG FDT-manual failed; stopping retries");
      return FALSE;
    }

  {
    int mean = gx_fdt_mean (cur);
    int drop = gx_fdt_drop (self->fdt_base, cur);

    fp_warn ("GXFP51A0 VERIFY_TRACE same-press FDT mean=%d drop=%d "
             "floor=%d threshold_drop=%d touch=0x%02x zones=%u",
             mean, drop, self->fdt_abs, GOODIX_FDT_DROP,
             touchflag, gx_fdt_touch_count (touchflag));
    if (!(gx_fdt_touch_is_finger (touchflag) ||
          drop > GOODIX_FDT_DROP || mean < self->fdt_abs))
      {
        fp_warn ("GXFP51A0 VERIFY_TRACE RetryCaptureIMG stopped: "
                 "finger not detected on current press");
        return TRUE;
      }
  }

  *finger_still_down = TRUE;

  /* The Windows-style RetryCaptureIMG path is a fresh sensor image request,
   * not a transport replay.  FDT-manual has just completed a separate MCU
   * transaction; give the sensor the same session-calibrated inter-command
   * settling window used by the normal capture recipe before issuing 0x20.
   *
   * On the reference MateBook, successful rel56 traces had later same-press
   * images accepted on their first GET_IMAGE, while failing rel58 traces sent
   * the retry immediately after FDT and every image needed the no-evidence
   * transport replay.  This gap is process-local, bounded to 30..90 ms and
   * never changes biometric scoring. */
  fp_warn ("GXFP51A0 VERIFY_TRACE RetryCaptureIMG pacing barrier=%u us",
           gx_capture_gap_us (self));
  g_usleep (gx_capture_gap_us (self));

  if (!gxfp_build_get_image (&packet))
    return FALSE;

  self->capture_retry_seen = FALSE;
  if (!gx_send_plain_drain (self, packet.inner, packet.inner_len, NULL, NULL))
    return FALSE;

  raw = gx_take_tls_frame (self, rec, GOODIX_RX_MAX);
  if (raw < 0)
    raw = gx_fail_get_image_after_tls_timeout (self);

  got = raw > 0
          ? gx_tls_decrypt_record (self->tls, rec, (gsize) raw,
                                   plain, GOODIX_RX_MAX)
          : -1;
  if (got != (gssize) GXFP_IMAGE_PLAINTEXT_LEN)
    {
      fp_warn ("same-press RetryCaptureIMG failed (%d raw, %ld plain); "
               "forcing full session recovery",
               raw, (long) got);
      if (!self->capture_recovery_pending)
        gx_capture_transport_desync (self);
      return FALSE;
    }

  if (!gxfp_decode_image_plaintext (plain, (gsize) got, px))
    {
      fp_warn ("same-press RetryCaptureIMG could not decode transport raster");
      return FALSE;
    }

  return TRUE;
}


#ifdef GXFP51A0_DEVELOPER
/* Dumps captures for offline evaluation. Enabled simply by the dump directory
 * existing, rather than by an environment variable, which cannot conveniently
 * be set on an already-running systemd service.
 *
 * pN.bin holds the raw frame as 16-bit samples and pN.bin.bg the matching
 * background, so that recorded sets can be replayed through the offline
 * matcher bench. */
#define GX_DUMP_DIR "/run/goodix51a0/dump"

static void
gx_dump_capture (FpiDeviceGoodix51A0 *self, const guint16 *px)
{
  static int seq = 0;
  g_autofree gchar *p = NULL, *b = NULL;

  if (!g_file_test (GX_DUMP_DIR, G_FILE_TEST_IS_DIR) || !self->bg_frame)
    return;

  /* Continue past captures already there, so a restart does not overwrite
   * an earlier collection run. */
  do
    {
      g_free (p);
      p = g_strdup_printf ("%s/p%03d.bin", GX_DUMP_DIR, ++seq);
    }
  while (g_file_test (p, G_FILE_TEST_EXISTS) && seq < 9999);

  b = g_strdup_printf ("%s.bg", p);
  if (g_file_set_contents_full (
        p, (const gchar *) px,
        sizeof (guint16) * GOODIX_IMG_PIXELS,
        G_FILE_SET_CONTENTS_CONSISTENT, 0600, NULL) &&
      g_file_set_contents_full (
        b, (const gchar *) self->bg_frame,
        sizeof (guint16) * GOODIX_IMG_PIXELS,
        G_FILE_SET_CONTENTS_CONSISTENT, 0600, NULL))
    fp_info ("capture saved: %s", p);
}

#endif

static GxSiftFeatures *
gx_features_from_pixels (FpiDeviceGoodix51A0 *self, const guint16 *px)
{
  g_autofree double *img = NULL;
  double m = 0, v = 0;
  int i;

  img = gx_preprocess (self, px);

  /* This quality gate is ESSENTIAL. The keypoint detector always returns
   * maxima, even on pure noise, so the number of points says nothing about
   * whether a capture is usable. What separates them is contrast: about 5
   * with no finger against about 200 with one. */
  for (i = 0; i < GOODIX_IMG_PIXELS; i++)
    m += img[i];
  m /= GOODIX_IMG_PIXELS;
  for (i = 0; i < GOODIX_IMG_PIXELS; i++)
    { double d = img[i] - m; v += d * d; }
  v = sqrt (v / GOODIX_IMG_PIXELS);
  if (v < GOODIX_FINGER_STD / 3.0)
    {
      fp_info ("capture rejected: contrast %.0f (no finger?)", v);
      return NULL;
    }

  return gx_sift_extract (img, GOODIX_IMG_WIDTH, GOODIX_IMG_HEIGHT);
}

/* Captures one press and extracts its descriptors. */
static GxSiftFeatures *
gx_capture_features (FpiDeviceGoodix51A0 *self)
{
  guint16 px[GOODIX_IMG_PIXELS];

  if (!gx_capture_frame (self, px, FALSE))
    return NULL;
  return gx_features_from_pixels (self, px);
}

/* ------------------------------------------------------------------ */
/*  Serialising a set of views into an FpPrint                         */
/* ------------------------------------------------------------------ */

/* Driver-private template schema. Increment this whenever the serialized
 * descriptor representation or matching semantics become incompatible.
 *
 * Versioned templates make future driver upgrades explicit: a newer driver
 * can request re-enrollment instead of silently interpreting stale biometric
 * data with changed semantics. Legacy aay templates from the development
 * harness remain readable so current research data does not get stranded. */
#define GX_TEMPLATE_VERSION 4u

static GVariant *
gx_views_to_variant (GPtrArray *views)
{
  GVariantBuilder b;
  GVariant *payload;
  guint i;

  g_variant_builder_init (&b, G_VARIANT_TYPE ("aay"));
  for (i = 0; i < views->len; i++)
    {
      g_autoptr(GByteArray) ba = gx_sift_serialize (g_ptr_array_index (views, i));
      g_variant_builder_add_value (
        &b, g_variant_new_fixed_array (G_VARIANT_TYPE_BYTE, ba->data, ba->len, 1));
    }

  payload = g_variant_builder_end (&b);
  return g_variant_new ("(u@aay)", GX_TEMPLATE_VERSION, payload);
}

static GPtrArray *
gx_views_from_print (FpPrint *print)
{
  g_autoptr(GVariant) data = NULL;
  g_autoptr(GVariant) payload = NULL;
  GPtrArray *views = g_ptr_array_new_with_free_func ((GDestroyNotify) gx_sift_free);
  GVariantIter it;
  GVariant *child;
  guint32 version = 0;

  g_object_get (print, "fpi-data", &data, NULL);
  if (!data)
    return views;

  if (g_variant_is_of_type (data, G_VARIANT_TYPE ("(uaay)")))
    {
      g_variant_get (data, "(u@aay)", &version, &payload);
      if (version != GX_TEMPLATE_VERSION)
        {
          fp_warn ("unsupported template version %u (driver expects %u); re-enrollment required",
                   version, GX_TEMPLATE_VERSION);
          return views;
        }
    }
  else
    {
      fp_warn ("unsupported template payload type %s",
               g_variant_get_type_string (data));
      return views;
    }

  g_variant_iter_init (&it, payload);
  while ((child = g_variant_iter_next_value (&it)))
    {
      gsize len = 0;
      const guint8 *raw = g_variant_get_fixed_array (child, &len, 1);
      GxSiftFeatures *f = gx_sift_deserialize (raw, len);
      if (f)
        g_ptr_array_add (views, f);
      g_variant_unref (child);
    }
  return views;
}

/* ------------------------------------------------------------------ */
/*  Enrolment and verification                                         */
/*                                                                     */
/*  A libfprint driver must not block the main loop: fprintd answers    */
/*  D-Bus calls while an operation is in flight. Waiting for the finger */
/*  is therefore done with successive detection polls driven by a       */
/*  timeout, each returning within about 33 ms.                        */
/* ------------------------------------------------------------------ */

typedef struct
{
  GPtrArray *views;        /* descriptors accumulated while enrolling */
  GxSiftFeatures *probe;   /* descriptors of the capture being verified */
  int        best;         /* best score reached across attempts */
  int        tries;        /* captures attempted for this verification */
  int        stage;        /* vue en cours */
  int        polls;        /* polls done in the current state */
  gboolean   verifying;       /* verify or identify: capture exactly one usable press */
  gboolean   cold_contact_gate; /* first cold-start auth: short FDT placement stabilization only */
  gint64     contact_since_us;  /* monotonic first contact; reset between physical presses */
  gboolean   identifying;     /* one-to-many gallery match */
  gboolean   match_reported;  /* terminal match/no-match already sent early */
  gboolean   rearm_mcu_before_retry; /* failed usable press: refresh imaging after lift */
  gboolean   rebase_before_retry; /* finger is off: refresh background/FDT before next pose */
  FpPrint   *identify_match;  /* matched gallery print, owned while the task lives */
} GxTask;

enum {
  GX_ST_SESSION,       /* init TLS + fond + ligne de base FDT */
  GX_ST_WAIT_ON,       /* wait for the finger, polling without blocking */
  GX_ST_CAPTURE,       /* capture, then extract descriptors */
  GX_ST_WAIT_OFF,      /* wait for the finger to be lifted */
  GX_ST_DONE,
  GX_ST_NUM,
};

#define GX_POLL_MS     100     /* detection polling period */
#define GX_POLL_MAX    300     /* about 30 s before giving up */
#define GX_POLL_OFF    100     /* about 10 s to wait for release */
#define GX_RECOVERY_OFF_POLLS 2 /* ~1 s worst-case with failed FDT probes */
#define GX_VERIFY_MAX_ATTEMPTS 3 /* physical presses; fixed budget */

/* Only at initial cold login: an FDT threshold can trigger on 2/6 zones
 * before the finger reaches the full 5/6 coverage, wasting one of the three
 * physical attempts. Retain the exact same per-image matcher threshold.
 * A partial but strong persistent touch is allowed after 600ms so this never
 * becomes an indefinite 5-zone-only gate; 1-2 zones cannot trigger a bad
 * partial capture. Ordinary warm LOCK and the S3 calibration are UNCHANGED. */
#define GX_COLD_CONTACT_READY_US G_GINT64_CONSTANT(100000)
#define GX_COLD_PARTIAL_GRACE_US G_GINT64_CONSTANT(600000)
static gboolean
gx_cold_contact_gate_ready (gboolean contact, guint zones,
                            gint64 now_us, gint64 *first_us)
{
  if (!contact)
    {
      *first_us = 0;
      return FALSE;
    }
  if (*first_us == 0)
    {
      *first_us = now_us;
      return FALSE;
    }
  gint64 elapsed_us = now_us - *first_us;
  return (zones >= 5 && elapsed_us >= GX_COLD_CONTACT_READY_US) ||
         (zones >= 3 && elapsed_us >= GX_COLD_PARTIAL_GRACE_US);
}
#define GX_SAME_PRESS_CAPTURE_ATTEMPTS 3 /* initial image + up to 2 quality-only recaptures */

/* ------------------------------------------------------------------ */
/*  Off-loading the blocking work                                      */
/* ------------------------------------------------------------------ */

/* Two steps take about a second each and cannot be split: establishing the
 * session (TLS handshake plus a background capture) and capturing an image.
 * Both go through OpenSSL's blocking API over a synchronous SPI file
 * descriptor, which cannot be pumped from callbacks without restructuring the
 * whole TLS layer.
 *
 * Run inline they froze the main loop, so fprintd stopped answering D-Bus for
 * the duration. Since libfprint runs one operation at a time and the state
 * machine waits for the completion callback, nothing else touches the device
 * while the worker runs, which makes a worker thread safe here; libfprint's own
 * secugen driver off-loads its heavy work the same way.
 *
 * Only these two steps move off the main loop. Finger detection stays on it: at
 * roughly 33 ms a poll it is short enough not to be felt, and keeping it there
 * avoids any cross-thread access to the sensor between captures. */

typedef struct
{
  FpiSsm         *ssm;
  FpDevice       *dev;
  FpPrint        *verify_print;    /* worker-owned ref for 1:1 scoring */
  GPtrArray      *identify_gallery; /* worker-owned FpPrint refs for 1:N */
  GxSiftFeatures *feat;            /* best capture result, NULL on failure */
  guint           same_press_images;
  int             worker_score;
  guint           worker_candidate, worker_base, worker_adapt;
  gboolean        ok;
} GxWork;

static GxSiftFeatures *
gx_capture_auth_same_press (FpiDeviceGoodix51A0 *self,
                            FpPrint              *tmpl,
                            GPtrArray            *gallery,
                            guint                *out_images,
                            int                  *out_score,
                            guint                *out_candidate,
                            guint                *out_base,
                            guint                *out_adapt);

static void
gx_work_free (GxWork *w)
{
  g_clear_object (&w->verify_print);
  g_clear_pointer (&w->identify_gallery, g_ptr_array_unref);
  g_clear_pointer (&w->feat, gx_sift_free);
  g_free (w);
}

static void
gx_session_thread (GTask *task, gpointer src, gpointer data, GCancellable *c)
{
  GxWork *w = data;
  FpiDeviceGoodix51A0 *self = FPI_DEVICE_GOODIX51A0 (w->dev);
  GxTask *t = fpi_ssm_get_data (w->ssm);

  if (t && t->rebase_before_retry)
    {
      t->rebase_before_retry = FALSE;

      /* A biometric miss does not itself make ImageBase stale. rel71.8 live
       * traces showed that blindly rebasing after each weak placement wasted
       * ~1.7-1.9 s and was often contaminated by the user's next placement,
       * even though the reference had been refreshed only seconds earlier.
       * Rebase only when ImageBase has actually exceeded the freshness bound;
       * otherwise rearm immediately for the next physical placement. */
      if (gx_background_is_fresh (self))
        {
          fp_info ("GXFP51A0 AUTH_TRACE retry ImageBase still fresh age=%d ms; "
                   "rearming MCU immediately for retry %d/%d",
                   (int) (gx_background_age_us (self) / 1000),
                   t->tries + 1, GX_VERIFY_MAX_ATTEMPTS);
          w->ok = gx_wakeup_mcu (self);
        }
      else
        {
          gint64 previous_refresh = self->bg_last_refresh_us;

          fp_warn ("GXFP51A0 AUTH_TRACE refreshing stale background+FDT after "
                   "failed usable press before retry %d/%d",
                   t->tries + 1, GX_VERIFY_MAX_ATTEMPTS);

          if (gx_warm_validate (self))
            {
              if (self->bg_last_refresh_us > previous_refresh)
                fp_warn ("GXFP51A0 AUTH_TRACE fresh retry background ready; "
                         "rearming MCU before retry %d/%d",
                         t->tries + 1, GX_VERIFY_MAX_ATTEMPTS);
              else
                fp_warn ("GXFP51A0 AUTH_TRACE retry ImageBase refresh deferred "
                         "by touch; rearming with retained reference for retry "
                         "%d/%d",
                         t->tries + 1, GX_VERIFY_MAX_ATTEMPTS);
              w->ok = gx_wakeup_mcu (self);
            }
          else
            {
              /* A failed refresh is a transport/lifecycle problem, not
               * biometric evidence. Let the existing bounded session recovery
               * decide whether a cold rebuild is required. */
              fp_warn ("GXFP51A0 AUTH_TRACE retry background refresh failed; "
                       "using bounded session recovery");
              w->ok = gx_session_start (self);
            }
        }
    }
  else if (g_getenv ("GXFP_DIAGNOSTIC_TLS_ONLY"))
    w->ok = gx_tls_session (self);
  else
    w->ok = gx_session_start (self);
  g_task_return_boolean (task, TRUE);
}

static void
gx_capture_thread (GTask *task, gpointer src, gpointer data, GCancellable *c)
{
  GxWork *w = data;
  FpiDeviceGoodix51A0 *self = FPI_DEVICE_GOODIX51A0 (w->dev);

  w->worker_score = -1;
  if (w->verify_print || w->identify_gallery)
    w->feat = gx_capture_auth_same_press (self, w->verify_print,
                                          w->identify_gallery,
                                          &w->same_press_images,
                                          &w->worker_score,
                                          &w->worker_candidate,
                                          &w->worker_base,
                                          &w->worker_adapt);
  else
    w->feat = gx_capture_features (self);
  w->ok = (w->feat != NULL);
  g_task_return_boolean (task, TRUE);
}

/* libfprint's logind suspend callback cancels its operation and awaits the
 * task's completion before releasing its delay inhibitor.  Our TLS/image
 * worker runs synchronously on a separate thread; do NOT drive the next SSM
 * step after that worker returns if the operation was cancelled meanwhile.
 * Check on the main thread, after the worker's mandatory capture cleanup, so
 * no SPI FDs can be closed while the worker is using them. */
static gboolean gx_cancelled (FpDevice *dev, FpiSsm *ssm);

static void
gx_session_done (GObject *src, GAsyncResult *res, gpointer user_data)
{
  GxWork *w = g_task_get_task_data (G_TASK (res));
  FpiDeviceGoodix51A0 *self = FPI_DEVICE_GOODIX51A0 (w->dev);

  if (gx_cancelled (w->dev, w->ssm))
    {
      fp_warn ("GXFP51A0 S3_CANCEL session worker finished after suspend cancellation; do not rearm");
      return;
    }

  if (!w->ok)
    {
      fpi_ssm_mark_failed (w->ssm, fpi_device_error_new_msg (
        FP_DEVICE_ERROR_PROTO, "sensor initialisation failed"));
      return;
    }
  if (g_getenv ("GXFP_DIAGNOSTIC_TLS_ONLY"))
    {
      fp_info ("GXFP51A0 TLS-only diagnostic complete");
      fpi_ssm_mark_failed (w->ssm, fpi_device_error_new_msg (
        FP_DEVICE_ERROR_GENERAL, "TLS-only diagnostic complete"));
      return;
    }
  if (self->bg_dirty)
    {
      /* Better to ask for the finger to be removed than to return a collapsed
       * score the user could not possibly explain. */
      fpi_ssm_mark_failed (w->ssm,
        fpi_device_retry_new (FP_DEVICE_RETRY_REMOVE_FINGER));
      return;
    }

  if (g_getenv ("GXFP_DIAGNOSTIC_CAPTURE_ONCE"))
    fp_info ("GXFP51A0 CAPTURE_DIAGNOSTIC_READY: PRESS_AND_HOLD_FINGER");
  fpi_ssm_next_state (w->ssm);
}

/* Runs @fn on a worker thread, then @done on the main loop. */
static void
gx_run_async (FpiSsm *ssm, FpDevice *dev, GTaskThreadFunc fn,
              GAsyncReadyCallback done)
{
  GTask *task = g_task_new (dev, NULL, done, NULL);
  GxWork *w = g_new0 (GxWork, 1);
  GxTask *t = fpi_ssm_get_data (ssm);

  w->ssm = ssm;
  w->dev = dev;

  /* fprintd VerifyStart("any") may use libfprint Identify when the driver
   * advertises it.  Both 1:1 Verify and 1:N Identify therefore need the exact
   * Windows same-press recapture path. Copy only refs on the main thread; the
   * worker treats FpPrint objects as immutable. */
  if (fn == gx_capture_thread && t && t->verifying)
    {
      if (t->identifying)
        {
          GPtrArray *gallery = NULL;

          fpi_device_get_identify_data (dev, &gallery);
          if (gallery)
            {
              w->identify_gallery =
                g_ptr_array_new_with_free_func ((GDestroyNotify) g_object_unref);
              for (guint i = 0; i < gallery->len; i++)
                g_ptr_array_add (w->identify_gallery,
                                 g_object_ref (g_ptr_array_index (gallery, i)));
            }
        }
      else
        {
          FpPrint *tmpl = NULL;

          fpi_device_get_verify_data (dev, &tmpl);
          if (tmpl)
            w->verify_print = g_object_ref (tmpl);
        }
    }

  g_task_set_task_data (task, w, (GDestroyNotify) gx_work_free);
  g_task_run_in_thread (task, fn);
  g_object_unref (task);
}

static void
gx_task_free (GxTask *t)
{
  if (!t)
    return;
  if (t->views)
    g_ptr_array_free (t->views, TRUE);
  g_clear_pointer (&t->probe, gx_sift_free);
  g_clear_object (&t->identify_match);
  g_free (t);
}


/* libfprint provides a GCancellable per operation; that is how fprintd asks us
 * to stop when the user dismisses the prompt or picks the password instead.
 * Without checking it the polls run to their limit, roughly 30 seconds, the
 * stop call times out and the sensor stays busy. */
static gboolean
gx_cancelled (FpDevice *dev, FpiSsm *ssm)
{
  GCancellable *c = gx_action_cancellable_if_active (FPI_DEVICE_GOODIX51A0 (dev));

  if (!c || !g_cancellable_is_cancelled (c))
    return FALSE;
  fp_info ("operation cancelled by the caller");
  fpi_device_report_finger_status (dev, FP_FINGER_STATUS_NONE);
  fpi_ssm_mark_failed (ssm, g_error_new (G_IO_ERROR, G_IO_ERROR_CANCELLED,
                                         "operation cancelled"));
  return TRUE;
}

/* Handle the real desktop failure mode: the lockscreen may Claim/Open the
 * reader before suspend and keep that same operation alive across S3.  Compare
 * CLOCK_BOOTTIME with CLOCK_MONOTONIC on every lightweight finger poll; the
 * delta advances only while suspended.  Recovery stays entirely inside
 * libfprint, so no systemd/logind/D-Bus helper races the password PAM stack. */
static gboolean
gx_active_sleep_recovery (FpDevice *dev, FpiSsm *ssm)
{
  FpiDeviceGoodix51A0 *self = FPI_DEVICE_GOODIX51A0 (dev);

  if (!gx_warm_crossed_sleep (self))
    return FALSE;

  fp_warn ("GXFP51A0 active S3 boundary detected during authentication; "
           "scheduling native cold recovery");
  self->force_cold_reset = TRUE;
  self->capture_recovery_pending = FALSE;
  self->capture_gap_scale = 0;
  self->capture_clean_streak = 0;
  self->capture_retry_seen = FALSE;
  self->capture_pacing_suppressed = TRUE;

  fpi_device_report_finger_status (dev, FP_FINGER_STATUS_NONE);
  self->poll_id = 0;
  fpi_ssm_jump_to_state (ssm, GX_ST_SESSION);
  return TRUE;
}

/* Non-blocking poll, re-armed by timeout until the finger is detected. */
static gboolean
gx_poll_on (gpointer user_data)
{
  FpiSsm *ssm = user_data;
  FpDevice *dev = fpi_ssm_get_device (ssm);
  FpiDeviceGoodix51A0 *self = FPI_DEVICE_GOODIX51A0 (dev);
  GxTask *t = fpi_ssm_get_data (ssm);
  int cur[GXFP_FDT_ZONE_COUNT];
  guint8 touchflag = 0;

  if (gx_cancelled (dev, ssm))
    { self->poll_id = 0; return G_SOURCE_REMOVE; }

  if (gx_active_sleep_recovery (dev, ssm))
    return G_SOURCE_REMOVE;

  if (gx_fdt_probe_ex (self, cur, &touchflag) != 0)
    {
      fp_dbg ("wait-on: detection probe failed");
      goto again;
    }

  /* Reuse the values just read; probing twice per poll perturbs the sensor. */
  if (t->polls % 5 == 0)
    {
      fp_info ("wait-on: mean=%d drop=%d (thresholds %d / %d)",
               gx_fdt_mean (cur), gx_fdt_drop (self->fdt_base, cur),
               self->fdt_abs, GOODIX_FDT_DROP);
      if (g_getenv ("GXFP_DIAGNOSTIC_CAPTURE_ONCE"))
        fp_info ("GXFP51A0 CAPTURE_DIAGNOSTIC_WAITING_FOR_FINGER: PRESS_AND_HOLD_NOW");
    }

  gboolean contact = gx_fdt_touch_is_finger (touchflag) ||
                     gx_fdt_drop (self->fdt_base, cur) > GOODIX_FDT_DROP ||
                     gx_fdt_mean (cur) < self->fdt_abs;
  if (t->verifying && t->cold_contact_gate &&
      !gx_cold_contact_gate_ready (contact, gx_fdt_touch_count (touchflag),
                                   g_get_monotonic_time (), &t->contact_since_us))
    goto again;

  if (contact)
    {
      /* Keep NEEDED asserted: the capture still takes about 1.1 s and the
       * finger must stay down for it. This flag is what lets a user interface
       * say when to lift — without it people lift on detection, one second too
       * early, and spoil the capture. */
      fpi_device_report_finger_status (dev, FP_FINGER_STATUS_NEEDED |
                                            FP_FINGER_STATUS_PRESENT);
      if (t->verifying)
        fp_warn ("GXFP51A0 %s_TRACE physical press %d/%d DETECTED_HOLD "
                 "touch=0x%02x zones=%u mean=%d drop=%d",
                 t->identifying ? "IDENTIFY" : "VERIFY",
                 t->tries + 1, GX_VERIFY_MAX_ATTEMPTS,
                 touchflag, gx_fdt_touch_count (touchflag),
                 gx_fdt_mean (cur), gx_fdt_drop (self->fdt_base, cur));
      fp_info ("finger status: needed=1 present=1; capture starting");
      self->poll_id = 0;
      fpi_ssm_jump_to_state (ssm, GX_ST_CAPTURE);
      return G_SOURCE_REMOVE;
    }
again:
  if (++t->polls > GX_POLL_MAX)
    {
      self->poll_id = 0;
      fpi_ssm_mark_failed (ssm, fpi_device_retry_new (FP_DEVICE_RETRY_GENERAL));
      return G_SOURCE_REMOVE;
    }
  return G_SOURCE_CONTINUE;
}

static gboolean
gx_poll_off (gpointer user_data)
{
  FpiSsm *ssm = user_data;
  FpDevice *dev = fpi_ssm_get_device (ssm);
  FpiDeviceGoodix51A0 *self = FPI_DEVICE_GOODIX51A0 (dev);
  GxTask *t = fpi_ssm_get_data (ssm);
  int cur[GXFP_FDT_ZONE_COUNT];
  guint8 touchflag = 0;
  gboolean off = FALSE;

  if (gx_cancelled (dev, ssm))
    { self->poll_id = 0; return G_SOURCE_REMOVE; }

  if (gx_active_sleep_recovery (dev, ssm))
    return G_SOURCE_REMOVE;

  if (gx_fdt_probe_ex (self, cur, &touchflag) == 0 &&
      !gx_fdt_touch_is_finger (touchflag) &&
      gx_fdt_drop (self->fdt_base, cur) < GOODIX_FDT_DROP / 2 &&
      gx_fdt_mean (cur) >= self->fdt_abs)
    off = TRUE;

  if (off || ++t->polls >
             (self->capture_recovery_pending ? GX_RECOVERY_OFF_POLLS
                                             : GX_POLL_OFF))
    {
      fpi_device_report_finger_status (dev, FP_FINGER_STATUS_NONE);
      if (t->verifying)
        fp_warn ("GXFP51A0 %s_TRACE physical press %d/%d RELEASED",
                 t->identifying ? "IDENTIFY" : "VERIFY",
                 MAX (t->tries, 1), GX_VERIFY_MAX_ATTEMPTS);

      if (self->capture_recovery_pending)
        {
          fp_info ("GXFP51A0 finger released/timeout after transport desync; "
                   "rebuilding session before another biometric press");
          fpi_ssm_jump_to_state (ssm, GX_ST_SESSION);
        }
      /* Next enrollment view, fixed-budget verify retry, or finish. */
      else if (t->verifying && !t->match_reported &&
               t->tries < GX_VERIFY_MAX_ATTEMPTS)
        {
          if (t->rearm_mcu_before_retry)
            {
              /* A low-scoring but otherwise valid press can mean the retained
               * warm background/imaging state drifted.  Historical reference MateBook 13
               * runs show fresh WARM_REBASE frames restoring scores from 2-4
               * to 7-20+.  Refresh only now, after release, and do it on the
               * existing SESSION worker so the fprintd main loop stays live. */
              t->rearm_mcu_before_retry = FALSE;
              t->rebase_before_retry = TRUE;
              fp_warn ("GXFP51A0 AUTH_TRACE scheduling fresh background after "
                       "failed usable press before retry %d/%d",
                       t->tries + 1, GX_VERIFY_MAX_ATTEMPTS);
              self->poll_id = 0;
              fpi_ssm_jump_to_state (ssm, GX_ST_SESSION);
              return G_SOURCE_REMOVE;
            }

          fp_info ("verify: finger released; waiting for retry press %d/%d",
                   t->tries + 1, GX_VERIFY_MAX_ATTEMPTS);
          fpi_ssm_jump_to_state (ssm, GX_ST_WAIT_ON);
        }
      else if (t->verifying || t->stage >= GX_ENROLL_STAGES)
        fpi_ssm_jump_to_state (ssm, GX_ST_DONE);
      else
        fpi_ssm_jump_to_state (ssm, GX_ST_WAIT_ON);
      self->poll_id = 0;
      return G_SOURCE_REMOVE;
    }
  return G_SOURCE_CONTINUE;
}

static gboolean
gx_pixel_rescue_view_pass (const GxSiftFeatures *probe,
                           const GxSiftFeatures *enrolled,
                           int                   score,
                           int                  *out_inliers,
                           int                  *out_overlap,
                           int                  *out_zncc,
                           int                  *out_agree)
{
  int inliers = 0, overlap = 0, zncc = 0, agree = 0;

  if (!probe || !enrolled ||
      score < GX_PIXEL_RESCUE_SCORE_MIN ||
      score > GX_PIXEL_RESCUE_SCORE_MAX)
    return FALSE;

  if (!gx_sift_pixel_overlap_metrics (probe, enrolled,
                                      &inliers, &overlap,
                                      &zncc, &agree))
    return FALSE;

  if (out_inliers)
    *out_inliers = inliers;
  if (out_overlap)
    *out_overlap = overlap;
  if (out_zncc)
    *out_zncc = zncc;
  if (out_agree)
    *out_agree = agree;

  return overlap >= GX_PIXEL_RESCUE_MIN_OVERLAP &&
         zncc >= GX_PIXEL_RESCUE_MIN_ZNCC_MILLI &&
         agree >= GX_PIXEL_RESCUE_MIN_AGREE_PERMILLE;
}

static int
gx_score_probe_against_print (FpPrint *tmpl,
                              const GxSiftFeatures *probe,
                              guint *out_base_views,
                              guint *out_adapt_views)
{
  g_autoptr(GPtrArray) views = gx_views_from_print (tmpl);
  g_autofree int *view_scores = NULL;
  int best = 0;
  int top[5] = { 0, 0, 0, 0, 0 };
  guint top_idx[5] = { 0, 0, 0, 0, 0 };
  gboolean rescued = FALSE;
  guint rescue_idx = 0;
  int rescue_inliers = 0;
  int rescue_overlap = 0;
  int rescue_zncc = 0;
  int rescue_agree = 0;
  gboolean near_valid = FALSE;
  guint near_idx = 0;
  int near_score = 0;
  int near_inliers = 0;
  int near_overlap = 0;
  int near_zncc = 0;
  int near_agree = 0;

  if (out_base_views)
    *out_base_views = views ? views->len : 0;
  if (out_adapt_views)
    *out_adapt_views = 0;

  if (!probe)
    return 0;

  if (views && views->len > 0)
    view_scores = g_new0 (int, views->len);

  /*
   * Primary authentication remains the historical best single-enrolled-view
   * SIGFM score.  rel71.18 rejected cross-view union/fusion because it could
   * inflate wrong-finger candidates.
   *
   * rel71.28 adds one deliberately narrow SECONDARY decision only when the
   * primary best score is still below threshold.  It evaluates the normalized
   * raster already serialized in the SAME enrolled view and requires all
   * strict photometric gates to pass.  No scores are summed across views.
   */
  for (guint i = 0; views && i < views->len; i++)
    {
      int score = gx_sift_match (probe, g_ptr_array_index (views, i));

      view_scores[i] = score;
      if (score > best)
        best = score;

      for (guint rank = 0; rank < G_N_ELEMENTS (top); rank++)
        if (score > top[rank])
          {
            for (guint j = G_N_ELEMENTS (top) - 1; j > rank; j--)
              {
                top[j] = top[j - 1];
                top_idx[j] = top_idx[j - 1];
              }
            top[rank] = score;
            top_idx[rank] = i;
            break;
          }
    }

  if (best >= GX_PIXEL_RESCUE_CANDIDATE_BEST_MIN &&
      best < GX_MATCH_THRESHOLD && views)
    for (guint i = 0; i < views->len; i++)
      {
        int score = view_scores[i];
        int inliers = 0, overlap = 0, zncc = 0, agree = 0;

        gboolean pass = gx_pixel_rescue_view_pass (probe,
                                                   g_ptr_array_index (views, i),
                                                   score,
                                                   &inliers, &overlap,
                                                   &zncc, &agree);

        if (overlap > 0 &&
            (!near_valid || agree > near_agree ||
             (agree == near_agree && zncc > near_zncc)))
          {
            near_valid = TRUE;
            near_idx = i;
            near_score = score;
            near_inliers = inliers;
            near_overlap = overlap;
            near_zncc = zncc;
            near_agree = agree;
          }

        if (pass)
          {
            rescued = TRUE;
            rescue_idx = i;
            rescue_inliers = inliers;
            rescue_overlap = overlap;
            rescue_zncc = zncc;
            rescue_agree = agree;
            break;
          }
      }

  if (g_getenv ("GXFP_MATCH_DIAGNOSTICS"))
    {
      fp_info ("gallery scores: %d@%u %d@%u %d@%u %d@%u %d@%u (views=%u)",
               top[0], top_idx[0], top[1], top_idx[1], top[2], top_idx[2],
               top[3], top_idx[3], top[4], top_idx[4],
               views ? views->len : 0);

      if (views)
        for (guint rank = 0; rank < G_N_ELEMENTS (top); rank++)
          {
            guint idx = top_idx[rank];
            int inliers = 0, overlap = 0, zncc = 0, agree = 0;

            if (top[rank] <= 0 || idx >= views->len)
              continue;
            if (gx_sift_pixel_overlap_metrics (probe,
                                               g_ptr_array_index (views, idx),
                                               &inliers, &overlap, &zncc, &agree))
              fp_info ("pixel diagnostic: rank=%u view=%u baseline=%d "
                       "inliers=%d overlap=%d zncc_milli=%d agree_permille=%d",
                       rank, idx, top[rank], inliers, overlap, zncc, agree);
          }
    }

  if (!rescued && near_valid &&
      best >= GX_PIXEL_RESCUE_CANDIDATE_BEST_MIN &&
      best < GX_MATCH_THRESHOLD)
    fp_info ("GXFP51A0 PIXEL_NEARMISS baseline=%d view=%u view_score=%d "
             "inliers=%d overlap=%d zncc_milli=%d agree_permille=%d",
             best, near_idx, near_score, near_inliers, near_overlap,
             near_zncc, near_agree);

  if (rescued)
    {
      fp_warn ("GXFP51A0 PIXEL_RESCUE baseline=%d view=%u inliers=%d "
               "overlap=%d zncc_milli=%d agree_permille=%d effective=%d",
               best, rescue_idx, rescue_inliers, rescue_overlap,
               rescue_zncc, rescue_agree, GX_MATCH_THRESHOLD);
      return GX_MATCH_THRESHOLD;
    }

  return best;
}

/* One physical press may yield up to three independent biometric images.
 * This mirrors Windows/Goodix RetryCaptureIMG: after the initial frame,
 * FDT-manual confirms the finger is still down and a fresh 0x20 image is
 * captured without requiring a lift. Every image must independently satisfy
 * the unchanged matcher threshold; scores are never summed or fused. */
static int
gx_score_probe_against_gallery (GPtrArray            *gallery,
                                const GxSiftFeatures *probe,
                                guint                *out_candidate)
{
  int best = 0;
  guint best_candidate = 0;

  for (guint i = 0; gallery && i < gallery->len; i++)
    {
      FpPrint *candidate = g_ptr_array_index (gallery, i);
      int score = gx_score_probe_against_print (candidate, probe, NULL, NULL);

      if (score > best)
        {
          best = score;
          best_candidate = i;
        }
    }

  if (out_candidate)
    *out_candidate = best_candidate;
  return best;
}

/* One physical press may yield up to three sensor frames, but at most ONE
 * usable biometric image is ever scored.
 *
 * Windows/Goodix RetryCaptureIMG remains available only as a quality-recovery
 * mechanism: if contrast extraction fails or the frame has too few keypoints,
 * the bad frame is discarded before matching and a fresh 0x20 image may be
 * requested while the finger is still down. Once a frame passes both quality
 * gates it is scored exactly once and that score is final for this physical
 * press. A below-threshold usable image therefore requires lift/reposition,
 * never a score-conditioned best-of-N recapture.
 *
 * This applies equally to 1:1 Verify and fprintd's multi-print Identify path. */
static GxSiftFeatures *
gx_capture_auth_same_press (FpiDeviceGoodix51A0 *self,
                            FpPrint              *tmpl,
                            GPtrArray            *gallery,
                            guint                *out_images,
                            int                  *out_score,
                            guint                *out_candidate,
                            guint                *out_base,
                            guint                *out_adapt)
{
  GxSiftFeatures *best_probe = NULL;
  int best_score = -1;
  guint best_candidate = 0, best_base = 0, best_adapt = 0;
  guint images = 0;
  gboolean cleanup_needed = FALSE;
  const gchar *mode = gallery ? "identify" : "verify";
  /* Stage timings, no pixels, fingerprints or templates in observability.
   * Opt-in so normal hot capture avoids even one extra logfile write. */
  gboolean perf_qa =
    g_strcmp0 (g_getenv ("GXFP_CAPTURE_PERF_QA"), "1") == 0;

  for (guint attempt = 1; attempt <= GX_SAME_PRESS_CAPTURE_ATTEMPTS; attempt++)
    {
      guint16 px[GOODIX_IMG_PIXELS];
      GxSiftFeatures *probe = NULL;
      gboolean ok;
      int score;
      guint candidate = 0;
      guint base_views = 0, adapt_views = 0;
      gint64 t_start = perf_qa ? g_get_monotonic_time () : 0;
      gint64 t_frame = 0;
      gint64 t_features = 0;

      if (attempt == 1)
        {
          ok = gx_capture_frame_ex (self, px, GX_CAPTURE_FINGER, FALSE);
          if (!ok)
            break;
          cleanup_needed = TRUE;
        }
      else
        {
          gboolean finger_still_down = FALSE;

          ok = gx_capture_retry_same_press_frame (self, px,
                                                  &finger_still_down);
          if (!ok || !finger_still_down)
            break;
        }

      /* If this authenticated image needed the safe no-evidence GET_IMAGE
       * replay, calibrate capture pacing NOW.  Waiting until final cleanup made
       * images 2/3 reuse the same too-tight timing even though image 1 had
       * already proved it inadequate.  The next RetryCaptureIMG therefore gets
       * the calibrated gap on this same physical pose. */
      if (self->capture_retry_seen)
        {
          int before = MAX (self->capture_gap_scale, GX_CAPTURE_SCALE_MIN);

          gx_capture_pacing_success (self);
          fp_warn ("GXFP51A0 AUTH_TRACE mode=%s same-press retry-assisted "
                   "pacing applied before next image: %d%% -> %d%% (%u us)",
                   mode, before, self->capture_gap_scale,
                   gx_capture_gap_us (self));
        }

      images++;
      if (perf_qa)
        t_frame = g_get_monotonic_time ();
      probe = gx_features_from_pixels (self, px);
      if (perf_qa)
        t_features = g_get_monotonic_time ();
      if (!probe)
        {
          fp_warn ("GXFP51A0 AUTH_TRACE mode=%s same-press image %u/%u "
                   "rejected before scoring: contrast quality gate",
                   mode, attempt, GX_SAME_PRESS_CAPTURE_ATTEMPTS);
          continue;
        }

      {
        guint keypoints = gx_sift_keypoints (probe);

        if (keypoints < GX_MIN_CAPTURE_KEYPOINTS)
          {
            fp_warn ("GXFP51A0 AUTH_TRACE mode=%s same-press image %u/%u "
                     "rejected before scoring: keypoints=%u minimum=%u",
                     mode, attempt, GX_SAME_PRESS_CAPTURE_ATTEMPTS,
                     keypoints, GX_MIN_CAPTURE_KEYPOINTS);
            g_clear_pointer (&probe, gx_sift_free);
            continue;
          }
      }

      if (gallery)
        score = gx_score_probe_against_gallery (gallery, probe, &candidate);
      else
        score = gx_score_probe_against_print (tmpl, probe,
                                              &base_views, &adapt_views);

      if (perf_qa)
        {
          gint64 t_score = g_get_monotonic_time ();
          fp_warn ("GXFP51A0 CAPTURE_PERF_QA mode=%s image=%u "
                   "frame_ms=%ld sift_ms=%ld match_ms=%ld total_ms=%ld",
                   mode, attempt,
                   (long) ((t_frame - t_start) / 1000),
                   (long) ((t_features - t_frame) / 1000),
                   (long) ((t_score - t_features) / 1000),
                   (long) ((t_score - t_start) / 1000));
        }

      fp_warn ("GXFP51A0 AUTH_TRACE mode=%s same-press image %u/%u "
               "score=%d threshold=%d candidate=%u decision=final-for-pose",
               mode, attempt, GX_SAME_PRESS_CAPTURE_ATTEMPTS, score,
               GX_MATCH_THRESHOLD, candidate);

      g_clear_pointer (&best_probe, gx_sift_free);
      best_probe = probe;
      probe = NULL;
      best_score = score;
      best_candidate = candidate;
      best_base = base_views;
      best_adapt = adapt_views;

      /*
       * Security boundary: the first usable image is the only scored draw for
       * this physical press.  A score below threshold requests a complete new
       * pose through the existing outer verify/identify retry budget.  Never
       * turn the fixed threshold into a best-of-N score-conditioned retry.
       */
      break;
    }

  if (cleanup_needed && !self->capture_recovery_pending)
    {
      gint64 t_cleanup = perf_qa ? g_get_monotonic_time () : 0;
      gboolean cleanup_ok = gx_send_capture_cleanup (self);
      if (perf_qa)
        fp_warn ("GXFP51A0 CAPTURE_PERF_QA cleanup_ms=%ld ok=%d",
                 (long) ((g_get_monotonic_time () - t_cleanup) / 1000),
                 cleanup_ok ? 1 : 0);
      if (!cleanup_ok)
        {
          fp_warn ("GXFP51A0 same-press final cleanup failed");
          /* Authenticated TLS image + completed biometric match came BEFORE
           * the protocol cleanup. A missing post-capture FDT response must
           * retire this TLS session, but not retroactively erase valid
           * positive biometric evidence. Never apply to enroll, a negative
           * score, missing/partial image or a cancelled operation. */
          if (g_strcmp0 (g_getenv ("GXFP_POSTAUTH_CLEANUP_DEFER_QA"), "1") == 0 &&
              (tmpl || gallery) && best_probe &&
              best_score >= GX_MATCH_THRESHOLD &&
              self->capture_recovery_pending)
            {
              self->force_cold_reset = TRUE;
              fp_warn ("GXFP51A0 POSTAUTH_CLEANUP_DEFER_QA "
                       "authenticated score=%d; retiring failed TLS context "
                       "at Close; positive first press preserved", best_score);
            }
          else
            g_clear_pointer (&best_probe, gx_sift_free);
        }
      else
        {
          /* Any image-level retry evidence was already consumed immediately
           * above so it could help the SAME press.  Cleanup may itself observe
           * fresh retry evidence; account for that independently here. */
          gx_capture_pacing_success (self);
        }
    }

  if (out_images)
    *out_images = images;
  if (out_score)
    *out_score = best_probe ? best_score : -1;
  if (out_candidate)
    *out_candidate = best_probe ? best_candidate : 0;
  if (out_base)
    *out_base = best_probe ? best_base : 0;
  if (out_adapt)
    *out_adapt = best_probe ? best_adapt : 0;

  fp_warn ("GXFP51A0 AUTH_TRACE mode=%s same-press completed "
           "images=%u best=%d threshold=%d",
           mode, images, MAX (best_score, 0), GX_MATCH_THRESHOLD);
  return best_probe;
}

static gboolean
gx_enroll_view_connected (GPtrArray             *accepted,
                          const GxSiftFeatures  *candidate,
                          int                    stage,
                          int                   *out_best,
                          guint                 *out_weak_links)
{
  int best = 0;
  guint weak_links = 0;

  if (out_best)
    *out_best = 0;
  if (out_weak_links)
    *out_weak_links = 0;

  if (!candidate)
    return FALSE;

  if (!accepted || accepted->len == 0)
    return TRUE;

  for (guint i = 0; i < accepted->len; i++)
    {
      int score = gx_sift_match (candidate, g_ptr_array_index (accepted, i));

      best = MAX (best, score);
      if (score >= GX_ENROLL_EXTENSION_WEAK_SCORE)
        weak_links++;
    }

  if (out_best)
    *out_best = best;
  if (out_weak_links)
    *out_weak_links = weak_links;

  if (stage < GX_ENROLL_ANCHOR_STAGES)
    return best >= GX_ENROLL_ANCHOR_MIN_SCORE;

  return best >= GX_ENROLL_EXTENSION_MIN_SCORE ||
         weak_links >= GX_ENROLL_EXTENSION_WEAK_LINKS;
}

static void
gx_capture_done (GObject *src, GAsyncResult *res, gpointer user_data)
{
  GxWork *w = g_task_get_task_data (G_TASK (res));
  FpiSsm *ssm = w->ssm;
  FpDevice *dev = w->dev;
  FpiDeviceGoodix51A0 *self = FPI_DEVICE_GOODIX51A0 (dev);
  GxTask *t = fpi_ssm_get_data (ssm);
  GxSiftFeatures *f = g_steal_pointer (&w->feat);

  if (gx_cancelled (dev, ssm))
    {
      /* No match scoring, image reuse, retry or SPI restart after the
       * suspend request.  A worker-held capture is already finished here. */
      fp_warn ("GXFP51A0 S3_CANCEL capture worker completed; dropping unscored result");
      g_clear_pointer (&f, gx_sift_free);
      return;
    }

  if (g_getenv ("GXFP_DIAGNOSTIC_CAPTURE_ONCE"))
    {
      guint nfeatures = f ? gx_sift_keypoints (f) : 0;

      fp_info ("GXFP51A0 capture-only diagnostic complete: features=%u",
               nfeatures);
      fp_info ("GXFP51A0 CAPTURE_DIAGNOSTIC_DONE: REMOVE_FINGER");
      g_clear_pointer (&f, gx_sift_free);
      fpi_device_report_finger_status (dev, FP_FINGER_STATUS_NONE);
      fpi_ssm_mark_failed (ssm, fpi_device_error_new_msg (
        FP_DEVICE_ERROR_GENERAL, "capture-only diagnostic complete"));
      return;
    }

      if (!f && self->capture_recovery_pending)
        {
          /* No authenticated image exists, so this is transport recovery, not
           * a biometric no-match and must not consume the fixed verify budget.
           * Ask for release, rebuild the session, then request a fresh press. */
          fp_warn ("GXFP51A0 capture transport failed; recovering before "
                   "another biometric attempt");
          fpi_device_report_finger_status (dev, FP_FINGER_STATUS_PRESENT);
          t->polls = 0;
          self->poll_id = g_timeout_add (GX_POLL_MS, gx_poll_off, ssm);
          return;
        }

      if (!f || gx_sift_keypoints (f) < GX_MIN_CAPTURE_KEYPOINTS)
        {
          /* Tell the two causes apart: they call for opposite gestures.
           * The image capture takes about 1.4 s, so a finger pressed and
           * immediately lifted does trigger detection, but the image is taken
           * once it has gone — contrast around 5, the empty level. Reporting
           * "not centred" there sends the user to fix the wrong thing, which
           * is glaring the first time someone unfamiliar tries the sensor. */
          FpDeviceRetry why = gx_finger_present (self)
                                ? FP_DEVICE_RETRY_CENTER_FINGER
                                : FP_DEVICE_RETRY_TOO_SHORT;

          g_clear_pointer (&f, gx_sift_free);
          if (why == FP_DEVICE_RETRY_TOO_SHORT)
            fp_info ("finger lifted before the capture finished");

          if (t->verifying)
            {
              fpi_ssm_mark_failed (ssm, fpi_device_retry_new (why));
              return;
            }
          /* unusable press: ask again without advancing the stage */
          fpi_device_enroll_progress (dev, t->stage, NULL,
                                      fpi_device_retry_new (why));
        }
      else if (t->verifying)
        {
          /* A usable press is one independent biometric decision. Verification
           * has a small fixed retry budget for partial-sensor placement misses;
           * retries never depend on how close a score is to the threshold. */
          t->tries++;
          g_clear_pointer (&t->probe, gx_sift_free);
          t->probe = f;

          if (t->identifying)
            {
              GPtrArray *gallery = NULL;
              FpPrint *best_print = NULL;
              int best = 0;

              fpi_device_get_identify_data (dev, &gallery);
              if (gallery && gallery->len > 0)
                {
                  if (!w->identify_gallery ||
                      w->identify_gallery->len != gallery->len ||
                      w->worker_candidate >= gallery->len ||
                      w->worker_score < 0)
                    {
                      fpi_ssm_mark_failed (ssm, fpi_device_error_new_msg (
                        FP_DEVICE_ERROR_PROTO, "identify gallery changed"));
                      return;
                    }
                  for (guint i = 0; i < gallery->len; i++)
                    if (g_ptr_array_index (gallery, i) !=
                        g_ptr_array_index (w->identify_gallery, i))
                      {
                        fpi_ssm_mark_failed (ssm, fpi_device_error_new_msg (
                          FP_DEVICE_ERROR_PROTO, "identify gallery reordered"));
                        return;
                      }
                  best = w->worker_score;
                  best_print = g_ptr_array_index (gallery,
                                                  w->worker_candidate);
                }
              fp_info ("identify: cached score=%d candidate=%u threshold=%d",
                       best, w->worker_candidate, GX_MATCH_THRESHOLD);

              t->best = MAX (t->best, best);
              if (best_print && best >= GX_MATCH_THRESHOLD)
                {
                  g_clear_object (&t->identify_match);
                  t->identify_match = g_object_ref (best_print);
                  fpi_device_identify_report (dev,
                                              t->identify_match,
                                              NULL,
                                              NULL);
                  t->match_reported = TRUE;
                  fp_info ("identify: match reported on press %d/%d "
                           "score=%d threshold=%d gallery=%u",
                           t->tries, GX_VERIFY_MAX_ATTEMPTS,
                           best, GX_MATCH_THRESHOLD,
                           gallery ? gallery->len : 0);
                }
              else if (t->tries >= GX_VERIFY_MAX_ATTEMPTS)
                {
                  fpi_device_identify_report (dev, NULL, NULL, NULL);
                  t->match_reported = TRUE;
                  fp_info ("identify: no-match reported after %d fixed presses "
                           "(best=%d threshold=%d gallery=%u)",
                           t->tries, t->best, GX_MATCH_THRESHOLD,
                           gallery ? gallery->len : 0);
                }
              else
                {
                  t->rearm_mcu_before_retry = TRUE;
                  fp_info ("identify: no-match press %d/%d (score=%d best=%d); "
                           "request another complete press with MCU rearm",
                           t->tries, GX_VERIFY_MAX_ATTEMPTS,
                           best, t->best);
                }
            }
          else
            {
              /* The authenticated capture worker already scored the
               * exact first usable frame against the held verify template.
               * Repeating the same comparison in this callback added
               * measured CPU latency without changing the decision. */
              int attempt_score = w->worker_score;
              guint base_views = w->worker_base;
              guint adapt_views = w->worker_adapt;

              if (attempt_score < 0)
                {
                  fpi_ssm_mark_failed (ssm, fpi_device_error_new_msg (
                    FP_DEVICE_ERROR_PROTO, "worker score unavailable"));
                  return;
                }
              t->best = MAX (t->best, attempt_score);

              fp_info ("verify: attempt %d/%d -> %d matches (best=%d threshold=%d, %u views + %u acquired)",
                       t->tries, GX_VERIFY_MAX_ATTEMPTS, attempt_score, t->best,
                       GX_MATCH_THRESHOLD, base_views, adapt_views);

              if (attempt_score >= GX_MATCH_THRESHOLD)
                {
                  fpi_device_verify_report (dev, FPI_MATCH_SUCCESS, NULL, NULL);
                  t->match_reported = TRUE;
                  fp_info ("verify: match reported on attempt %d at %d matches",
                           t->tries, attempt_score);
                }
              else if (t->tries >= GX_VERIFY_MAX_ATTEMPTS)
                {
                  fpi_device_verify_report (dev, FPI_MATCH_FAIL, NULL, NULL);
                  t->match_reported = TRUE;
                  fp_info ("verify: no-match reported after %d fixed attempts (best=%d threshold=%d)",
                           t->tries, t->best, GX_MATCH_THRESHOLD);
                }
              else
                {
                  t->rearm_mcu_before_retry = TRUE;
                  fp_info ("verify: no-match attempt %d/%d; request another complete press with MCU rearm",
                           t->tries, GX_VERIFY_MAX_ATTEMPTS);
                }
            }

        }
      else
        {
          guint keypoints = gx_sift_keypoints (f);
          int enroll_best = 0;
          guint weak_links = 0;

          /* A partial sensor needs diversity, but unconstrained diversity made
           * the rel71.24-era enrollment consume slots with views that had
           * almost no relation to the rest of the template.  Build a small
           * redundant anchor first, then permit connected expansion.
           *
           * These low scores are enrollment-topology evidence only.  They do
           * not alter the authentication threshold or accept a login. */
          if (!gx_enroll_view_connected (t->views, f, t->stage,
                                         &enroll_best, &weak_links))
            {
              fp_info ("enroll: rejected disconnected view stage=%d/%d "
                       "keypoints=%u best_link=%d weak_links=%u; retry",
                       t->stage + 1, GX_ENROLL_STAGES, keypoints,
                       enroll_best, weak_links);
              g_clear_pointer (&f, gx_sift_free);
              fpi_device_enroll_progress (
                dev, t->stage, NULL,
                fpi_device_retry_new (FP_DEVICE_RETRY_CENTER_FINGER));
            }
          else
            {
              g_ptr_array_add (t->views, f);
              t->stage++;
              fp_info ("enroll: accepted stage=%d/%d keypoints=%u "
                       "best_link=%d weak_links=%u",
                       t->stage, GX_ENROLL_STAGES, keypoints,
                       enroll_best, weak_links);
              fpi_device_enroll_progress (dev, t->stage, NULL, NULL);
            }
        }
      /* Capture done: we no longer need the finger, only its release.
       * PRESENT without NEEDED is the "you may lift now" signal. */
      if (t->verifying)
        fp_warn ("GXFP51A0 %s_TRACE physical press %d/%d LIFT_NOW",
                 t->identifying ? "IDENTIFY" : "VERIFY",
                 MAX (t->tries, 1), GX_VERIFY_MAX_ATTEMPTS);
      /* Verification/identification already reported a positive match, and
       * the capture thread has completed GET_IMAGE authentication and the
       * mandatory sensor/TLS cleanup BEFORE reaching this callback.
       *
       * Waiting for a physical lift now only delays verify_complete and the
       * lock-screen verdict.  A negative match still goes through the
       * unmodified lift/reposition and fixed three-press budget.  Do not
       * resume another SPI operation from this callback after success. */
      if (t->verifying && t->match_reported &&
          t->best >= GX_MATCH_THRESHOLD)
        {
          fp_warn ("GXFP51A0 AUTH_TRACE verified positive terminal "
                   "without release wait (score=%d threshold=%d)",
                   t->best, GX_MATCH_THRESHOLD);
          fpi_device_report_finger_status (dev, FP_FINGER_STATUS_NONE);
          fpi_ssm_jump_to_state (ssm, GX_ST_DONE);
          return;
        }

      fpi_device_report_finger_status (dev, FP_FINGER_STATUS_PRESENT);
      t->polls = 0;
      self->poll_id = g_timeout_add (GX_POLL_MS, gx_poll_off, ssm);
}

static void
gx_run_state (FpiSsm *ssm, FpDevice *dev)
{
  FpiDeviceGoodix51A0 *self = FPI_DEVICE_GOODIX51A0 (dev);
  GxTask *t = fpi_ssm_get_data (ssm);

  switch (fpi_ssm_get_cur_state (ssm))
    {
    case GX_ST_SESSION:
      gx_run_async (ssm, dev, gx_session_thread, gx_session_done);
      break;

    case GX_ST_WAIT_ON:
      {
        gboolean changed;

        t->polls = 0;
        t->contact_since_us = 0;
        changed = fpi_device_report_finger_status (dev, FP_FINGER_STATUS_NEEDED);
        if (t->verifying)
          fp_warn ("GXFP51A0 %s_TRACE physical press %d/%d READY",
                   t->identifying ? "IDENTIFY" : "VERIFY",
                   t->tries + 1, GX_VERIFY_MAX_ATTEMPTS);
        fp_info ("finger status: needed=1 present=0 changed=%d",
                 changed ? 1 : 0);
        self->poll_id = g_timeout_add (GX_POLL_MS, gx_poll_on, ssm);
        break;                     /* the poll drives the next state */
      }

    case GX_ST_CAPTURE:
      gx_run_async (ssm, dev, gx_capture_thread, gx_capture_done);
      break;

    case GX_ST_WAIT_OFF:
      /* only reached by jumping here; the poll drives what follows */
      break;

    case GX_ST_DONE:
      fpi_ssm_mark_completed (ssm);
      break;
    }
}

/* --- enrolment completion ------------------------------------------ */
static void
gx_enroll_done (FpiSsm *ssm, FpDevice *dev, GError *error)
{
  GxTask *t = fpi_ssm_get_data (ssm);
  FpPrint *print = NULL;

  if (error)
    {
      fpi_device_enroll_complete (dev, NULL, error);
      return;
    }
  if (!t->views || t->views->len < 3)
    {
      fpi_device_enroll_complete (dev, NULL,
        fpi_device_error_new_msg (FP_DEVICE_ERROR_GENERAL,
                                  "too few usable views"));
      return;
    }

  fpi_device_get_enroll_data (dev, &print);
  fpi_print_set_type (print, FPI_PRINT_RAW);
  fpi_print_set_device_stored (print, FALSE);
  g_object_set (print, "fpi-data", gx_views_to_variant (t->views), NULL);

  fpi_device_enroll_complete (dev, g_object_ref (print), NULL);
}

/* ------------------------------------------------------------------ */
/*  Device operations                                                  */
/* ------------------------------------------------------------------ */

static void
gx_transport_close (FpiDeviceGoodix51A0 *self)
{
  if (self->irq_fd >= 0)
    {
      close (self->irq_fd);
      self->irq_fd = -1;
    }

  if (self->spi_fd >= 0)
    {
      close (self->spi_fd);
      self->spi_fd = -1;
    }
}

static gboolean
gx_transport_open (FpDevice *dev, GError **error)
{
  FpiDeviceGoodix51A0 *self = FPI_DEVICE_GOODIX51A0 (dev);
  const gchar *path;
  guint8 mode = SPI_MODE_0 | SPI_CS_HIGH;
  guint8 bits = 8;
  guint32 speed = 1000000;

  path = fpi_device_get_udev_data (dev, FPI_DEVICE_UDEV_SUBTYPE_SPIDEV);
  if (!path || !*path)
    {
      g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND,
                           "GXFP51A0 spidev node was not supplied by libfprint udev discovery");
      return FALSE;
    }

  self->spi_fd = open (path, O_RDWR | O_CLOEXEC);
  if (self->spi_fd < 0)
    {
      g_set_error (error, G_IO_ERROR, g_io_error_from_errno (errno),
                   "cannot open spidev node %s", path);
      return FALSE;
    }

  if (flock (self->spi_fd, LOCK_EX | LOCK_NB) != 0)
    {
      gx_transport_close (self);
      g_set_error_literal (error, FP_DEVICE_ERROR, FP_DEVICE_ERROR_BUSY,
                           "sensor already in use by another process");
      return FALSE;
    }

  if (ioctl (self->spi_fd, SPI_IOC_WR_MODE, &mode) < 0 ||
      ioctl (self->spi_fd, SPI_IOC_WR_BITS_PER_WORD, &bits) < 0 ||
      ioctl (self->spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0)
    {
      int saved_errno = errno;

      gx_transport_close (self);
      g_set_error (error, G_IO_ERROR, g_io_error_from_errno (saved_errno),
                   "cannot configure GXFP51A0 SPI transport: %s",
                   g_strerror (saved_errno));
      return FALSE;
    }

  self->irq_fd = gx51_open_irq_gpio48 ();
  if (self->irq_fd < 0)
    {
      gx_transport_close (self);
      g_set_error_literal (error, FP_DEVICE_ERROR, FP_DEVICE_ERROR_PROTO,
                           "GXFP51A0 GPIO48 IRQ input is not available");
      return FALSE;
    }

  fp_info ("GXFP51A0 IRQ source: direct GPIO48 level polling");
  return TRUE;
}

#define GX_SLEEP_DELTA_STALE_US (250 * 1000)
/* Keep a healthy same-process context across an awake workday.  This is not a
 * blind lease: every new Claim still performs a cheap FDT liveness probe, and
 * suspend or any failed liveness check forces the full recovery path. */
#define GX_WARM_IDLE_TTL_US (G_GINT64_CONSTANT(12) * 60 * 60 * G_USEC_PER_SEC)
G_STATIC_ASSERT (GX_WARM_IDLE_TTL_US == G_GINT64_CONSTANT(43200000000));

static gboolean
gx_sleep_delta_us (gint64 *out)
{
  struct timespec boot = { 0, }, mono = { 0, };

  g_return_val_if_fail (out != NULL, FALSE);

  if (clock_gettime (CLOCK_BOOTTIME, &boot) != 0 ||
      clock_gettime (CLOCK_MONOTONIC, &mono) != 0)
    return FALSE;

  *out = ((gint64) boot.tv_sec - (gint64) mono.tv_sec) * G_USEC_PER_SEC +
         ((gint64) boot.tv_nsec - (gint64) mono.tv_nsec) / 1000;
  return TRUE;
}

static gboolean
gx_warm_crossed_sleep (FpiDeviceGoodix51A0 *self)
{
  gint64 now = 0;

  if (!self->warm_sleep_clock_valid)
    return FALSE;

  if (!gx_sleep_delta_us (&now))
    return FALSE;

  if (now - self->warm_sleep_delta_us > GX_SLEEP_DELTA_STALE_US)
    {
      fp_info ("GXFP51A0 sleep boundary detected: boottime-monotonic advanced by %d ms",
               (int) ((now - self->warm_sleep_delta_us) / 1000));
      return TRUE;
    }

  return FALSE;
}

static gboolean
gx_warm_idle_expired (FpiDeviceGoodix51A0 *self)
{
  gint64 now;

  if (!self->warm_valid || self->warm_last_activity_us <= 0)
    return FALSE;

  now = g_get_monotonic_time ();
  if (now - self->warm_last_activity_us > GX_WARM_IDLE_TTL_US)
    {
      fp_info ("GXFP51A0 warm context idle for %d s; forcing cold rebuild",
               (int) ((now - self->warm_last_activity_us) / G_USEC_PER_SEC));
      return TRUE;
    }

  return FALSE;
}

static void
gx_warm_abandon (FpiDeviceGoodix51A0 *self)
{
  /* Host-only invalidation.  Use this after suspend or another lifecycle
   * boundary where sensor-side TLS state cannot be trusted: never send a
   * close_notify over a session that may no longer exist. */
  self->warm_valid = FALSE;
  self->production_ready = FALSE;
  self->have_fdt = FALSE;
  self->bg_dirty = FALSE;
  self->bg_last_refresh_us = 0;
  self->tls_rxlen = 0;
  self->tls_rxpos = 0;
  self->tls_up = FALSE;
  self->warm_last_activity_us = 0;
  self->warm_sleep_delta_us = 0;
  self->warm_sleep_clock_valid = FALSE;
  g_clear_pointer (&self->tls, gx_tls_free);
  gx_pmk_clear (self);
  g_clear_pointer (&self->bg_frame, g_free);
}

static void
gx_warm_discard (FpiDeviceGoodix51A0 *self)
{
  if (self->tls_up && self->spi_fd >= 0 && self->irq_fd >= 0)
    gx_tls_teardown (self);
  gx_warm_abandon (self);
}

static gboolean
gx_warm_available (FpiDeviceGoodix51A0 *self)
{
  return self->warm_valid && self->tls_up && self->tls &&
         self->bg_frame && self->have_fdt;
}

static gboolean
gx_warm_fast_ready (FpiDeviceGoodix51A0 *self)
{
  int cur[GXFP_FDT_ZONE_COUNT];
  guint8 touchflag = 0;
  int mean;
  gint64 t0 = g_get_monotonic_time ();

  /* Fast path for an already prepared sensor: prove only that the FDT path is
   * alive.  Do not take a new background image before telling the desktop the
   * reader is ready; that old rebase made every normal lock pay 1-20 seconds
   * before the user could even be detected.  Sleep is rejected before this
   * function and any failed FDT probe falls back to the full validated path. */
  if (gx_fdt_probe_ex (self, cur, &touchflag) != 0)
    return FALSE;

  mean = gx_fdt_mean (cur);
  self->warm_last_activity_us = g_get_monotonic_time ();

  if (!gx_fdt_touch_is_finger (touchflag) && mean >= GOODIX_FDT_ABS)
    {
      memcpy (self->fdt_base, cur, sizeof self->fdt_base);
      self->fdt_abs = mean - GOODIX_FDT_ABS_MARGIN;
      self->have_fdt = TRUE;
    }

  fp_warn ("GXFP51A0 FAST_READY in %d ms: FDT alive touch=0x%02x mean=%d "
           "floor=%d; reusing prepared TLS/background",
           (int) ((self->warm_last_activity_us - t0) / 1000),
           touchflag, mean, self->fdt_abs);
  return TRUE;
}

static gboolean
gx_warm_validate (FpiDeviceGoodix51A0 *self)
{
  g_autofree guint16 *fresh_bg = g_new (guint16, GOODIX_IMG_PIXELS);
  gboolean previous_pacing_suppression = self->capture_pacing_suppressed;
  gboolean ok = FALSE;
  int cur[GXFP_FDT_ZONE_COUNT];
  int after[GXFP_FDT_ZONE_COUNT];
  guint8 before_touchflag = 0;
  guint8 after_touchflag = 0;
  int before_mean;
  int after_mean;
  gint64 t0 = g_get_monotonic_time ();

  /* Failure while validating a retained lifecycle context is evidence that the
   * session went stale, not that steady-state capture pacing is too fast. */
  self->capture_pacing_suppressed = TRUE;

  if (gx_fdt_probe_ex (self, cur, &before_touchflag) != 0)
    goto out;

  before_mean = gx_fdt_mean (cur);

  /* A user may already be holding the power-button sensor when Claim/Open runs.
   * Never overwrite ImageBase/background with a finger frame.  Sleep and idle
   * boundaries were handled before this function, so an otherwise warm
   * context with a responsive FDT may proceed and let the real Verify capture
   * exercise TLS.  The previous clean background is retained for this press. */
  if (gx_fdt_touch_is_finger (before_touchflag) ||
      before_mean < GOODIX_FDT_ABS)
    {
      self->warm_last_activity_us = g_get_monotonic_time ();
      fp_warn ("GXFP51A0 WARM_REBASE deferred: finger already present "
               "(FDT mean=%d floor=%d); retaining previous background",
               before_mean, GOODIX_FDT_ABS);
      ok = TRUE;
      goto out;
    }

  /* Exact-target imaging depends on a no-finger background. Refresh only
   * the ImageBase plane here: cold preparation owns OTP-derived DAC writes.
   * Rewriting those DACs during a warm NeedUpdateImageBase-style transition
   * conflates calibration with baseline refresh and can reset useful sensor
   * state. Reuse the warm T0 validation image as the new background, then
   * prove the sensor stayed clear before adopting it. */
  if (!gx_capture_warm_background_frame (self, fresh_bg))
    {
      fp_warn ("GXFP51A0 warm FDT answered but GET_IMAGE/TLS validation failed");
      goto out;
    }

  if (gx_fdt_probe_ex (self, after, &after_touchflag) != 0)
    goto out;

  after_mean = gx_fdt_mean (after);
  if (gx_fdt_touch_is_finger (after_touchflag) ||
      after_mean < GOODIX_FDT_ABS)
    {
      self->warm_last_activity_us = g_get_monotonic_time ();
      fp_warn ("GXFP51A0 WARM_REBASE discarded: finger landed during "
               "background capture (FDT mean=%d floor=%d)",
               after_mean, GOODIX_FDT_ABS);
      ok = TRUE;
      goto out;
    }

  memcpy (self->bg_frame, fresh_bg,
          sizeof (guint16) * GOODIX_IMG_PIXELS);
  self->bg_last_refresh_us = g_get_monotonic_time ();
  memcpy (self->fdt_base, after, sizeof self->fdt_base);
  self->fdt_abs = after_mean - GOODIX_FDT_ABS_MARGIN;
  self->have_fdt = TRUE;
  self->warm_last_activity_us = g_get_monotonic_time ();

  fp_warn ("GXFP51A0 WARM_REBASE refreshed background+FDT in %d ms "
           "(idle=%d floor=%d)",
           (int) ((self->warm_last_activity_us - t0) / 1000),
           after_mean, self->fdt_abs);
  ok = TRUE;

out:
  self->capture_pacing_suppressed = previous_pacing_suppression;
  return ok;
}

static gboolean
gx_cold_prepare (FpiDeviceGoodix51A0 *self)
{
  gchar fw[64] = "";
  gboolean stage_qa =
    g_strcmp0 (g_getenv ("GXFP_RESUME_STAGE_QA"), "1") == 0;
  gint64 qa_t0 = stage_qa ? g_get_monotonic_time () : 0;
  gint64 qa_stage = qa_t0;
  gboolean driverstate_ok;

  self->fdt_abs = GOODIX_FDT_ABS;

  /* A fresh fprintd process starts at nominal timing, but a same-process cold
   * recovery keeps any protocol timing already proven necessary by missed
   * ACK/FDT responses. This is RAM-only and therefore cannot ratchet across
   * boots. Capture pacing remains an independent biometric-path setting. */
  if (self->timing_scale < GX_TIMING_SCALE_MIN)
    self->timing_scale = GX_TIMING_SCALE_MIN;
  if (self->capture_gap_scale < GX_CAPTURE_SCALE_MIN)
    self->capture_gap_scale = GX_CAPTURE_SCALE_MIN;

  driverstate_ok = gx_driverstate_install_windows (self);
  if (stage_qa)
    fp_warn ("GXFP51A0 RESUME_STAGE_QA stage=driverstate elapsed_ms=%ld "
             "ack=%d sleep_boundary=%d",
             (long) ((g_get_monotonic_time () - qa_stage) / 1000),
             driverstate_ok ? 1 : 0, self->s3_first_tls_pending ? 1 : 0);
  if (!driverstate_ok)
    {
      if (stage_qa)
        fp_warn ("GXFP51A0 RESUME_STAGE_QA reset_reason=driverstate_silent");
      fp_info ("GXFP51A0: DriverState silent; applying reviewed Windows fallback reset");
      gx_gpio_reset (self);
      fp_warn ("GXFP51A0: continuing to init_MCU after DriverState fallback as Windows does");
    }
  qa_stage = stage_qa ? g_get_monotonic_time () : 0;

  if (gx_read_fw_version (self, fw, sizeof fw))
    fp_info ("GXFP51A0 firmware: %s", fw);
  else
    {
      fp_info ("GXFP51A0: three A8 attempts silent; applying common-init reset fallback");
      if (stage_qa)
        fp_warn ("GXFP51A0 RESUME_STAGE_QA reset_reason=fw_version_silent");
      gx_gpio_reset (self);
      if (gx_read_fw_version_once (self, fw, sizeof fw))
        fp_info ("GXFP51A0 firmware after reset fallback: %s", fw);
      else
        fp_warn ("GXFP51A0: no A8 firmware response after final fallback");
    }

  if (stage_qa)
    fp_warn ("GXFP51A0 RESUME_STAGE_QA stage=fw_version elapsed_ms=%ld",
             (long) ((g_get_monotonic_time () - qa_stage) / 1000));
  fp_info ("GXFP51A0 opened; experimental PMK/TLS/capture path enabled");
  qa_stage = stage_qa ? g_get_monotonic_time () : 0;

  if (g_getenv ("GXFP_DIAGNOSTIC_CAPTURE_ONCE") ||
      g_getenv ("GXFP_DIAGNOSTIC_ONESHOT") ||
      g_getenv ("GXFP_DIAGNOSTIC_TLS_ONLY"))
    return FALSE;

  /* Claim/open is the authoritative preparation boundary.  Unlike rel24/25
   * probe prewarm, this uses the bounded recovery wrapper, including a fresh
   * MCU boundary between attempts and the production PMK fallback. */
  if (!gx_prepare_capture_context (self, FALSE))
    {
      if (stage_qa)
        fp_warn ("GXFP51A0 RESUME_STAGE_QA stage=full_capture_context "
                 "elapsed_ms=%ld success=0",
                 (long) ((g_get_monotonic_time () - qa_stage) / 1000));
      fp_warn ("GXFP51A0 cold preparation failed after bounded recovery");
      gx_invalidate_capture_context (self);
      gx_tls_teardown (self);
      gx_pmk_clear (self);
      g_clear_pointer (&self->bg_frame, g_free);
      return FALSE;
    }

  if (stage_qa)
    fp_warn ("GXFP51A0 RESUME_STAGE_QA stage=full_capture_context "
             "elapsed_ms=%ld total_cold_ms=%ld success=1",
             (long) ((g_get_monotonic_time () - qa_stage) / 1000),
             (long) ((g_get_monotonic_time () - qa_t0) / 1000));
  /* Protocol ACK/FDT misses measured during this cold preparation are useful
   * controller-speed evidence before the user ever touches the sensor.  Seed
   * the capture gap conservatively from that evidence so the first biometric
   * GET_IMAGE does not have to fail once merely to learn the same fact.
   * 300% protocol -> 250% capture on the reference MateBook; a nominal/fast
   * controller stays at 100%.  This remains process-local and non-persistent. */
  {
    int protocol_floor =
      CLAMP (self->timing_scale - GX_CAPTURE_SCALE_STEP,
             GX_CAPTURE_SCALE_MIN, 250);
    int previous = MAX (self->capture_gap_scale, GX_CAPTURE_SCALE_MIN);

    if (protocol_floor > previous)
      {
        self->capture_gap_scale = protocol_floor;
        fp_info ("GXFP51A0 capture pacing seeded from protocol calibration: "
                 "%d%% -> %d%% (%u us gap, protocol=%d%%); not persisted",
                 previous, self->capture_gap_scale,
                 gx_capture_gap_us (self), self->timing_scale);
      }
  }

  self->capture_recovery_pending = FALSE;
  self->capture_pacing_suppressed = FALSE;
  self->production_ready = TRUE;
  self->warm_valid = TRUE;
  self->s3_first_tls_pending = FALSE;
  self->s3_rel61_tls_fallback = FALSE;
  self->warm_last_activity_us = g_get_monotonic_time ();
  self->warm_sleep_clock_valid =
    gx_sleep_delta_us (&self->warm_sleep_delta_us);
  fp_info ("GXFP51A0 production capture context ready; native warm state armed "
           "(capture pacing=%d%% gap=%u us)",
           self->capture_gap_scale, gx_capture_gap_us (self));
  return TRUE;
}

static void
gx_dev_probe (FpDevice *dev)
{
  FpiDeviceGoodix51A0 *self = FPI_DEVICE_GOODIX51A0 (dev);
  GError *err = NULL;

  /* Restore rel23 native enumeration-time preparation, but use the exact
   * known-working rel71.36 FULL recovery path rather than the truncated
   * prewarm from rejected rel71.37. fprintd synchronously enumerates BEFORE
   * owning its D-Bus name; the existing service is already ordered before
   * the Plasma login manager. On success its FIRST Claim is FAST_READY.
   *
   * Every reset must be done with SPI and IRQ CLOSED, unlike rel24/25;
   * a transient accepted GET_IMAGE/TLS timeout gets normal whole-session
   * recovery and PMK staging fallback WITHOUT deleting the validated cache.
   * Do not ship any custom daemon, helper, timer or system-sleep hook. */
  gboolean diagnostic = g_getenv ("GXFP_DIAGNOSTIC_CAPTURE_ONCE") != NULL ||
                        g_getenv ("GXFP_DIAGNOSTIC_ONESHOT") != NULL ||
                        g_getenv ("GXFP_DIAGNOSTIC_TLS_ONLY") != NULL;
  gboolean ready = FALSE;
  gint64 started_us = g_get_monotonic_time ();

  if (diagnostic)
    {
      if (!gx_transport_open (dev, &err))
        {
          fpi_device_probe_complete (dev, NULL, NULL, err);
          return;
        }
      gx_transport_close (self);
      fpi_device_probe_complete (dev, NULL, NULL, NULL);
      return;
    }

  gx_transport_close (self);
  g_usleep (G_USEC_PER_SEC);
  gx_gpio_reset (self);
  g_usleep (G_USEC_PER_SEC);

  if (gx_transport_open (dev, &err))
    {
      /* The SAME bounded full preparation/recovery and TLS staging proven
       * by the 7-16-second real Claim tests, not a shorter unproven fork. */
      ready = gx_cold_prepare (self) && gx_warm_available (self);
      gx_transport_close (self);
    }

  if (ready)
    fp_info ("GXFP51A0 NATIVE_PROBE_PREWARM ready elapsed_ms=%d; "
             "first greeter Claim can reuse warm TLS/FDT",
             (int)((g_get_monotonic_time () - started_us)/1000));
  else
    {
      fp_warn ("GXFP51A0 NATIVE_PROBE_PREWARM full cold preparation failed; "
               "keeping the recognized device available for later Claim");
      gx_warm_abandon (self);
    }

  /* Never hide the sensor or delete its enrolled prints after a recoverable
   * TLS/image initialization miss. The password greeter remains independent. */
  g_clear_error (&err);
  fpi_device_probe_complete (dev, NULL, NULL, NULL);
}

static void
gx_dev_open (FpDevice *dev)
{
  FpiDeviceGoodix51A0 *self = FPI_DEVICE_GOODIX51A0 (dev);
  GError *err = NULL;
  gboolean slept = gx_warm_crossed_sleep (self);
  gboolean expired = gx_warm_idle_expired (self);
  gboolean had_warm = self->warm_valid || self->tls != NULL || self->tls_up;
  gboolean warm_candidate;

  /* An idle libfprint device may not receive the driver suspend vfunc. Detect
   * that case before touching hardware, while stale TLS can still be discarded
   * host-side without sending anything to the sensor. */
  if (slept || expired || self->force_cold_reset)
    {
      fp_info ("GXFP51A0 lifecycle/idle boundary detected; invalidating warm state");
      gx_warm_abandon (self);
      self->capture_recovery_pending = FALSE;
      self->capture_gap_scale = 0;
      self->capture_clean_streak = 0;
      self->capture_retry_seen = FALSE;
      self->capture_pacing_suppressed = TRUE;
      had_warm = FALSE;
    }

  warm_candidate = gx_warm_available (self);

  /* A real cold boundary must occur BEFORE spidev is opened. rel71.16 did the
   * opposite (open -> GPIO reset) and a daemon restart could leave the MCU in
   * a persistent TLS digest-failure state. A fresh process, stale/incomplete
   * warm context, or pending transport recovery therefore gets:
   *
   *   closed transport -> GPIO264 HIGH300/LOW600 -> fresh spidev open
   *
   * Healthy same-process warm state is the only case that skips this reset,
   * because FAST_READY intentionally preserves the live TLS/background state. */
  if (!warm_candidate || self->capture_recovery_pending)
    {
      if (had_warm)
        {
          fp_info ("GXFP51A0 incomplete warm context; returning to detached cold boundary");
          gx_warm_discard (self);
        }

      /* The prestart reset previously succeeded only after a fully quiet
       * detached bus. Keep this timing in libfprint itself; do not rebind the
       * kernel spidev child from inside a live FpDevice (udev removal race). */
      fp_warn ("GXFP51A0 NATIVE_COLD_QUIESCE before SPI open");
      gx_transport_close (self);
      g_usleep (G_USEC_PER_SEC);
      gx_reset_detached_s3_boundary (self);
      g_usleep (G_USEC_PER_SEC);
      self->force_cold_reset = FALSE;
    }

  if (!gx_transport_open (dev, &err))
    {
      fpi_device_open_complete (dev, err);
      return;
    }

  if (warm_candidate && !self->capture_recovery_pending)
    {
      if (gx_warm_fast_ready (self))
        {
          self->production_ready = TRUE;
          fp_info ("GXFP51A0 reusing native warm context without blocking background recapture");
          fpi_device_open_complete (dev, NULL);
          return;
        }

      fp_warn ("GXFP51A0 fast FDT readiness probe failed; trying full warm validation");
      if (gx_warm_validate (self))
        {
          self->production_ready = TRUE;
          fp_info ("GXFP51A0 full warm fallback recovered the native context");
          fpi_device_open_complete (dev, NULL);
          return;
        }

      /* Warm validation itself touched SPI and failed. Re-establish the same
       * detached cold boundary before cold preparation; never reset while the
       * failed transport remains open. */
      fp_warn ("GXFP51A0 warm context failed readiness validation; using detached cold recovery");
      gx_warm_abandon (self);
      gx_transport_close (self);
      gx_gpio_reset (self);
      err = NULL;
      if (!gx_transport_open (dev, &err))
        {
          fpi_device_open_complete (dev, err);
          return;
        }
    }

  if (!gx_cold_prepare (self))
    {
      gx_warm_abandon (self);
      gx_transport_close (self);
      fpi_device_open_complete (
        dev, fpi_device_error_new_msg (FP_DEVICE_ERROR_PROTO,
                                       "GXFP51A0 cold preparation failed"));
      return;
    }

  fpi_device_open_complete (dev, NULL);
}

static void
gx_dev_close (FpDevice *dev)
{
  FpiDeviceGoodix51A0 *self = FPI_DEVICE_GOODIX51A0 (dev);

  g_clear_handle_id (&self->poll_id, g_source_remove);

  /* Close may be requested more than once around a short-lived fprintd Claim:
   * the first close marks production_ready=FALSE, while later cleanup can call
   * close again on the same still-valid FpDevice.  Requiring production_ready
   * here made that second close tear down an otherwise complete TLS/background
   * context, so FAST_READY only survived a very narrow handoff window.
   *
   * Make warm close idempotent: if the retained context is complete and no
   * real cold boundary was requested, keep it regardless of the UI/action
   * readiness flag. Host SPI/GPIO FDs are still closed, so this adds no polling
   * or periodic I/O while idle. */
  if (!self->force_cold_reset && gx_warm_available (self))
    {
      gboolean was_ready = self->production_ready;

      self->production_ready = FALSE;
      gx_pmk_clear (self);
      gx_transport_close (self);
      fp_warn ("GXFP51A0 CLOSE_TRACE retained complete warm context "
               "(production_ready=%d); next open uses FDT-only FAST_READY",
               was_ready ? 1 : 0);
      fpi_device_close_complete (dev, NULL);
      return;
    }

  if (self->force_cold_reset)
    gx_warm_abandon (self);
  else
    gx_warm_discard (self);
  gx_transport_close (self);
  fpi_device_close_complete (dev, NULL);
}

static void
gx_dev_suspend (FpDevice *dev)
{
  FpiDeviceGoodix51A0 *self = FPI_DEVICE_GOODIX51A0 (dev);
  gboolean idle = self->spi_fd < 0 && self->irq_fd < 0;
  gboolean warm = gx_warm_available (self);
  gboolean attempted = FALSE;
  gboolean parked = FALSE;
  gboolean pre_sleep_clean = FALSE;
  gboolean qa_presleep_interrupted = FALSE;

  /* The patched native libfprint idle suspend invokes this callback inside
   * fprintd's EXISTING logind delay inhibitor; there is no external service,
   * system-sleep hook, timer or polling worker.
   *
   * Normal Close preserves the useful warm TLS session and closes the file
   * descriptors. Open them ONLY at a real system suspend boundary to issue
   * Windows' SLEEP 0x60 / payload 01 00 with a mandatory ACK. */
  if (idle)
    {
      g_autoptr(GError) err = NULL;
      struct gxfp_target_packet packet;

      if (gx_transport_open (dev, &err))
        {
          /* At suspend time the warm TLS channel still exists. Prove that
           * the sensor is TRULY clear immediately before sleep, not merely
           * that a stored ImageBase was clear minutes earlier. If this
           * optional fast probe fails, proceed with the normal sleep and
           * leave resume to ordinary full cold calibration. */
          if (warm && self->have_fdt && !self->bg_dirty)
            {
              int park_fdt[GXFP_FDT_ZONE_COUNT];
              guint8 park_touchflag = 0xff;

              if (gx_fdt_probe_ex (self, park_fdt, &park_touchflag) == 0)
                {
                  int park_mean = gx_fdt_mean (park_fdt);
                  int baseline_mean = gx_fdt_mean (self->fdt_base);

                  pre_sleep_clean = (park_touchflag & 0x3fu) == 0 &&
                                    park_mean >= GOODIX_FDT_ABS &&
                                    ABS (park_mean - baseline_mean) <=
                                    GX_DIAG_BASELINE_MAX_DRIFT;
                }
            }

          /* Optional native S3 improvement: a 33-minute-old, but otherwise
           * proven-clean ImageBase was reused after S3 in the rel71.63
           * hardware trace; the first genuine finger was rejected by the
           * contrast quality gate before biometric scoring.
           *
           * If the sensor is CLEAR at S3 park, take a new background NOW
           * (while finger-free) using the existing warm validated capture
           * path. Require both the fresh timestamp AND another independent
           * clear FDT probe before allowing the snapshot to be preserved.
           * A failed or touch-contaminated refresh must NOT save any stale
           * reference: ordinary S3 cold calibration remains the fallback.
           * Disabled unless explicitly opted in; never changes matcher
           * threshold, enrollment or capture scoring. */
          if (pre_sleep_clean &&
              g_strcmp0 (g_getenv ("GXFP_S3_FRESH_BG_QA"), "1") == 0 &&
              !gx_background_is_fresh (self))
            {
              gint64 old_refresh_us = self->bg_last_refresh_us;
              int fdt_after[GXFP_FDT_ZONE_COUNT];
              guint8 touch_after = 0xff;
              gboolean refreshed;
              gint64 started_us = g_get_monotonic_time ();

              /* systemd-logind defaults to only 5 seconds of delay-inhibit.
               * Cap the extra QA refresh at 2.3s, leaving headroom for the
               * native SLEEP ACK and post-transaction handling. Every GET_IMAGE
               * drain and TLS poll checks this deadline. Incomplete images are
               * never accepted or saved as new ImageBase. */
              self->s3_presleep_deadline_us = started_us +
                                             G_GINT64_CONSTANT(2300000);
              refreshed = gx_warm_validate (self);
              qa_presleep_interrupted =
                g_get_monotonic_time () >= self->s3_presleep_deadline_us;
              self->s3_presleep_deadline_us = 0;

              if (!refreshed || qa_presleep_interrupted ||
                  self->bg_last_refresh_us <= old_refresh_us ||
                  !gx_background_is_fresh (self) ||
                  gx_fdt_probe_ex (self, fdt_after, &touch_after) != 0 ||
                  (touch_after & 0x3fu) != 0 ||
                  gx_fdt_mean (fdt_after) < GOODIX_FDT_ABS ||
                  ABS (gx_fdt_mean (fdt_after) -
                       gx_fdt_mean (self->fdt_base)) >
                       GX_DIAG_BASELINE_MAX_DRIFT)
                {
                  pre_sleep_clean = FALSE;
                  fp_warn ("GXFP51A0 S3_FRESH_BG_QA no clean refreshed "
                           "reference; fail closed to native cold S3");
                }
              else
                {
                  fp_warn ("GXFP51A0 S3_FRESH_BG_QA refreshed ImageBase "
                           "before SLEEP ACK, age_ms=%d",
                           (int) (gx_background_age_us (self) / 1000));
                }
            }

          /* If the presleep image did not finish by the bounded deadline,
           * the TLS record may still be in flight. Do NOT send an unrelated
           * SLEEP command into a possibly desynced transport; the following
           * native S3 resume will cold-reset and prepare a clean context. */
          if (!qa_presleep_interrupted && !self->capture_recovery_pending &&
              gxfp_build_sleep (&packet))
            {
              attempted = TRUE;
              parked = gx_target_send_ack (self, &packet, 0x60, NULL);
            }
          gx_transport_close (self);
        }
      else
        fp_warn ("GXFP51A0 NATIVE_S3_PARK transport unavailable: %s",
                 err ? err->message : "unknown");
    }

  /* Capture only an explicitly clean, recent ImageBase at a successfully
   * acknowledged idle S3 park. This RAM-only image is not a biometric print,
   * is never persisted and is discarded after one use or expiration. */
  if (idle && parked && warm && pre_sleep_clean &&
      !self->bg_dirty && self->bg_frame && self->have_fdt)
    gx_s3_snapshot_clean_bg (self);
  else
    {
      /* If a cleanup/desync made warm=0 just before suspend, preserve the
       * last independently verified no-finger RAM baseline. It remains
       * age-bounded, only becomes active on a live 6/6 finger FDT proof,
       * and is never reused as a TLS session or a biometric image. */
      if (g_strcmp0 (g_getenv ("GXFP_HELD_FINGER_ROLLING_BG_QA"), "1") == 0 &&
          self->s3_saved_bg)
        fp_warn ("GXFP51A0 HELD_FINGER_QA retaining proven clean RAM "
                 "reference despite warm=%d parked=%d",
                 warm ? 1 : 0, parked ? 1 : 0);
      else
        gx_s3_discard_clean_bg (self);
      if (warm && idle && parked)
        fp_warn ("GXFP51A0 NATIVE_S3_CLEAN_REFERENCE_NOT_SAVED "
                 "pre_sleep_clear=%d recent_reference=%d",
                 pre_sleep_clean ? 1 : 0,
                 self->bg_last_refresh_us > 0 ? 1 : 0);
    }

  /* Idle and in-progress actions have different libfprint semantics. Never
   * transmit Sleep during an active biometric capture. A failed park cannot
   * justify reusing a warm context; the following Claim must be cold. */
  self->force_cold_reset = TRUE;
  self->s3_first_tls_pending = TRUE;
  self->s3_rel61_tls_fallback = FALSE;
  if (idle)
    gx_warm_abandon (self);
  else
    {
      self->warm_valid = FALSE;
      self->production_ready = FALSE;
    }

  fp_warn ("GXFP51A0 NATIVE_S3_PARK idle=%d attempted=%d ack=%d warm=%d",
           idle ? 1 : 0, attempted ? 1 : 0,
           parked ? 1 : 0, warm ? 1 : 0);
  if (idle)
    fpi_device_suspend_complete (dev, NULL);
  else
    fpi_device_suspend_complete (
      dev, fpi_device_error_new (FP_DEVICE_ERROR_NOT_SUPPORTED));
}

/* After true S3, the normal first Claim may arrive after the user has
 * already placed a finger.  Cold TLS and ImageBase preparation then compete
 * with that placement: rejecting contaminated backgrounds is necessary, but
 * it took four-five placements on the reference MateBook.
 *
 * For an IDLE fprintd device libfprint 1.94.100 calls the native resume
 * vfunc before releasing its suspended flag. Perform the detached SPI/GPIO
 * and clean ImageBase preparation on a worker thread and COMPLETE resume
 * only after that worker terminates. Thus a greeter cannot start Verify
 * before the sensor has finished prewarming. No external systemd hook or
 * fprintd restart, and no new background acceptance or reduced threshold.
 *
 * The normal Claim path remains responsible for recovery if this optional
 * prewarm could not establish a clean (finger-off) reference. */
static void
gx_resume_preflight_thread (GTask *task,
                            gpointer source,
                            gpointer task_data,
                            GCancellable *cancellable)
{
  FpDevice *dev = FP_DEVICE (source);
  FpiDeviceGoodix51A0 *self = FPI_DEVICE_GOODIX51A0 (dev);
  g_autoptr(GError) err = NULL;
  gboolean ok = FALSE;
  gboolean fast_settle =
    g_strcmp0 (g_getenv ("GXFP_RESUME_FAST_SETTLE_QA"), "1") == 0;
  gboolean profile =
    g_strcmp0 (g_getenv ("GXFP_RESUME_PERF_QA"), "1") == 0;
  gint64 t_start = g_get_monotonic_time ();
  gint64 t_prepare;

  gx_warm_abandon (self);
  gx_transport_close (self);

  /* The sensor has just resumed from deep sleep. The old host-side TLS
   * context is never reused, and no GPIO pulse may run with live SPI/IRQ.
   *
   * QA-only timing A/B, independently opt-in. Rel71.69 waited 1000ms on
   * either side of the native detached reset (2000ms artificial latency).
   * An unproven faster variant uses 250ms before and 500ms after and retains
   * the already-hardware-validated HIGH10/LOW150 reset. No change to the
   * default or device semantics before a real hardware A/B. */
  g_usleep (fast_settle ? (250 * 1000) : G_USEC_PER_SEC);
  gx_reset_detached_s3_boundary (self);
  g_usleep (fast_settle ? (500 * 1000) : G_USEC_PER_SEC);
  t_prepare = g_get_monotonic_time ();

  if (gx_transport_open (dev, &err))
    {
      ok = gx_cold_prepare (self);
      gx_transport_close (self);
    }
  else
    {
      fp_warn ("GXFP51A0 NATIVE_S3_RESUME_PREWARM transport unavailable: %s",
               err ? err->message : "unknown");
    }

  if (!ok)
    {
      gx_warm_abandon (self);
      gx_transport_close (self);
    }

  if (profile)
    fp_warn ("GXFP51A0 RESUME_PERF_QA fast_settle=%d stage_reset_ms=%ld "
             "cold_tls_background_ms=%ld total_ms=%ld success=%d",
             fast_settle ? 1 : 0,
             (long) ((t_prepare - t_start) / 1000),
             (long) ((g_get_monotonic_time () - t_prepare) / 1000),
             (long) ((g_get_monotonic_time () - t_start) / 1000),
             ok ? 1 : 0);

  g_task_return_boolean (task, ok);
}

static void
gx_resume_preflight_done (GObject *source,
                          GAsyncResult *result,
                          gpointer user_data)
{
  FpDevice *dev = FP_DEVICE (source);
  FpiDeviceGoodix51A0 *self = FPI_DEVICE_GOODIX51A0 (dev);
  gboolean ready = g_task_propagate_boolean (G_TASK (result), NULL);

  if (ready && gx_warm_available (self))
    {
      self->force_cold_reset = FALSE;
      fp_warn ("GXFP51A0 NATIVE_S3_RESUME_PREWARM_READY "
               "clean TLS/ImageBase ready before next fingerprint Claim");
    }
  else
    {
      /* Do not leave the device in libfprint's suspended state after a
       * recoverable calibration miss (for example finger already on glass).
       * The following real Claim uses the established cold path, with
       * fail-closed PAM and the password always available. */
      self->force_cold_reset = TRUE;
      gx_warm_abandon (self);
      fp_warn ("GXFP51A0 NATIVE_S3_RESUME_PREWARM_FALLBACK "
               "clean background unavailable; next Claim retries safely");
    }

  fpi_device_resume_complete (dev, NULL);
}

static void
gx_dev_resume (FpDevice *dev)
{
  FpiDeviceGoodix51A0 *self = FPI_DEVICE_GOODIX51A0 (dev);
  gboolean idle = fpi_device_get_current_action (dev) == FPI_DEVICE_ACTION_NONE &&
                  self->spi_fd < 0 && self->irq_fd < 0;

  self->force_cold_reset = TRUE;
  fp_warn ("GXFP51A0 NATIVE_S3_RESUME_BEGIN idle=%d; native prewarm candidate",
           idle ? 1 : 0);

  if (idle)
    {
      /* Keep all blocking SPI/TLS and ~2-second background acquisition
       * off fprintd's main D-Bus loop, but hold libfprint's resume completion
       * until the transport has been closed and state is consistent. */
      GTask *task = g_task_new (dev, NULL, gx_resume_preflight_done, NULL);
      g_task_run_in_thread (task, gx_resume_preflight_thread);
      g_object_unref (task);
      return;
    }

  /* Active Verify/Enroll cannot be raced by an idle cold-prewarm worker.
   * Existing suspend cancellation handles it, and the next Claim recovers. */
  if (self->spi_fd < 0 && self->irq_fd < 0)
    gx_warm_abandon (self);
  fpi_device_resume_complete (dev, NULL);
}


/* --- verification completion --------------------------------------- */
static void
gx_verify_done (FpiSsm *ssm, FpDevice *dev, GError *error)
{
  GxTask *t = fpi_ssm_get_data (ssm);
  FpPrint *template = NULL;
  g_autoptr(GPtrArray) views = NULL;
  int best = 0;

  if (error)
    {
      /* fprintd/PAM may cancel immediately after receiving an early terminal
       * result. That cancellation is cleanup, not a failed authentication. */
      if (t->match_reported &&
          g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
        {
          fp_info ("verify: caller stopped operation after early result");
          g_clear_error (&error);
          fpi_device_verify_complete (dev, NULL);
          return;
        }
      if (error->domain == FP_DEVICE_RETRY)
        fpi_device_verify_report (dev, FPI_MATCH_ERROR, NULL, g_steal_pointer (&error));
      fpi_device_verify_complete (dev, error);
      return;
    }
  if (!t->probe)
    {
      fpi_device_verify_report (dev, FPI_MATCH_ERROR, NULL,
        fpi_device_retry_new (FP_DEVICE_RETRY_GENERAL));
      fpi_device_verify_complete (dev, NULL);
      return;
    }

  best = t->best;
  fpi_device_get_verify_data (dev, &template);
  views = gx_views_from_print (template);
  fp_info ("verify: %d matches in %d attempt(s) "
           "(threshold %d, %u enrolled views)", best, t->tries,
           GX_MATCH_THRESHOLD, views->len);

  /* Defensive fallback for paths that reached completion without a usable
   * capture-side report. The normal production path reports before WAIT_OFF. */
  if (!t->match_reported)
    fpi_device_verify_report (dev,
                              best >= GX_MATCH_THRESHOLD ? FPI_MATCH_SUCCESS
                                                         : FPI_MATCH_FAIL,
                              NULL, NULL);
  fpi_device_verify_complete (dev, NULL);
}

static void
gx_identify_done (FpiSsm *ssm, FpDevice *dev, GError *error)
{
  GxTask *t = fpi_ssm_get_data (ssm);

  if (error)
    {
      if (t->match_reported &&
          g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
        {
          fp_info ("identify: caller stopped operation after early result");
          g_clear_error (&error);
          fpi_device_identify_complete (dev, NULL);
          return;
        }

      if (error->domain == FP_DEVICE_RETRY)
        {
          fpi_device_identify_report (dev, NULL, NULL,
                                      g_steal_pointer (&error));
          fpi_device_identify_complete (dev, NULL);
          return;
        }

      fpi_device_identify_complete (dev, error);
      return;
    }

  if (!t->probe)
    {
      fpi_device_identify_report (dev, NULL, NULL,
                                  fpi_device_retry_new (FP_DEVICE_RETRY_GENERAL));
      fpi_device_identify_complete (dev, NULL);
      return;
    }

  if (!t->match_reported)
    fpi_device_identify_report (dev,
                                t->best >= GX_MATCH_THRESHOLD
                                  ? t->identify_match
                                  : NULL,
                                NULL, NULL);

  fpi_device_identify_complete (dev, NULL);
}

static void
gx_dev_enroll (FpDevice *dev)
{
  FpiSsm *ssm = fpi_ssm_new (dev, gx_run_state, GX_ST_NUM);
  GxTask *t = g_new0 (GxTask, 1);
  FpPrint *print = NULL;
  FpiPrintType ptype = FPI_PRINT_UNDEFINED;

  t->views = g_ptr_array_new_with_free_func ((GDestroyNotify) gx_sift_free);

  /* Updating an existing print: keep its views and add the new ones.
   * Note this path is unused with fprintd, whose enrol always supplies a fresh
   * FpPrint, so in practice re-enrolling REPLACES the template. */
  fpi_device_get_enroll_data (dev, &print);
  if (print)
    g_object_get (print, "fpi-type", &ptype, NULL);
  if (ptype != FPI_PRINT_UNDEFINED)
    {
      g_autoptr(GPtrArray) old = gx_views_from_print (print);
      guint i;
      for (i = 0; i < old->len; i++)
        g_ptr_array_add (t->views, g_ptr_array_index (old, i));
      g_ptr_array_set_free_func (old, NULL);   /* ownership transferred */
      if (t->views->len)
        fp_info ("enroll: kept %u existing views", t->views->len);
    }
  fpi_ssm_set_data (ssm, t, (GDestroyNotify) gx_task_free);
  fpi_ssm_start (ssm, gx_enroll_done);
}

static void
gx_dev_verify (FpDevice *dev)
{
  FpiSsm *ssm = fpi_ssm_new (dev, gx_run_state, GX_ST_NUM);
  GxTask *t = g_new0 (GxTask, 1);

  t->verifying = TRUE;
  t->cold_contact_gate = FPI_DEVICE_GOODIX51A0 (dev)->cold_verify_placement_budget > 0;
  if (t->cold_contact_gate)
    FPI_DEVICE_GOODIX51A0 (dev)->cold_verify_placement_budget--;
  fpi_ssm_set_data (ssm, t, (GDestroyNotify) gx_task_free);
  fpi_ssm_start (ssm, gx_verify_done);
}

static void
gx_dev_identify (FpDevice *dev)
{
  FpiSsm *ssm = fpi_ssm_new (dev, gx_run_state, GX_ST_NUM);
  GxTask *t = g_new0 (GxTask, 1);

  t->verifying = TRUE;
  t->identifying = TRUE;
  t->cold_contact_gate = FPI_DEVICE_GOODIX51A0 (dev)->cold_verify_placement_budget > 0;
  if (t->cold_contact_gate)
    FPI_DEVICE_GOODIX51A0 (dev)->cold_verify_placement_budget--;
  fpi_ssm_set_data (ssm, t, (GDestroyNotify) gx_task_free);
  fpi_ssm_start (ssm, gx_identify_done);
}

/* ------------------------------------------------------------------ */

static void
fpi_device_goodix51a0_init (FpiDeviceGoodix51A0 *self)
{
  self->spi_fd = -1;
  self->irq_fd = -1;
  self->capture_gap_scale = 0; /* starts nominal at first cold preparation */
  self->capture_clean_streak = 0;
  self->capture_retry_seen = FALSE;
  self->capture_pacing_suppressed = FALSE;
  self->capture_recovery_pending = FALSE;
  self->cold_verify_placement_budget = 2;
  self->warm_last_activity_us = 0;
  self->bg_last_refresh_us = 0;
  self->warm_sleep_delta_us = 0;
  self->warm_sleep_clock_valid = FALSE;
  self->force_cold_reset = FALSE;
}


static void
fpi_device_goodix51a0_finalize (GObject *object)
{
  FpiDeviceGoodix51A0 *self = FPI_DEVICE_GOODIX51A0 (object);

  /*
   * A daemon exit is a process-lifecycle boundary, not another in-process
   * Claim/Release.  Do not reopen SPI just to send TLS close_notify here.
   *
   * reference MateBook 13 rel71.11/71.13 showed that this finalize-only close_notify path can
   * leave ST411 in a persistent half-open TLS state: the next fprintd process
   * then hits repeated digest-check failures until staging or a true power
   * cycle recovers it.  Earlier rel26 deliberately abandoned the stale host
   * TLS state at lifecycle boundaries, and the next real open already owns the
   * deterministic GPIO264 cold reset before starting a fresh handshake.
   *
   * This also matches current Goodix TLS work: a close_notify is not assumed to
   * have cleared MCU session state.  D5 force-unlock remains research-only on
   * GXFP51A0 because our one-shot ST411 experiment received no ACK.
   */
  if (self->tls_up || self->tls || self->warm_valid)
    fp_warn ("GXFP51A0 FINALIZE_TRACE abandoning host warm TLS context; "
             "next process performs native cold reset");

  gx_warm_abandon (self);
  gx_s3_discard_clean_bg (self);
  gx_transport_close (self);
  G_OBJECT_CLASS (fpi_device_goodix51a0_parent_class)->finalize (object);
}


static void
fpi_device_goodix51a0_class_init (FpiDeviceGoodix51A0Class *klass)
{
  FpDeviceClass *dev_class = FP_DEVICE_CLASS (klass);

  dev_class->id = "goodix51a0";
  dev_class->full_name = "Goodix GXFP51A0 SPI (ST411 / experimental)";
  dev_class->type = FP_DEVICE_TYPE_UDEV;
  dev_class->id_table = goodix51a0_id_table;
  dev_class->scan_type = FP_SCAN_TYPE_PRESS;
  dev_class->nr_enroll_stages = GX_ENROLL_STAGES;

  dev_class->temp_hot_seconds = -1;   /* slow capture: no thermal throttling */

  dev_class->probe = gx_dev_probe;
  dev_class->open = gx_dev_open;
  dev_class->close = gx_dev_close;
  dev_class->suspend = gx_dev_suspend;
  dev_class->resume = gx_dev_resume;
  dev_class->enroll = gx_dev_enroll;
  dev_class->verify = gx_dev_verify;
  dev_class->identify = gx_dev_identify;

  G_OBJECT_CLASS (klass)->finalize = fpi_device_goodix51a0_finalize;

  /* Derives the advertised features from the vfuncs implemented above.
   * Must be called LAST: otherwise the device is announced without
   * verification support. FpImageDevice did this for us; deriving straight
   * from FpDevice makes it the driver's job. */
  fpi_device_class_auto_initialize_features (dev_class);

  /* Advertise that an existing print can be updated rather than replaced,
   * for callers that pass one in. */
  dev_class->features |= FP_DEVICE_FEATURE_UPDATE_PRINT;
}
