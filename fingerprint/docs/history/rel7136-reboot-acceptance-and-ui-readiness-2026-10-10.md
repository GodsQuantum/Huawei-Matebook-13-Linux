# rel71.36 — post-reboot hardware acceptance, new kernel and UI readiness

2026-10-10. Pegasus MateBook 13, GXFP51A0/ST411. Experimental source branch;
DO NOT promote main/stable until real physical gates A+B+C pass.

## VERIFIED before reboot

- Installed libfprint-goodix51a0 1.94.100.goodix51a0-71.36 and stock
  fprintd 1.94.5 with native --no-timeout, no GXFP helper, timer or hook.
  Enrolled right-index and Bitwarden PIN/Polkit remain unchanged.
- On booted 7.2.9-1-cachyos, REAL S3 at 16:22:08–16:22:14: park opcode
  0x60 ACK, first TLS failed, native hardware recovery restored historic
  rel61 TLS fallback, right-index matched 26/7 at 16:22:39. Normal KDE
  lock had previously succeeded 14/7 in 1 pose.
- Runtime kernel was 7.2.9-1; installed future kernel is 7.2.9-2, with
  matching Limine image/initramfs files and installed NVIDIA DKMS for it.
  User alone will trigger the reboot; assistant has not rebooted Pegasus.
- Package file integrity tests on libfprint/fprintd/PLM/kscreenlocker:
  zero altered files. Stock fprintd unit and existing libfprint drop-in
  only; no new GXFP service. PCI runtime power control remains auto.
- PLM 6.7.5-3.9 custom native package ALREADY supplies independent
  password/fingerprint authenticators, autostart fingerprint PAM,
  forwards PAM information to greeter, continuous bounded scan retries.
  Previous cold boot login 11:08:26 began fingerprint PAM; the REAL
  ready-to-place-finger text only came at 11:08:39 after device opened
  and verification started. A visible password input is independent:
  never block password availability on sensor readiness. Don't display
  a false fingerprint-ready instruction before PAM reports readiness.
  No additional background service or desktop hook is needed.
- KScreenLocker 6.7.5 existing patched resume logic rearms native PAM
  at wake independently of mouse movement. Fprintd after S3 may need
  ~23s to reach verify readiness, partly from finger-on-sensor during
  background calibration; do not weaken calibration contamination checks.

## User-performed physical tests, read journald via Pegasus RDC afterward

A. Cold BOOT after user reboot: confirm running kernel 7.2.9-2-cachyos.
   At Plasma Login Manager, try enrolled right-index WHEN actual fingerprint
   instruction appears. Report success/failure/poses. Password remains
   available separately. Evidence: PLM FingerprintLogin, fprintd Claim,
   native production READY, actual match score >=7 and login session.
   Fingerprint-only initial login may leave KWallet locked without a
   password-derived wallet secret; don't alter wallet or Bitwarden PIN.
B. KDE normal Super+L lock/unlock with right index, success and poses.
   Only if B succeeds continue C.
C. Manually KDE Mettre en veille, wake with mouse/keyboard, try index
   right. Evidence: kernel PM suspend entry (deep) / exit, native S3
   park attempt/ACK, short GPIO reset, rel61 fallback if needed, verified
   image capture READY, accepted matcher score >=7; measure timing and
   possible rejected calibration frames, not just whether screen unlocks.

Assistant must NEVER automatically reboot/lock/suspend user machine;
no new helper/service/timer, no deletion of PMK/fingerprint templates,
no matcher-threshold relaxation, no NFS/mount changes. No unbinding SPI
under a live libfprint FpContext.

## Finalization conditions

Only after A+B+C pass: inspect installed packages, fprintd, PAM and
login logs, running kernel, actual enrollment, extra-unit absence,
and package integrity; update experimental branch and stable/main only
as a separately documented physically field-tested release. Keep known
good recovery package for rollback, update private authoritative
HANDOFF_CURRENT/PROMPT_CURRENT, clean temporary build/research trees,
synchronize Cloud9/Pegasus. A failed gate stays experimental.
