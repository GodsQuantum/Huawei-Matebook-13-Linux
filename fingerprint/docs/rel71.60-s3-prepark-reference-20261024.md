# rel71.60 — pre-S3 clean reference (experimental opt-in)

Not production certified. The stable rel71.55 remains the rollback.

## Observation on Pegasus

2026-10-24 real S3, installed rel71.59: native suspend logged a
clean RAM-only ImageBase age 1,051,990ms (~17m32s) BEFORE
deep S3. At resume 21m49s-old reference and early finger
contact yielded scored 0/7. Synchronous clean finger-off waits
after wake repeatedly failed, adding seconds and breaking usability.
Normal later LOCK with 6m49s-old image scored 16/7, so age
alone is not a sufficient predictor.

## New 71.60 behavior (opt-in only)

GXFP_S3_PREPARK_FRESH_QA=1. Only on a real native libfprint
S3-suspend callback with sensor proven finger-OFF, refresh a
reference older than the regular 20s bound BEFORE MCU sleep.
Use existing authenticated TLS GET_IMAGE and require a
strictly newer calibration timestamp plus fresh finger-OFF
FDT. If refreshed image was contaminated, do not snapshot it.
The standard sleep command and ACK are unchanged.
No added work on initial BOOT, routine LOCK or a reference
already younger than 20s. No extra daemon, timer or hook.

Tradeoff: potentially ~1.2s extra during actual suspend entry,
none at the moment of the user's first post-wake biometric
capture, but physical timings remain UNVERIFIED.
Existing firmware and matcher threshold=7 unchanged.
Never enable rejected rel71.56, .58 or .59 QA options together.

Real LOCK, deep S3, cold BOOT, reboot, active Verify suspend,
wrong-finger rejection and password fallback remain untested
with this version. Do not merge until hardware certification.

## 2026-10-24 23:58 CEST — Pegasus QA installed, hardware authentication pending

- Native source and regression suite PASS in CT700. Twelve GitHub CI
  checks PASS for source commit 6825cb4, covering Arch, Debian,
  Ubuntu 24.04/26.04, Fedora, openSUSE, Alpine, shell, native KDE
  and password-safe dual PAM integration.
- Isolated Arch base-devel build version 1.94.100.goodix51a0-71.60
  with the read-only desktop doctor included. Verified SHA-256:
  c38d0e43973d01cd1938fe70e2d4ea7e264f9674aab28bdc4f8f8ea3ea5aa4e3.
  Package transferred via Cloud9 to Pegasus and hash reverified.
- Native pacman upgrade rel71.55 -> rel71.60 committed at
  23:55:47, with Snapper pre 1860 and post 1861. Post-transact
  hook 35-systemd-udev-reload reported a 120s timeout because
  a separate block-device udev worker was stuck on sdb.
  We did NOT terminate the device worker, remove the pacman lock,
  reset sdb or reboot. The passive udev settle eventually timed
  out; subsequent hooks executed and Pacman unlocked.
  This is an independent device/udev issue, NOT a clean zero-error
  package transaction.
- Post-install pacman -Qkk: **34 files, 0 altered**. fprintd
  active since 23:55:48. fprintd-list enumerated exactly one
  Goodix GXFP51A0 ST411 and preserved enrolled right-index-finger.
- The only QA service override is
  /etc/systemd/system/fprintd.service.d/80-gxfp51a0-s3-prepark-qa.conf
  with Environment=GXFP_S3_PREPARK_FRESH_QA=1. No rejected old S3
  QA flags activated. No kernel/GPU/KDE/PAM edits, manual suspend
  or host reboot. Rollback package rel71.55 pinned with SHA256
  f1432414c19fd13bf5abbde9b8b713b3a606e18af89dcdb85d0c6334e5a14deb.
- Native desktop doctor on actual Pegasus:
  kde_lock: direct pam_fprintd TRUE;
  plasma_login: direct pam_fprintd TRUE;
  polkit: direct pam_fprintd TRUE with system-auth fallback;
  sudo: direct pam_fprintd FALSE (distro policy preserved);
  GNOME GDM: not installed, so not falsely claimed.
  KDE fingerprint setting inherited desktop default; native LOCK
  previously worked, but no rel71.60 physical LOCK has been run yet.
  Runtime device discovery and stored fingerprint are proven,
  generic app compatibility is constrained by app API support.

**Release gate still OPEN:** ask owner for one real LOCK first
(score/latency), then a natural or owner-initiated deep S3.
After S3, check native S3_PREPARK_FRESH_QA log for whether the
pre-sleep clean reference was refreshed, refresh duration,
wake prewarm delay, 1st physical press, matcher score, and
first-touch success. Record individual hardware trials under
HARDWARE_TRIALS.csv and regenerate HALL_OF_FAME.md without
mixing rel71.55 baseline measurements. Cold BOOT, reboot, active
verify suspend and wrong-finger/password checks are still pending.
## Hardware NO-GO: LOCK rel71.60, 2026-10-25 00:07 CEST

User observed **two physical poses, slow**. Installed rel71.60,
GXFP_S3_PREPARK_FRESH_QA=1; this was a normal LOCK (not S3).

Native fprintd recorded FAST_READY 83ms at 00:06:59.754.
Retained ImageBase aged 663969ms (~11m04s); warm refresh was
deferred because FDT already detected a finger.
First physical finger DETECTED_HOLD at 00:07:00.252939,
single image **4/7 rejected** at 00:07:01.274464.
LIFT_NOW 00:07:02.152; RELEASED 00:07:04.640.
Clean retry ImageBase discarded 00:07:05.853 because the
next finger arrived during calibration. Second independent
physical finger DETECTED_HOLD 00:07:06.242201 and scored
**8/7 accepted** 00:07:07.259352; final positive terminal
00:07:08.137697. First physical detection to unlock:
**7885 ms**; second detection to unlock **1895 ms**.
This is a confirmed first-touch regression gate FAILURE.
The presleep-only opt-in was not exercised in this normal LOCK,
so causality cannot be attributed exclusively to its source changes.

### Rollback confirmed 00:10 CEST
Pinned rel71.55 package SHA256
f1432414c19fd13bf5abbde9b8b713b3a606e18af89dcdb85d0c6334e5a14deb
reinstalled through native pacman. Snapper 1862/1863;
32 package files, none altered; fprintd active with no QA Environment.
No host reboot, forced suspend, or global PAM edits.
**rel71.60 PR #22 stays draft / NOT production-certified.**
No rel71.60 S3 hardware success demonstrated; do not merge.
