#!/usr/bin/env python3
from pathlib import Path
import unittest
source=(Path(__file__).resolve().parents[2]/'driver/goodix51a0/goodix51a0.c').read_text()
def body(name):
 start=source.index(chr(10)+name+' (')
 a=source.index('{',start); d=0
 for i in range(a,len(source)):
  if source[i]=='{':d+=1
  elif source[i]=='}':
   d-=1
   if d==0:return source[a:i+1]
 raise AssertionError(name)
class ResumeProfile(unittest.TestCase):
 def test_metadata_logging_default_off(self):
  for name in ('gx_cold_prepare','gx_tls_session','gx_factory_acquire_staging_pmk'):
   s=body(name)
   self.assertIn('GXFP_RESUME_STAGE_QA',s)
   self.assertIn('if (stage_qa)',s)
 def test_reset_reasons_explicit(self):
  for reason in ('driverstate_silent','fw_version_silent','tls_in_process_retry',
                 'tls_detached_retry','pmk_staging','pmk_post_stage'):
   self.assertIn('reset_reason='+reason,source)
  for stage in ('driverstate','fw_version','full_capture_context','pmk_cache','tls_attempt'):
   self.assertIn('stage='+stage,source)
 def test_crypto_security_unchanged(self):
  s=body('gx_tls_session')
  self.assertIn('gx_upload_config_and_reqtls (self, TRUE)',s)
  self.assertIn('gx_tls_handshake (self)',s)
  self.assertIn('gx_tls_teardown (self)',s)
  self.assertIn('#define GX_MATCH_THRESHOLD            7',source)
 def test_no_secret_logging(self):
  for secret in ('psk_bytes','pmk_hex','secret_value','raw_frame_data'):
   self.assertNotIn(secret,source)
if __name__=='__main__':unittest.main()
