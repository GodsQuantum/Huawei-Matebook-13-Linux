#!/usr/bin/env python3
"""Ensure QA64 is opt-in observability ONLY; strict failures stay failures."""
from pathlib import Path
import unittest

D = Path(__file__).resolve().parents[2] / "driver" / "goodix51a0"
driver = (D / "goodix51a0.c").read_text()
transport = (D / "gx51_transport.c").read_text()
pkg = (Path(__file__).resolve().parents[2] / "packaging" /
       "arch" / "fprintd-goodix51a0.conf").read_text()

class QA64Safety(unittest.TestCase):
    def test_opt_in_not_shipped(self):
        self.assertIn('getenv("GXFP_FDT_IRQ_EDGE_WATCH_QA")', transport)
        self.assertIn('strcmp(flag, "1") == 0', transport)
        self.assertEqual(pkg.count("GXFP_FDT_IRQ_EDGE_WATCH_QA=1"), 1)
        self.assertIn("GPIO_V2_LINE_FLAG_EDGE_RISING", transport)
        self.assertIn("req.config.flags = GPIO_V2_LINE_FLAG_INPUT;", transport)
        self.assertIn('req.num_lines = 1;', transport)

    def test_diag_only_on_strict_failure(self):
        pos = driver.index('GXFP51A0 FDT_DOWN_PROOF incomplete before GET_IMAGE')
        watch = driver.index('gx51_irq_edge_observe (')
        recover = driver.index('gx_capture_transport_desync (self);', watch)
        self.assertLess(watch, pos)
        self.assertLess(pos, recover)
        self.assertIn('self->irq_edge_watch_enabled && ack_seen &&', driver)
        self.assertIn('got < expected_plain_replies && fdt_ack_us > 0', driver)
        self.assertIn('watch_ms = 1000;', driver)
        self.assertIn('self->s3_presleep_deadline_us', driver[watch-1300:watch+600])

    def test_transport_never_reads_spi_in_observation(self):
        obs = transport.split('int gx51_irq_edge_observe(', 1)[1]
        obs = obs.split('\nstatic int\ngx51_wait_irq_gpio48_level(', 1)[0]
        self.assertIn('poll(&pfd, 1,', obs)
        self.assertIn('GPIO_V2_LINE_EVENT_RISING_EDGE', obs)
        self.assertNotIn('SPI_IOC', obs)
        self.assertNotIn('gx51_write_frame', obs)
        self.assertNotIn('gx51_read_frame', obs)

    def test_biometric_invariants(self):
        self.assertIn('#define GX_MATCH_THRESHOLD            7', driver)
        self.assertIn('case 0x32u: expected_plain_replies = 2;', driver)
        self.assertIn('G_STATIC_ASSERT (GX_FDT_DOWN_SECOND_REPLY_POLLS == 10);', driver)
        self.assertIn('gxfp_ack_status_success (ack_status)', driver)
        self.assertIn('GXFP51A0 post-capture cleanup failed', driver)

if __name__ == '__main__':
    unittest.main(verbosity=2)
