/* SPDX-License-Identifier: LGPL-2.1-or-later */
#pragma once
#include <stddef.h>
#include <stdint.h>
#include <linux/spi/spidev.h>

#define GX51_MAX_FRAME 65535u
#define GX51_RESET_LINE 264u
#define GX51_EXPECTED_HWIRQ 48u
#define GX51_SPI_READ_CHUNK_MAX 4096u

struct gx51_outer {
    uint8_t type;
    uint8_t len_lo;
    uint8_t len_hi;
    uint8_t checksum;
};

/* SPI plan with an optional, zero-length CS-assert delay transaction.
 * Return 0 on successful payload transfer, -1 on error; no silent fallback. */
int gx51_spi_message_fd(int fd, const struct spi_ioc_transfer *payload);
int gx51_make_outer(uint8_t type, size_t len, struct gx51_outer *out);
uint8_t gx51_body_checksum(const uint8_t *data, size_t len);
size_t gx51_read_chunk_size(size_t remaining);
int gx51_write_frame_fd(int spi_fd, uint8_t type,
                        const uint8_t *body, size_t len);
int gx51_open_irq_gpio48(void);
/* Optional QA-only GPIO v2 rising-edge observation on the ALREADY OWNED
 * GPIO48 input request. Never requests a second consumer or clocks SPI. */
struct gx51_irq_edge_result {
    unsigned int rising_after_ack;
    unsigned int rising_before_ack;
    unsigned int sequence_gaps;
    int first_after_ack_ms;
    int last_after_ack_ms;
};
int gx51_open_irq_gpio48_ex(int *edge_enabled);
int gx51_irq_edge_observe(int line_fd, uint64_t ack_monotonic_ns,
                         unsigned int timeout_ms,
                         struct gx51_irq_edge_result *result);
int gx51_wait_irq_gpio48(int line_fd, unsigned int timeout_ms);
int gx51_wait_irq_gpio48_low(int line_fd, unsigned int timeout_ms);
int gx51_read_frame_fd(int spi_fd, int irq_gpio_fd, uint8_t *type,
                       uint8_t *body, size_t capacity, size_t *body_len);
int gx51_reset_gpio264(void);
int gx51_reset_gpio264_short(void);
