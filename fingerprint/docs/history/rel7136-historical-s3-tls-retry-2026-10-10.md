# rel71.36 — restoring historic real S3 TLS recovery phase

**2026-10-10.** Native libfprint v1.94.100 experimental code and tested package.
**NOT INSTALLED as of 15:43 CEST:** Pegasus has an ACTIVE pacman -Syu process
PID 31735 holding /var/lib/pacman/db.lck. Do not remove this genuine lock or
terminate user's package manager. Pegasus remains rel71.35 and has an intact
enrolled right-index print, existing Bitwarden PIN/Polkit, active fprintd.
No extra fingerprint service, timer, hook or helper.

## Verified exact human test, rel71.35

15:33:12 regular KDE fingerprint authentication:
- First physical right-index pose scored 3/7 and was refused.
- Second physical right-index pose scored 7/7 and succeeded.
- Fresh ImageBase obtained before first press (no false score acceptance).
15:33:30 KDE-triggered S3, kernel deep from 15:33:31 to 15:33:35;
native 0x60 ACK with attempted=1, ack=1, warm=1.
15:33:36 first post-S3 short GPIO264 HIGH10ms/LOW150 pulse.
15:33:44 TLS failed; NATIVE_S3_TLS_EARLY_RECOVER immediately escaped.
15:33:45 another full detached transport recovery + short pulse + reopen A8.
15:33:53 TLS again failed; SAME early-exit branch escaped.
15:33:55 whole session gave up and returned cold preparation failed;
zero biometric image/score could be produced after S3.

## What the real old historical Git branches did

Original archived full-history Git bundle was read again in Cloud9 LXC700.
The source of refs fingerprint-rel59-samepress-pacing and
fingerprint-rel61-pam-fingerprint-resume were compared function by function,
including gx_dev_suspend/resume/open, gx_gpio_reset, gx_tls_session,
gx_recover_capture_context, gx_cold_prepare, and gx_wakeup_mcu.
- rel59 did record a real S3 match 8/7, after early weak tries then another
  identify cycle with fresh MCU wake; readiness about 7.8 seconds.
- rel61 did record real S3 match 20/7, with full cold preparation after
  kernel S3, about 21.5 seconds to ready. It retained five bounded normal
  Goodix TLS retries with protocol reset/A8 on each failure, plus PMK staging
  fallback when cached session key could not establish TLS. Not every S3
  trial of those old releases was successful.
- rel66 conditionally succeeded after a Windows 0x60 Sleep ACK; current
  rel71.35 native S3 park already sends 0x60 and gets ACK. The sleep opcode
  alone demonstrably does not solve post-S3 TLS.
- rel71.21 blanket SPI close/reset/reopen around protocol resets was a
  regression. Do not repeat it; keep standard Goodix protocol resets on
  the same open SPI fd.

The newer rel71.33->71.35 optimization introduced
s3_first_tls_pending, forcing gx_tls_session() to BREAK after its first
failed handshake on *every* whole-session retry while the S3 marker was
pending. It also disabled the cached-PMK-fallback staging while this flag
was set. Therefore the actual old rel59/61 full TLS recovery was no longer
reachable after genuine S3 even though it remained present in source.

## rel71.36 change: two distinct native S3 recovery phases

1. After actual S3, first failed TLS handshake aborts quickly and requests
   the existing safe whole-session recovery: close SPI and IRQ, detached
   GPIO264 reset, reopen host handles, verify firmware A8 signature.
2. Once that detached recovery SUCCEEDS, set a separate transient RAM flag
   s3_rel61_tls_fallback. The next TLS session uses original rel59/61
   bounded normal Goodix protocol retry semantics (up to 5 normal attempts,
   timing adaptation, reset/A8 within the existing open descriptor).
   If appropriate it may attempt the original fresh factory staging
   without deleting the validated on-disk PMK cache.
3. Keep s3_first_tls_pending until the entire TLS/image/FDT production
   context is truly ready; then clear BOTH phase markers. Set legacy
   phase FALSE again on next real native suspend.
4. No change to regular warm/non-S3 Claim, matches, score threshold 7,
   image acceptance, enrollment, PMK file, Polkit or Bitwarden PIN.
   No spidev hot-unbind with a live FpContext and no extra service,
   background worker, timer or system sleep hook.

## Software status

Cloud9-only development under LXC700. Native libfprint build PASS,
SOURCE_MANIFEST PASS, full fingerprint/research test suite PASS,
isolated Arch base-devel makepkg PASS, package inspection PASS.
The actual signed-off SHA256 from local artifact manifest:

8a82718bdfce6cf0175f3e34ae8d8a7dc7f6edc6f314e1d03e453197b35069cb

Portable package available under the canonical private Syncthing workspace:
private/candidates/rel71.36-historic-rel61-tls-fallback/

**Hardware QA not performed on rel71.36.** There is a genuine active
pacman -Syu operation on Pegasus at 15:43 CEST, so DO NOT install, remove
/var/lib/pacman/db.lck, or stop the user's system update. After package
manager finishes, check the new installed CachyOS kernel, systemd and
fprintd dependencies BEFORE applying the candidate. Only then validate
a native no-finger DBus Claim, one normal KDE enrolled-finger unlock,
and manually initiated genuine S3, plus returned TLS/FDT/match result.
Do not remotely sleep or reboot the user's laptop without request.

## Pegasus live validation after user finished pacman -Syu, 16:08-16:13

The user's separate pacman transaction completed, db.lck no longer exists.
Updated installed kernel package is linux-cachyos 7.2.9-2, but the currently
BOOTED kernel stays at 7.2.9-1-cachyos until user explicitly reboots.
The new kernel 7.2.9-2 and LTS 6.18.55-2 were compiled/installed together
with NVIDIA DKMS by pacman, and initramfs/Limine files generated. No reboot
was initiated by assistant. Do not confuse tests on 7.2.9-1 with tests on
the not-yet-booted 7.2.9-2 kernel.

rel71.36 package (SHA256 previously recorded) INSTALLED on Pegasus using
pacman -U. Snapper root pre/post snapshots 1557/1558. Existing right-index
template, Bitwarden PIN and Polkit left untouched. Existing stock fprintd
1.94.5-2.1 active with no extra GXFP-specific units/hooks/helpers; PCI SPI
controller power/control left auto; no kernel/sysfs/mount changes.

Native no-finger D-Bus Claim tests:
- 16:09:57 brand-new fprintd initial cold Claim returned a D-Bus error after
  ~25 seconds, with 5 TLS digest/handshake failures. The existing driver
  continued an independent recovery and FINALLY reached production_ready=1
  at 16:10:43 (~46 sec after initiating). Thus COLD first-Claim is not
  consistently below D-Bus operation deadline; record as FAILED first
  Claim, not successful merely because it later recovered.
- 16:11:16 repeated warm Claim 1/2/3 all succeeded at 125ms, 113ms,
  112ms with native FAST_READY at 83ms, 82ms, 82ms respectively.
- 16:11:59 controlled restart of **only stock fprintd**. Fresh cold Claim
  succeeded in ~7431ms; production_ready=1 by 16:12:06.
- 16:12:25 another controlled restart of **only stock fprintd**. Fresh
  cold Claim succeeded in ~7275ms; production_ready=1 by 16:12:33.
- None of these tests captured human fingerprints, tested matcher
  scores after update, or exercised actual post-S3 on rel71.36.

Conclusion: native regular cold Claim works in 2 out of 3 fresh process
initializations, but the first needed >25sec and violated the D-Bus Claim
deadline. Driver cannot yet be pronounced fully reliable. The defining
physical test now required WITHOUT REBOOT is KDE Super+L normal unlock
with enrolled right index, followed IF SUCCESS by manually invoking one
deep S3 from KDE, waking and trying same right index. Read kernel S3
and native fprintd log markers (S3_PARK, NATIVE_S3_SHORT_RESET,
NATIVE_S3_TLS_EARLY_RECOVER, NATIVE_S3_REL61_FALLBACK, ready, match score).
Do not auto-suspend/reboot the machine from RDC. The next kernel 7.2.9-2
behavior must be tested separately only after user authorizes reboot.
