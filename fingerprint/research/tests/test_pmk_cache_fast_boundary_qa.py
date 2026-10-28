#!/usr/bin/env python3
from pathlib import Path
import unittest
s=(Path(__file__).resolve().parents[2]/'driver/goodix51a0/goodix51a0.c').read_text()
start=s.index('gx_upload_config_and_reqtls (FpiDeviceGoodix51A0 *self')
end=s.index('static gboolean',start+50)
code=s[start:end]
class PmkFastBoundary(unittest.TestCase):
 def test_optin_only_if_cached_key(self):
  self.assertIn('GXFP_PMK_CACHE_SKIP_RESET_QA',code)
  self.assertIn('self->psk_from_cache',code)
  self.assertIn('if (cached_pmk_fast)',code)
  self.assertIn('if (!cached_pmk_fast)',code)
 def test_factory_stage_still_requires_original_reset_and_firmware_proof(self):
  self.assertIn('gx_factory_acquire_staging_pmk (self, allow_cache)',code)
  self.assertIn('gx_gpio_reset (self);',code)
  self.assertIn('gx_read_fw_version_stage2e (self, fw, sizeof fw)',code)
  self.assertIn('GF_ST411SEC_APP_14115',code)
 def test_tls_mandatory_for_all_fast_cache_paths(self):
  self.assertIn('gx_target_configure (self)',code)
  self.assertIn('GX_REQTLS',code)
  self.assertIn('gxfp_parse_ack (ack, n, 0xd0',code)
  self.assertIn('gx_tls_handshake (self)',s)
  self.assertIn('#define GX_MATCH_THRESHOLD            7',s)
 def test_non_cache_never_skips_reset(self):
  self.assertIn('self->psk_from_cache',code)
  self.assertIn('GXFP51A0 cached PMK',code)
if __name__=='__main__':unittest.main()
