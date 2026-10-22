# Plasma Login Manager 6.8.1: fingerprint QA integration

**Hardware-tested release: NOT CERTIFIED. Do not switch Pegasus's display
manager unattended.** This project produces an optional Arch/CachyOS package,
not the default generic Linux driver. Upstream KDE is not modified in place:
the patch applies cleanly to version 6.8.1.

## Behavior
- At greeter startup, independently begin native fprintd/PAM for the selected
  user without needing mouse movement or keyboard input.
- Password PAM remains unchanged and runs simultaneously through a separate
  helper. A single winning authentication starts the session and stops loser.
- Dedicated plasmalogin-fingerprint uses fail-closed pam_fprintd plus pam_deny,
  NO pam_permit or autologin bypass. Each biometric try is bounded; failed or
  unavailable attempts rearm while the greeter remains visible.
- User/session selection uses the 6.8.1 GreeterState.sessionIndex API.
- A failed attempt must not allocate a fresh login VT repeatedly.

## Build and scope
On a disposable Arch container or VM, run makepkg --verifysource,
makepkg -s and python3 ./test_native_login_qa.py before installation.
A full cmake build and DESTDIR install inside isolated Arch Linux passed.
Hardware behavior is NOT certified. Other KDE PLM versions need fresh porting;
GNOME/GDM uses native fprintd, not this package.

## Production decision
Requires actual cold BOOT, reboot, wrong finger, S3/s2idle, battery usage,
and password-fallback physical tests. No fingerprints or private keys included.
