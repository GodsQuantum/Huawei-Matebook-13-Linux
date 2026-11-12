#!/usr/bin/env python3
"""Native fprintd/PAM capability assessment must not invent app support."""
import importlib.util
import tempfile
import unittest
from pathlib import Path
SCRIPT=Path(__file__).resolve().parents[2]/"scripts/gxfp51a0-desktop-native-doctor.py"
spec=importlib.util.spec_from_file_location("doctor",SCRIPT)
doctor=importlib.util.module_from_spec(spec)
spec.loader.exec_module(doctor)
class NativeAudit(unittest.TestCase):
 def test_mock_native_kde_polkit_password_fallback(self):
  with tempfile.TemporaryDirectory() as td:
   root=Path(td)
   d=root/"etc/pam.d"
   d.mkdir(parents=True)
   (d/"kde-fingerprint").write_text("auth [success=done default=die] pam_fprintd.so\n")
   (d/"polkit-1").write_text("auth sufficient pam_fprintd.so\nauth include system-auth\n")
   (d/"sudo").write_text("auth include system-auth\n")
   home=root/"home/tester"
   (home/".config").mkdir(parents=True)
   (home/".config/kscreenlockerrc").write_text("[Authenticators]\nFingerprint=true\n")
   got=doctor.audit(root,home)
   self.assertTrue(got["read_only"])
   self.assertEqual(got["kde_fingerprint_selection"],"enabled")
   self.assertTrue(got["pam"]["kde_lock"]["fingerprint_directly_enabled"])
   self.assertTrue(got["pam"]["polkit"]["fingerprint_directly_enabled"])
   self.assertTrue(got["pam"]["polkit"]["password_stack_kept"])
   self.assertFalse(got["pam"]["sudo"]["fingerprint_directly_enabled"])
   self.assertFalse(got["pam"]["plasma_login"]["fingerprint_directly_enabled"])
 def test_no_pam_side_effects(self):
  with tempfile.TemporaryDirectory() as td:
   root=Path(td)
   home=root/"homedir"
   before=list(root.rglob("*"))
   got=doctor.audit(root,home)
   self.assertEqual(before,list(root.rglob("*")))
   self.assertEqual(got["kde_fingerprint_selection"],"distro_or_KDE_default")
   self.assertFalse(got["pam"]["kde_lock"]["fingerprint_directly_enabled"])
 def test_documented_limitations(self):
  with tempfile.TemporaryDirectory() as td:
   result=doctor.audit(Path(td),Path(td))
   warnings=" ".join(result["caveats"]).lower()
   self.assertIn("kwallet",warnings)
   self.assertIn("webauthn",warnings)
   self.assertIn("password fallback",warnings)
if __name__=="__main__":unittest.main(verbosity=2)
