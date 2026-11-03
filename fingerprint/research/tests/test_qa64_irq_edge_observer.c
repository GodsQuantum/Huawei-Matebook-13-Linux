/* QA64 deterministic Linux GPIO v2 event-parser test: no hardware access. */
#define _POSIX_C_SOURCE 200809L
#include "../../driver/goodix51a0/gx51_transport.h"
#include <assert.h>
#include <linux/gpio.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static void send_event(int fd, unsigned int id, unsigned int offset,
                       uint64_t timestamp_ns, uint32_t seqno)
{
    struct gpio_v2_line_event e;
    memset(&e, 0, sizeof(e));
    e.id = id;
    e.offset = offset;
    e.timestamp_ns = timestamp_ns;
    e.seqno = seqno;
    assert(write(fd, &e, sizeof e) == sizeof e);
}

int main(void)
{
    int fds[2];
    struct gx51_irq_edge_result out;
    const uint64_t ack_ns = 100000000000ull;
    assert(pipe(fds) == 0);
    send_event(fds[1], GPIO_V2_LINE_EVENT_RISING_EDGE, GX51_EXPECTED_HWIRQ,
               ack_ns - 1000000ull, 1);
    send_event(fds[1], GPIO_V2_LINE_EVENT_RISING_EDGE, GX51_EXPECTED_HWIRQ,
               ack_ns + 750000ull, 2);
    send_event(fds[1], GPIO_V2_LINE_EVENT_FALLING_EDGE, GX51_EXPECTED_HWIRQ,
               ack_ns + 4000000ull, 3);
    send_event(fds[1], GPIO_V2_LINE_EVENT_RISING_EDGE, GX51_EXPECTED_HWIRQ,
               ack_ns + 178000000ull, 4);
    send_event(fds[1], GPIO_V2_LINE_EVENT_RISING_EDGE, GX51_EXPECTED_HWIRQ + 1u,
               ack_ns + 250000000ull, 5);
    assert(gx51_irq_edge_observe(fds[0], ack_ns, 0, &out) == 0);
    assert(out.rising_before_ack == 1);
    assert(out.rising_after_ack == 2);
    assert(out.first_after_ack_ms == 0);
    assert(out.last_after_ack_ms == 178);
    assert(out.sequence_gaps == 1); /* falling edge skipped in rising counters */
    assert(gx51_irq_edge_observe(fds[0], ack_ns, 1, &out) == 0);
    assert(out.rising_after_ack == 0);
    assert(out.first_after_ack_ms == -1);
    assert(out.last_after_ack_ms == -1);
    assert(gx51_irq_edge_observe(fds[0], ack_ns, 2001, &out) == -1);
    assert(gx51_irq_edge_observe(-1, ack_ns, 0, &out) == -1);
    close(fds[0]);
    close(fds[1]);
    puts("QA64_IRQ_EDGE_OBSERVER=PASS (typed events, sub-ms, late, missing, bounded)");
    return 0;
}
