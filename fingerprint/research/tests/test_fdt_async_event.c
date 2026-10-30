#include "../../driver/goodix51a0/gx51_target.h"
#include "../../driver/goodix51a0/gx51_capture_recipe.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void build_event(uint8_t pkt[20], uint8_t tag, uint8_t flags, uint16_t adc)
{
    memset(pkt, 0, 20);
    pkt[0] = tag;
    pkt[1] = 17;
    pkt[5] = flags;
    for (size_t i = 0; i < GXFP_FDT_ZONE_COUNT; i++) {
        pkt[7 + i * 2] = (uint8_t)((adc + i) & 0xffu);
        pkt[8 + i * 2] = (uint8_t)((adc + i) >> 8);
    }
    pkt[19] = gxfp_body_checksum(pkt, 19);
}

int main(void)
{
    struct gxfp_target_packet packet;
    struct gxfp_capture_recipe finger;
    uint16_t zones[GXFP_FDT_ZONE_COUNT];
    uint8_t touch, event[20], saved[20];

    assert(gxfp_build_fdt_async_arm(&packet));
    assert(gxfp_build_finger_capture_recipe(&finger));
    assert(packet.inner_len == finger.steps[1].inner_len);
    assert(memcmp(packet.inner, finger.steps[1].inner, packet.inner_len) == 0);
    assert(packet.inner[0] == 0x32 && packet.inner[3] == 0x0c);

    /* The report is an event many seconds AFTER the separate arm ACK.
     * Parser accepts no unverified/partial/impostor transport packets. */
    build_event(event, 0x32u, 0x3fu, 231u);
    assert(gxfp_parse_fdt_down_event(event, 20, &touch, zones));
    assert(touch == 0x3fu && zones[0] == 231 && zones[5] == 236);
    memcpy(saved, event, 20);
    build_event(event, 0x36u, 0x00u, 353u);
    assert(gxfp_parse_fdt_down_event(event, 20, &touch, zones));
    assert(touch == 0x00 && zones[0] == 353 && zones[5] == 358);
    assert(!gxfp_parse_fdt_down_event(event, 19, &touch, zones));
    event[0] = 0x20;
    assert(!gxfp_parse_fdt_down_event(event, 20, &touch, zones));
    memcpy(event, saved, 20);
    event[19] ^= 0x01;
    assert(!gxfp_parse_fdt_down_event(event, 20, &touch, zones));
    assert(!gxfp_parse_fdt_down_event(NULL, 20, &touch, zones));
    assert(!gxfp_parse_fdt_down_event(saved, 20, NULL, zones));
    assert(!gxfp_parse_fdt_down_event(saved, 20, &touch, NULL));
    puts("test_fdt_async_event: PASS");
    return 0;
}
