#!/usr/bin/env python3
"""Isolated real Linux-PAM regression: reject PAM_IGNORE without modifying /etc/pam.d."""
import ctypes
import ctypes.util
import pathlib
import tempfile
import unittest

class PamMessage(ctypes.Structure):
    _fields_ = [("msg_style", ctypes.c_int), ("msg", ctypes.c_char_p)]

class PamResponse(ctypes.Structure):
    _fields_ = [("resp", ctypes.c_char_p), ("resp_retcode", ctypes.c_int)]

ConvCallback = ctypes.CFUNCTYPE(
    ctypes.c_int,
    ctypes.c_int,
    ctypes.POINTER(ctypes.POINTER(PamMessage)),
    ctypes.POINTER(ctypes.POINTER(PamResponse)),
    ctypes.c_void_p,
)

class PamConv(ctypes.Structure):
    _fields_ = [("conv", ConvCallback), ("appdata_ptr", ctypes.c_void_p)]

@ConvCallback
def no_prompts(num_msg, messages, response, appdata):
    return 19  # PAM_CONV_ERR; these tests must never request credentials.

libname = ctypes.util.find_library("pam")
if libname:
    pam = ctypes.CDLL(libname)
    pam.pam_start_confdir.argtypes = [
        ctypes.c_char_p, ctypes.c_char_p, ctypes.POINTER(PamConv),
        ctypes.c_char_p, ctypes.POINTER(ctypes.c_void_p),
    ]
    pam.pam_start_confdir.restype = ctypes.c_int
    pam.pam_authenticate.argtypes = [ctypes.c_void_p, ctypes.c_int]
    pam.pam_authenticate.restype = ctypes.c_int
    pam.pam_end.argtypes = [ctypes.c_void_p, ctypes.c_int]
    pam.pam_end.restype = ctypes.c_int

def authenticate(auth_lines):
    with tempfile.TemporaryDirectory(prefix="gxfp51a0-pam-") as tmp:
        service = pathlib.Path(tmp, "gxfp51a0-test")
        service.write_text("#%PAM-1.0\n" + auth_lines + "\n", encoding="utf-8")
        handle = ctypes.c_void_p()
        conv = PamConv(no_prompts, None)
        rc = pam.pam_start_confdir(b"gxfp51a0-test", b"pam-sandbox-user", ctypes.byref(conv),
                                   tmp.encode(), ctypes.byref(handle))
        if rc != 0:
            raise RuntimeError("pam_start_confdir failed: " + str(rc))
        try:
            return pam.pam_authenticate(handle, 0)
        finally:
            pam.pam_end(handle, 0)

class FprintPamSafety(unittest.TestCase):
    @unittest.skipUnless(libname and hasattr(pam, "pam_start_confdir"), "pam_start_confdir required")
    def test_insecure_required_plus_permit_can_ignore_biometric(self):
        # pam_debug auth=ignore simulates a module that has no biometric result.
        rc = authenticate("auth required pam_debug.so auth=ignore\n"
                          "auth optional pam_permit.so")
        self.assertEqual(rc, 0, "Expected to reproduce permissive stack flaw")

    @unittest.skipUnless(libname and hasattr(pam, "pam_start_confdir"), "pam_start_confdir required")
    def test_fail_closed_when_module_ignores(self):
        self.assertNotEqual(authenticate("auth required pam_env.so\n"
                                         "auth [success=done default=die] pam_debug.so auth=ignore"), 0)

    @unittest.skipUnless(libname and hasattr(pam, "pam_start_confdir"), "pam_start_confdir required")
    def test_fail_closed_when_module_errors(self):
        self.assertNotEqual(authenticate("auth required pam_env.so\n"
                                         "auth [success=done default=die] pam_debug.so auth=auth_err"), 0)

    @unittest.skipUnless(libname and hasattr(pam, "pam_start_confdir"), "pam_start_confdir required")
    def test_fail_closed_when_module_missing(self):
        self.assertNotEqual(authenticate("auth required pam_env.so\n"
                                         "auth [success=done default=die] pam_nonexistent.so"), 0)

    @unittest.skipUnless(libname and hasattr(pam, "pam_start_confdir"), "pam_start_confdir required")
    def test_success_when_biometric_module_succeeds(self):
        self.assertEqual(authenticate("auth required pam_env.so\n"
                                      "auth [success=done default=die] pam_debug.so auth=success"), 0)

    def test_doctor_detects_unsafe_kde_pam_and_missing_login_integration(self):
        root = pathlib.Path(__file__).resolve().parents[1]
        doctor = (root / "fingerprint/gxfp51a0-doctor.sh").read_text()
        self.assertIn("unsafe KDE biometric PAM", doctor)
        self.assertIn("pam_fprintd[.]so", doctor)
        self.assertIn("pam_permit[.]so", doctor)
        self.assertIn("Plasma Login Manager PAM has no direct fingerprint", doctor)

    def test_template_is_fail_closed(self):
        root = pathlib.Path(__file__).resolve().parents[1]
        template = (root / "fingerprint/integration/kscreenlocker-upstream-s3/kde-fingerprint.pam").read_text()
        self.assertIn("[success=done default=die] pam_fprintd.so", template)
        self.assertNotIn("pam_permit.so", template)
        self.assertNotIn("-auth      required", template)

if __name__ == "__main__":
    unittest.main(verbosity=2)
