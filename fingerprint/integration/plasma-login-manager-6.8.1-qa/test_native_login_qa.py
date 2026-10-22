#!/usr/bin/env python3
"""Source-gate the KDE PLM 6.8.1 dual-PAM, no-input, fail-closed QA port.
No password or fingerprint material is ever read.  Not a hardware certification.
"""
from pathlib import Path
import subprocess
import unittest
import os
ROOT=Path(__file__).resolve().parent
S=Path(os.environ.get("PLM_QA_UPSTREAM", str(ROOT/"upstream-6.8.1")))
def read(rel):return (S/rel).read_text()
class BootBiometricQA(unittest.TestCase):
 def test_official_parent_and_version(self):
  r=subprocess.run(["git","-C",str(S),"describe","--tags","--always"],text=True,capture_output=True)
  self.assertIn("v6.8.1",r.stdout)
 def test_password_pam_exact_upstream(self):
  expected=subprocess.check_output(["git","-C",str(S),"show","HEAD:data/pam/arch/plasmalogin"],text=True)
  self.assertEqual(expected,read("data/pam/arch/plasmalogin"))
 def test_fingerprint_pam_fail_closed(self):
  pam=read("data/pam/arch/plasmalogin-fingerprint")
  self.assertIn("[success=done default=die] pam_fprintd.so max-tries=1 timeout=30",pam)
  self.assertIn("pam_deny.so",pam)
  self.assertIn("pam_faillock.so preauth",pam)
  for bad in ("pam_permit.so","pam_unix.so","nullok","pam_exec.so","success=ok"):
   self.assertNotIn(bad,pam)
  self.assertIn("plasmalogin-fingerprint",read("data/CMakeLists.txt"))
 def test_biometric_ready_without_mouse_keyboard(self):
  ui=read("src/frontend/greeter/qml/Login.qml")
  self.assertIn("Qt.callLater(root.maybeStartFingerprintLogin)",ui)
  self.assertIn("onVisibleChanged:",ui)
  self.assertIn("if (!root.visible",ui)
  self.assertNotIn("|| !loginScreenUiVisible",ui)
  self.assertIn("interval: 900",ui)
  self.assertIn("onTriggered: root.maybeStartFingerprintLogin()",ui)
  self.assertIn("property Timer fingerprintRetryTimer: Timer {",ui)
  self.assertIn("fingerprintRetryTimer.stop()",ui)
  self.assertIn("fingerprintRetryTimer.restart()",ui)
 def test_state_for_session_matches_password(self):
  state=read("src/frontend/greeter/qml/GreeterState.qml")
  self.assertIn("greeterState.sessionIndex",state)
  self.assertIn("PlasmaLogin.Authenticator.fingerprintLogin(username, sessionType, sessionFileName)",state)
  self.assertIn("PlasmaLogin.Authenticator.login(username, password, sessionType, sessionFileName)",state)
 def test_password_fingerprint_independent_auth_helpers(self):
  d=read("src/daemon/Display.cpp")
  self.assertIn("m_fingerprintAuth(new Auth(this))",d)
  self.assertIn('QStringLiteral("plasmalogin-fingerprint")',d)
  self.assertIn("startAuth(m_auth, user, password, session)",d)
  self.assertIn("if (!startAuth(m_auth, user, password, session))",d)
  self.assertIn("emit loginFailed(socket);",d)
  self.assertIn("startAuth(m_fingerprintAuth, user, QString(), session",d)
  self.assertIn("m_loginCommitted = true",d)
  self.assertIn("m_committedAuth = winner",d)
  self.assertIn("if (winner != m_auth && m_auth->isActive())",d)
  self.assertIn("if (winner != m_fingerprintAuth && m_fingerprintAuth->isActive())",d)
  self.assertIn("m_socketServer->loginCancelled(socket);",d)
 def test_no_extra_session_vt_on_retries(self):
  d=read("src/daemon/Display.cpp")
  reset=d[d.index("void Display::resetAuthContextIfIdle()"):d.index("void Display::slotAuthenticationFinished")]
  self.assertNotIn("m_authContextPrepared = false",reset)
  self.assertIn("m_auth->isActive() || m_fingerprintAuth->isActive()",reset)
  self.assertIn("m_authContextUser == user",d)
  self.assertIn("m_authContextSessionFile == session.fileName()",d)
 def test_biometric_unavailable_does_not_affect_password(self):
  d=read("src/daemon/Display.cpp")
  self.assertIn('m_socketServer->loginCancelled(m_fingerprintSocket);',d)
  self.assertIn("startAuth(m_auth, user, password, session)",d)
  self.assertIn("password remains usable",read("src/frontend/greeter/qml/Login.qml"))
 def test_no_user_capture_uploads_or_custom_daemon(self):
  for file in ("src/frontend/greeter/qml/Login.qml","src/frontend/greeter/qml/GreeterState.qml"):
   code=read(file)
   for bad in ("curl ","wget ","https://api.","autoLogin = true","autologinUser ="):
    self.assertNotIn(bad,code)
if __name__=="__main__":unittest.main(verbosity=2)
