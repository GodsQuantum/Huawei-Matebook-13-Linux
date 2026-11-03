#!/usr/bin/env python3
"""Check the full Huawei + standalone GXFP51A0 installers select QA63."""
from pathlib import Path
import re, unittest
ROOT=Path(__file__).resolve().parents[1]
FP=ROOT/"fingerprint"
FLAGS=("GXFP_S3_FRESH_BG_QA", "GXFP_FDT_DOWN_PROOF_QA",
       "GXFP_SPI_CS_SETUP_QA", "GXFP_ROTATION_RESCUE_QA",
       "GXFP_DECOUPLE_ACK_PACING_QA", "GXFP_WIDE_ROTATION_QA")
class HuaweiQA63Install(unittest.TestCase):
 def test_arch_package_proven_and_huawei_exact_version(self):
  pkg=(FP/"packaging/arch/PKGBUILD").read_text()
  self.assertIn("pkgver=1.94.100.goodix51a0.qa63\npkgrel=1",pkg)
  root=(ROOT/"install.sh").read_text()
  self.assertIn("fingerprint_expected_version()",root)
  self.assertIn("--fingerprint-only) DO_PLATFORM=0; DO_GPU=0; DO_FINGERPRINT=1",root)
  self.assertIn('if [[ -e "$launcher" && ! -L "$launcher" ]]',root)
  self.assertIn('"$ROOT/fingerprint/install.sh"',root)
  standalone=(FP/"install-linux.sh").read_text()
  self.assertIn('exec "$ROOT/install-arch.sh"',standalone)
  self.assertIn('PORTABLE_RELEASE="rel71.74-qa63.1"',standalone)
 def test_all_qa_gates_in_one_native_service_and_portable(self):
  arch=(FP/"packaging/arch/fprintd-goodix51a0.conf").read_text()
  portable=(FP/"install-linux.sh").read_text()
  for flag in FLAGS:
   self.assertEqual(arch.count(flag+"=1"),1,flag)
   self.assertGreaterEqual(portable.count(flag+"=1"),2,flag)
  self.assertIn("ExecStart=/usr/lib/fprintd --no-timeout",arch)
  self.assertIn("LimitCORE=0",arch)
  self.assertNotIn("fprintd-mr259",arch)
  self.assertNotIn("GXFP_MATCH_THRESHOLD=6",arch)
 def test_enrollment_is_manual_and_legacy_hof_preserved(self):
  arch=(FP/"install-arch.sh").read_text()
  self.assertIn("Existing templates are preserved",arch)
  self.assertIn("no CLI biometric enrollment",arch)
  ledger=(FP/"docs/HARDWARE_TRIALS.csv").read_text()
  for term in ("s3-20261102-qa59-132144","s3-20261102-qa59-132231",
               "s3-20261102-qa59-132429",
               "s3-20261101-qa57-fprintd259-222809"):
   self.assertIn(term,ledger)
if __name__=="__main__":
 unittest.main(verbosity=2)
