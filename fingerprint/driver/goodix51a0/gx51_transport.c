/* SPDX-License-Identifier: LGPL-2.1-or-later */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "gx51_transport.h"

#include <errno.h>
#include <fcntl.h>
#include <linux/gpio.h>
#include <linux/ioctl.h>
#include <poll.h>
#include <linux/spi/spidev.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>

static int gx51_sleep_us(long usec)
{
    struct timespec ts = {
        .tv_sec = usec / 1000000L,
        .tv_nsec = (usec % 1000000L) * 1000L,
    };
    while (nanosleep(&ts, &ts) < 0) {
        if (errno != EINTR)
            return -1;
    }
    return 0;
}

/* 2026 macOS GXFP51A0 report: 50 us CS assertion before first SCLK
 * eliminated TLS digest/no-IRQ failures on that board. Linux spidev has
 * no direct pre-CS setup field: delay_usecs acts AFTER a transfer.
 * Experimental two-transfer message: zero bytes, 50 us delay while CS
 * stays asserted, followed by unchanged one-payload transfer.
 * Only opt-in, strictly no fallback/replay if controller refuses the
 * message (an initial transfer may already have changed MCU state).
 * This is a lab hypothesis until actual pxa2xx trace verifies CS
 * behavior on Pegasus. No separate GPIO CS or custom daemon. */
int gx51_spi_message_fd(int fd, const struct spi_ioc_transfer *payload)
{
    const char *qa;
    if (fd < 0 || !payload || payload->len == 0)
        return -1;
    qa = getenv("GXFP_SPI_CS_SETUP_QA");
    if (qa && strcmp(qa, "1") == 0) {
        struct spi_ioc_transfer x[2] = {{0}};
        x[0].delay_usecs = 50;
        x[1] = *payload;
        return ioctl(fd, SPI_IOC_MESSAGE(2), x) >= 1 ? 0 : -1;
    }
    return ioctl(fd, SPI_IOC_MESSAGE(1), payload) >= 1 ? 0 : -1;
}

int gx51_make_outer(uint8_t type, size_t len, struct gx51_outer *out)
{
    if (!out || len == 0 || len > GX51_MAX_FRAME)
        return -1;

    out->type = type;
    out->len_lo = (uint8_t)(len & 0xff);
    out->len_hi = (uint8_t)((len >> 8) & 0xff);
    out->checksum = (uint8_t)(out->type + out->len_lo + out->len_hi);
    return 0;
}

uint8_t gx51_body_checksum(const uint8_t *data, size_t len)
{
    uint8_t sum = 0;
    size_t i;

    for (i = 0; i < len; i++)
        sum = (uint8_t)(sum + data[i]);

    return (uint8_t)(0xAAu - sum);
}

size_t gx51_read_chunk_size(size_t remaining)
{
    return remaining > GX51_SPI_READ_CHUNK_MAX
               ? GX51_SPI_READ_CHUNK_MAX
               : remaining;
}

int gx51_write_frame_fd(int spi_fd, uint8_t type,
                        const uint8_t *body, size_t len)
{
    struct gx51_outer hdr;
    struct spi_ioc_transfer x = {0};

    if (spi_fd < 0 || !body || gx51_make_outer(type, len, &hdr) < 0)
        return -1;

    x.tx_buf = (uintptr_t)&hdr;
    x.len = sizeof hdr;
    if (gx51_spi_message_fd(spi_fd, &x) < 0)
        return -1;

    /* Windows GXFP51A0 mode 5: separate synchronous writes, ~2 ms gap. */
    if (gx51_sleep_us(2000) < 0)
        return -1;

    memset(&x, 0, sizeof x);
    x.tx_buf = (uintptr_t)body;
    x.len = (uint32_t)len;
    return gx51_spi_message_fd(spi_fd, &x);
}

int gx51_read_frame_fd(int spi_fd, int irq_gpio_fd, uint8_t *type,
                       uint8_t *body, size_t capacity, size_t *body_len)
{
    struct gx51_outer hdr;
    struct spi_ioc_transfer x = {0};
    size_t len;

    if (spi_fd < 0 || irq_gpio_fd < 0 || !type || !body || !body_len)
        return -1;

    if (gx51_wait_irq_gpio48(irq_gpio_fd, 1200) < 0)
        return -1;

    memset(&hdr, 0, sizeof hdr);
    x.rx_buf = (uintptr_t)&hdr;
    x.len = sizeof hdr;
    if (gx51_spi_message_fd(spi_fd, &x) < 0)
        return -1;

    /* Interpret all-FF before header checksum: this is the target's proven
     * silent/high-Z-or-all-ones observation class. */
    if (hdr.type == 0xff && hdr.len_lo == 0xff &&
        hdr.len_hi == 0xff && hdr.checksum == 0xff)
        return -2;

    if ((uint8_t)(hdr.type + hdr.len_lo + hdr.len_hi) != hdr.checksum)
        return -3;

    len = (size_t)hdr.len_lo | ((size_t)hdr.len_hi << 8);
    if (len == 0 || len > capacity)
        return -4;

    for (size_t off = 0; off < len; ) {
        size_t chunk = gx51_read_chunk_size(len - off);

        memset(&x, 0, sizeof x);
        x.rx_buf = (uintptr_t)(body + off);
        x.len = (uint32_t)chunk;
        if (gx51_spi_message_fd(spi_fd, &x) < 0)
            return -1;
        off += chunk;
    }

    *type = hdr.type;
    *body_len = len;
    return 0;
}

static int gx51_open_int34bb(void)
{
    int n;

    for (n = 0; n < 16; n++) {
        char path[32];
        struct gpiochip_info info = {0};
        int fd;

        if (snprintf(path, sizeof path, "/dev/gpiochip%d", n) < 0)
            continue;

        fd = open(path, O_RDWR | O_CLOEXEC);
        if (fd < 0)
            continue;

        if (ioctl(fd, GPIO_GET_CHIPINFO_IOCTL, &info) == 0 &&
            strncmp(info.label, "INT34BB", 7) == 0 &&
            info.lines > GX51_RESET_LINE)
            return fd;

        close(fd);
    }

    return -1;
}

/* Use the existing GPIO48 request for IRQ-event diagnostics, rather than
 * requesting an already-owned line (which Linux correctly refuses EBUSY).
 * Edge timestamps use CLOCK_MONOTONIC. Unsupported edge requests fall back
 * to the original level-only input, without losing fingerprint availability.
 * The default (QA flag unset) is byte-for-byte the original GPIO v2 setup.
 */
int gx51_open_irq_gpio48_ex(int *edge_enabled)
{
    struct gpio_v2_line_request req = {0};
    const char *flag = getenv("GXFP_FDT_IRQ_EDGE_WATCH_QA");
    const int want_edges = flag && strcmp(flag, "1") == 0;
    int chip = gx51_open_int34bb();

    if (edge_enabled)
        *edge_enabled = 0;
    if (chip < 0)
        return -1;

    req.num_lines = 1;
    req.offsets[0] = GX51_EXPECTED_HWIRQ;
    req.config.flags = GPIO_V2_LINE_FLAG_INPUT |
                       (want_edges ? GPIO_V2_LINE_FLAG_EDGE_RISING : 0);
    if (want_edges)
        req.event_buffer_size = 64;
    strncpy(req.consumer, "goodix51a0-irq", sizeof(req.consumer) - 1);

    if (ioctl(chip, GPIO_V2_GET_LINE_IOCTL, &req) < 0 || req.fd < 0) {
        if (!want_edges) {
            close(chip);
            return -1;
        }
        /* Do not make optional instrumentation a new login dependency. */
        req.config.flags = GPIO_V2_LINE_FLAG_INPUT;
        req.event_buffer_size = 0;
        req.fd = -1;
        if (ioctl(chip, GPIO_V2_GET_LINE_IOCTL, &req) < 0 || req.fd < 0) {
            close(chip);
            return -1;
        }
    } else if (edge_enabled && want_edges) {
        *edge_enabled = 1;
    }

    close(chip);
    return req.fd;
}

int gx51_open_irq_gpio48(void)
{
    return gx51_open_irq_gpio48_ex(NULL);
}

/* Drain timestamped rising events without reading a single SPI byte.
 * Uses the IRQ fd already owned by fprintd and never changes its config.
 * timeout_ms is bounded by the caller and measured against CLOCK_MONOTONIC.
 * Missing IRQ is NOT evidence that the MCU did not generate a response:
 * GPIO controller, sub-ms electrical behaviour and later unobserved time
 * remain separate hypotheses. Observation can never authorize GET_IMAGE.
 */
int gx51_irq_edge_observe(int line_fd, uint64_t ack_monotonic_ns,
                         unsigned int timeout_ms,
                         struct gx51_irq_edge_result *result)
{
    struct timespec now;
    uint64_t deadline_ns;
    uint32_t last_seqno = 0;
    unsigned int records_read = 0;

    if (line_fd < 0 || !result || timeout_ms > 2000)
        return -1;
    memset(result, 0, sizeof(*result));
    result->first_after_ack_ms = -1;
    result->last_after_ack_ms = -1;

    if (clock_gettime(CLOCK_MONOTONIC, &now) < 0)
        return -1;
    deadline_ns = (uint64_t) now.tv_sec * 1000000000u +
                  (uint64_t) now.tv_nsec +
                  (uint64_t) timeout_ms * 1000000u;

    for (;;) {
        struct pollfd pfd = {.fd = line_fd, .events = POLLIN};
        struct gpio_v2_line_event event;
        uint64_t now_ns;
        unsigned int remaining_ms;
        int polled;
        ssize_t got;

        if (clock_gettime(CLOCK_MONOTONIC, &now) < 0)
            return -1;
        now_ns = (uint64_t) now.tv_sec * 1000000000u + (uint64_t) now.tv_nsec;
        remaining_ms = now_ns < deadline_ns
                       ? (unsigned int) ((deadline_ns - now_ns + 999999u)
                                         / 1000000u)
                       : 0;
        polled = poll(&pfd, 1, (int)remaining_ms);
        if (polled < 0) {
            if (errno == EINTR)
                continue;
            return -1;
        }
        if (polled == 0)
            break;
        if (!(pfd.revents & POLLIN))
            return -1;
        got = read(line_fd, &event, sizeof event);
        if (got < 0 && errno == EINTR)
            continue;
        if (got != (ssize_t)sizeof event)
            return -1;
        /* A malicious/noisy line cannot hold a failed auth indefinitely. */
        if (++records_read > 256)
            break;
        if (event.id != GPIO_V2_LINE_EVENT_RISING_EDGE ||
            event.offset != GX51_EXPECTED_HWIRQ)
            continue;
        if (last_seqno && event.seqno > last_seqno + 1)
            result->sequence_gaps += event.seqno - last_seqno - 1;
        last_seqno = event.seqno;
        if (event.timestamp_ns < ack_monotonic_ns) {
            result->rising_before_ack++;
        } else {
            uint64_t delta_ms =
                (event.timestamp_ns - ack_monotonic_ns) / 1000000u;
            if (result->rising_after_ack == 0)
                result->first_after_ack_ms =
                    delta_ms > 2147483647u ? -1 : (int)delta_ms;
            result->last_after_ack_ms =
                delta_ms > 2147483647u ? -1 : (int)delta_ms;
            result->rising_after_ack++;
        }
    }
    return 0;
}

static int
gx51_wait_irq_gpio48_level(int line_fd, unsigned int timeout_ms, int want_high)
{
    struct gpio_v2_line_values val = {
        .bits = 0,
        .mask = 1,
    };
    unsigned int elapsed_ms;

    if (line_fd < 0 || timeout_ms == 0)
        return -1;

    for (elapsed_ms = 0; elapsed_ms < timeout_ms; elapsed_ms++) {
        val.bits = 0;
        val.mask = 1;
        if (ioctl(line_fd, GPIO_V2_LINE_GET_VALUES_IOCTL, &val) < 0)
            return -1;
        if (!!(val.bits & 1) == !!want_high)
            return 0;
        if (gx51_sleep_us(1000) < 0)
            return -1;
    }

    errno = ETIMEDOUT;
    return -1;
}

int gx51_wait_irq_gpio48(int line_fd, unsigned int timeout_ms)
{
    /* Match the proven Stage2E userspace path: sample GPIO48 every 1 ms and
     * only clock the SPI response after the level-high IRQ is observable. */
    return gx51_wait_irq_gpio48_level(line_fd, timeout_ms, 1);
}

int gx51_wait_irq_gpio48_low(int line_fd, unsigned int timeout_ms)
{
    /* A level-high IRQ can remain asserted briefly after a response body has
     * been drained.  Do not launch the next target command until the MCU has
     * visibly returned the line LOW. */
    return gx51_wait_irq_gpio48_level(line_fd, timeout_ms, 0);
}

/* S3-only recovery reset with the short verified GPIO264 pulse from
 * the archived rel71.24 pre-enumeration recovery. No kernel rebinding:
 * caller must close SPI/IRQ before invoking this function. */
int gx51_reset_gpio264_short(void)
{
    struct gpio_v2_line_request req = {0};
    struct gpio_v2_line_values val = {0};
    int chip = gx51_open_int34bb();

    if (chip < 0)
        return -1;

    req.num_lines = 1;
    req.offsets[0] = GX51_RESET_LINE;
    req.config.flags = GPIO_V2_LINE_FLAG_OUTPUT;
    req.config.num_attrs = 1;
    req.config.attrs[0].attr.id = GPIO_V2_LINE_ATTR_ID_OUTPUT_VALUES;
    req.config.attrs[0].attr.values = 0; /* explicitly LOW before pulse */
    req.config.attrs[0].mask = 1;
    strncpy(req.consumer, "goodix51a0-native", sizeof req.consumer - 1);
    if (ioctl(chip, GPIO_V2_GET_LINE_IOCTL, &req) < 0 || req.fd < 0) {
        close(chip);
        return -1;
    }

    val.mask = 1;
    val.bits = 1; /* HIGH reset assertion */
    if (ioctl(req.fd, GPIO_V2_LINE_SET_VALUES_IOCTL, &val) < 0)
        goto fail;
    if (gx51_sleep_us(10000) < 0)
        goto fail;
    val.bits = 0; /* LOW release */
    if (ioctl(req.fd, GPIO_V2_LINE_SET_VALUES_IOCTL, &val) < 0)
        goto fail;
    close(req.fd);
    close(chip);
    return gx51_sleep_us(150000);

fail:
    val.bits = 0;
    (void)ioctl(req.fd, GPIO_V2_LINE_SET_VALUES_IOCTL, &val);
    close(req.fd);
    close(chip);
    return -1;
}

int gx51_reset_gpio264(void)
{
    struct gpio_v2_line_request req = {0};
    struct gpio_v2_line_values val = {0};
    int chip = gx51_open_int34bb();

    if (chip < 0)
        return -1;

    req.num_lines = 1;
    req.offsets[0] = GX51_RESET_LINE;
    req.config.flags = GPIO_V2_LINE_FLAG_OUTPUT;
    strncpy(req.consumer, "goodix51a0", sizeof(req.consumer) - 1);

    if (ioctl(chip, GPIO_V2_GET_LINE_IOCTL, &req) < 0 || req.fd < 0) {
        close(chip);
        return -1;
    }

    val.mask = 1;
    val.bits = 1; /* HIGH: active reset */
    if (ioctl(req.fd, GPIO_V2_LINE_SET_VALUES_IOCTL, &val) < 0)
        goto fail;
    if (gx51_sleep_us(300000) < 0)
        goto fail;

    val.bits = 0; /* LOW: MCU runs */
    if (ioctl(req.fd, GPIO_V2_LINE_SET_VALUES_IOCTL, &val) < 0)
        goto fail;
    if (gx51_sleep_us(600000) < 0)
        goto fail;

    /* Final state deliberately remains LOW. */
    close(req.fd);
    close(chip);
    return 0;

fail:
    /* Fail closed for the experiment, but fail LOW electrically: once the line
     * has been acquired, every error path explicitly releases active-HIGH reset
     * before closing the GPIO request. */
    val.mask = 1;
    val.bits = 0;
    (void)ioctl(req.fd, GPIO_V2_LINE_SET_VALUES_IOCTL, &val);
    close(req.fd);
    close(chip);
    return -1;
}
