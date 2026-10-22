# rel71.51 QA: native fingerprint LOCK / S3 / BOOT / reboot

Status: **software QA candidate, not hardware-certified, not a production
release**. This QA branch carries a driver candidate and an OPTIONAL KDE
Plasma Login Manager 6.8.1 integration. Do NOT automatically replace
Pegasus's installed rel71.49, whose actual human LOCK test succeeded.

## Verified locally
- Native libfprint v1.94.100 C build: PASS.
- rel71.51 S3 race regression: 6/6 PASS; entire fingerprint research suite PASS.
- Official KDE Plasma Login Manager v6.8.1 CMake compilation + DESTDIR
  installation in isolated Arch Linux: PASS.
- KDE login native dual-PAM security and greeter activation: 9/9 PASS.
- Official upstream 6.8.1 release-tarball SHA256 verified; patch applies
  fuzz-free without conflicts: PASS.
- Password PAM left byte-identical to upstream. Dedicated fingerprint PAM
  is fail-closed and does not enable autologin or permit-on-error.

## What rel71.51 changes
libfprint's existing logind delay inhibitor cancels an active operation and
waits for its completion before the suspend request finishes. Driver
callbacks after blocking sensor/TLS workers now check this cancellation
on the main thread, abandon image scoring/retries, and terminate the SSM
instead of opening another capture. The worker MUST finish sensor cleanup
before libfprint releases the inhibitor. No changes to fingerprint threshold
7, 20-enrollment-view format, GPIO, MCU reset timing or firmware.

## KDE startup authentication
The optional 6.8.1 package uses two independent PAM helpers, one for
password, one for fprintd. The first success wins; the other helper is
stopped. Unlike the historical KDE 6.7 experiment, biometric start is
not gated on mouse/keyboard activation of the password overlay. User
and session selection use the modern Plasma 6.8.1 session index. A
failed/cancelled fingerprint attempt re-arms with a bounded delay.
This OPTIONAL integration is for Arch/CachyOS + KDE 6.8.1 only, not
universal for every Linux desktop.

## Not certified and must be physically tested
- LOCK: installed rel71.49 first genuine touch SUCCESS on Pegasus, score
  11/7, time about 2s. rel71.51 not installed or physically tested.
- S3: active-verification race mitigation passes code/tests, but logind
  inhibitor, capture cancellation, fprintd ReleaseDevice, MCU park,
  resumed TLS and power usage require actual supervised hardware tests.
- Cold BOOT and reboot: upstream KDE greeter builds with parallel PAM,
  but actual Pegasus PLM prewarm, GDM alternative, fingerprint sensor
  recovery after power-off and full password fallback remain untested.
- Security: any one-person benchmark cannot prove low population FAR.
  No reduction of strict threshold 7, and no change to enrolled templates.

## Safe deployment gate
Physical tests MUST be controlled by the owner with password fallback,
a recovery TTY/live USB, rollback to stock KDE and rel71.49 packages,
and independent verification that no login session is created without
successful password or enrolled-finger authentication. Do not remote
reboot/suspend, restart login manager or install the QA package unattended.

## Sources
- Public upstream KDE plasma-login-manager v6.8.1: tagged 6.8.1 sources.
- Prior original 6.7 experimental PAM integration retained for provenance.
- GLib/libfprint native API for cancel/suspend; no custom background daemon.
