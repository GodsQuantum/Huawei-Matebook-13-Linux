/* GXFP51A0 FDT-down typed frame: link against real production target.c. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../../driver/goodix51a0/gx51_target.h"

static void report(uint8_t type, uint8_t out[20])
{
    memset(out, 0, 20);
    out[0] = type;
    out[1] = 17;
    out[2] = 0;
    out[5] = 0x3f;
    out[19] = gxfp_body_checksum(out, 19);
}
int main(void)
{
    uint8_t b[20];
    report(0x32, b);
    assert(gxfp_parse_fdt_down_reply(b, 20));
    assert(!gxfp_parse_fdt_response(b, 20, &(uint8_t){0}, (uint16_t[6]){0}));
    report(0x36, b);
    assert(gxfp_parse_fdt_down_reply(b, 20));
    assert(gxfp_parse_fdt_response(b, 20, &(uint8_t){0}, (uint16_t[6]){0}));
    report(0x34, b);
    assert(!gxfp_parse_fdt_down_reply(b, 20));
    report(0x50, b);
    assert(!gxfp_parse_fdt_down_reply(b, 20));
    report(0x32, b);
    b[19] ^= 1;
    assert(!gxfp_parse_fdt_down_reply(b, 20));
    report(0x32, b);
    b[1] = 16;
    b[19] = gxfp_body_checksum(b, 19);
    assert(!gxfp_parse_fdt_down_reply(b, 20));
    report(0x32, b);
    assert(!gxfp_parse_fdt_down_reply(b, 19));
    assert(!gxfp_parse_fdt_down_reply(b, 21));
    assert(!gxfp_parse_fdt_down_reply(NULL, 20));
    const uint8_t ack[6]={0xb0,0x03,0x00,0x32,0x01,0};
    assert(!gxfp_parse_fdt_down_reply(ack, 6));
    puts("FDT_TYPED_PARSER=PASS (0x32/0x36, wrong tag/checksum/length, ACK excluded)");
    return 0;
}
