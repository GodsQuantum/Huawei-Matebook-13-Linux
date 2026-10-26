/* Lab only. Linker wraps ioctl; no real /dev/spidev or GPIO. */
#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <linux/spi/spidev.h>
#include "gx51_transport.h"
static int calls;
static int expected_len;
static int mode;
int __wrap_ioctl(int fd, unsigned long request, ...)
{
    va_list ap;
    struct spi_ioc_transfer *x;
    va_start(ap, request);
    x = va_arg(ap, struct spi_ioc_transfer *);
    va_end(ap);
    assert(fd == 17);
    if (!mode) {
        assert(request == SPI_IOC_MESSAGE(1));
        assert(x->len == (unsigned)expected_len);
    } else {
        assert(request == SPI_IOC_MESSAGE(2));
        assert(x[0].len == 0);
        assert(x[0].delay_usecs == 50);
        assert(x[0].cs_change == 0);
        assert(x[0].tx_buf == 0 && x[0].rx_buf == 0);
        assert(x[1].len == (unsigned)expected_len);
        assert(x[1].cs_change == 0);
        assert(x[1].delay_usecs == 0);
    }
    calls++;
    return expected_len;
}
int main(void)
{
    unsigned char tx[4] = {1,2,3,4}, rx[4] = {0};
    struct spi_ioc_transfer t = {0};
    t.len = 4;
    t.tx_buf = (unsigned long)tx;
    expected_len = 4;
    unsetenv("GXFP_SPI_CS_SETUP_QA");
    assert(gx51_spi_message_fd(17, &t) == 0);
    mode = 1;
    assert(setenv("GXFP_SPI_CS_SETUP_QA","1",1)==0);
    assert(gx51_spi_message_fd(17, &t) == 0);
    t.tx_buf=0; t.rx_buf=(unsigned long)rx;
    assert(gx51_spi_message_fd(17, &t) == 0);
    assert(calls==3);
    assert(gx51_spi_message_fd(-1, &t) == -1);
    t.len=0;
    assert(gx51_spi_message_fd(17, &t) == -1);
    return 0;
}
