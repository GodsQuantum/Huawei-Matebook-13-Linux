#!/usr/bin/env python3
"""Source invariant tests for fail-closed FDT-down / TLS delivery telemetry."""
import re
import unittest
from pathlib import Path
R = Path(__file__).resolve().parents[2]
S = (R/"driver/goodix51a0/goodix51a0.c").read_text()
DRAIN = S.split("gx_send_plain_drain (", 1)[1].split("/* ------------------------------------------------------------------ */", 1)[0]
RECV = S.split("gx_take_tls_frame (", 1)[1].split("static int\ngx_fail_get_image_after_tls_timeout", 1)[0]
class ProtocolSafety(unittest.TestCase):
    def test_fdt_down_only_opt_in(self):
        self.assertIn('g_strcmp0 (g_getenv ("GXFP_FDT_DOWN_PROOF_QA"), "1") == 0',DRAIN)
        self.assertEqual(S.count('GXFP_FDT_DOWN_PROOF_QA'),1)
        self.assertIn("body[0] == 0x32u",DRAIN)
    def test_validate_two_responses_and_real_ack(self):
        self.assertRegex(DRAIN,r"got < expected_plain_replies\s*\|\|\s*!ack_seen")
        self.assertIn('FDT_DOWN_PROOF_QA',DRAIN)
        self.assertIn("gx_capture_transport_desync (self)",DRAIN)
        self.assertIn("return FALSE;",DRAIN)
    def test_fdt_validation_precedes_get_image(self):
        self.assertLess(DRAIN.index("FDT_DOWN_PROOF_QA"),DRAIN.index("if (body[0] != 0x20u)\n        return TRUE;"))
    def test_existing_get_image_replay_safety(self):
        self.assertIn("if (tls_seen || ack_seen)\n        return TRUE;",DRAIN)
        self.assertIn('GET_IMAGE had no ACK/TLS',DRAIN)
        self.assertNotIn("gx_retry_get_image_after_tls_timeout",S)
    def test_no_biometric_payload_logging(self):
        self.assertIn('TLS_FRAME_TIMEOUT_DIAG',RECV)
        self.assertRegex(RECV,r"irq_high[^;]*;")
        self.assertRegex(RECV,r"read_error[^;]*;")
        self.assertRegex(RECV,r"plain_frame[^;]*;")
        self.assertNotIn("fp_warn (rec",RECV)
        self.assertNotIn("fp_warn (self->tls_rx",RECV)
    def test_preserve_tls_crypto_and_matcher(self):
        self.assertIn("gx_tls_decrypt_record",S)
        self.assertRegex(S, r"#define\s+GX_MATCH_THRESHOLD\s+7\b")
        self.assertIn("GXFP51A0 NATIVE_S3_RESUME_PREWARM_READY",S)
        self.assertIn("GXFP_S3_FRESH_BG_QA",S)
        self.assertIn("GXFP_S3_LONG_FIRST_QA",S)
        self.assertNotIn("GXFP_CLEANUP_SETTLE_QA",S)
if __name__=="__main__":
    unittest.main()
