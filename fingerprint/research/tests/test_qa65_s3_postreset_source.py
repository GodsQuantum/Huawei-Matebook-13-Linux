#!/usr/bin/env python3
"""Check single opt-in S3 variable; no biometric or TLS semantics change."""
from pathlib import Path
import unittest

ROOT=Path(__file__).resolve().parents[2]
MAIN=(ROOT/'driver/goodix51a0/goodix51a0.c').read_text()
ARCH=(ROOT/'packaging/arch/fprintd-goodix51a0.conf').read_text()
PORT=(ROOT/'install-linux.sh').read_text()

class QA65(unittest.TestCase):
  def test_only_idle_native_resume(self):
    a=MAIN.index('gx_resume_preflight_thread (')
    b=MAIN.index('gx_resume_preflight_done (',a)
    body=MAIN[a:b]
    self.assertIn('qa65_postreset_300ms',body)
    self.assertIn('g_strcmp0 (g_getenv ("GXFP_S3_POSTRESET_300MS_QA"), "1") == 0',body)
    self.assertIn('g_usleep (fast_settle ? (250 * 1000) : G_USEC_PER_SEC);',body)
    self.assertIn('qa65_postreset_300ms ? (300 * 1000) :',body)
    self.assertIn('fast_settle ? (500 * 1000) : G_USEC_PER_SEC',body)
    self.assertIn('gx_reset_detached_s3_boundary (self)',body)
    self.assertIn('gx_cold_prepare (self)',body)
    self.assertIn('gx_transport_close (self)',body)
    self.assertIn('g_task_return_boolean (task, ok);',body)
    self.assertEqual(MAIN.count('GXFP_S3_POSTRESET_300MS_QA'),1)
  def test_packaging_has_explicit_optin(self):
    self.assertEqual(ARCH.count('GXFP_S3_POSTRESET_300MS_QA=1'),1)
    self.assertEqual(PORT.count('GXFP_S3_POSTRESET_300MS_QA=1'),2)
    self.assertNotIn('GXFP_FDT_IRQ_EDGE_WATCH_QA',MAIN+ARCH+PORT)
    self.assertIn('GXFP_RESUME_FAST_SETTLE_QA=1',ARCH)
  def test_security_constants(self):
    self.assertIn('#define GX_MATCH_THRESHOLD            7',MAIN)
    self.assertIn('case 0x32u: expected_plain_replies = 2;',MAIN)
    self.assertIn('gxfp_ack_status_success (ack_status)',MAIN)
    self.assertIn('gx_s3_try_early_finger_reference (self)',MAIN)
    self.assertIn('NATIVE_S3_RESUME_PREWARM_FALLBACK',MAIN)

if __name__=='__main__':
    unittest.main(verbosity=2)
