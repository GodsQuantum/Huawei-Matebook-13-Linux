# rel71.56 proposal: faster idle deep-S3 preflight (QA ONLY)

Status: experimental branch; installed on Pegasus for human-operated QA
2026-10-23 14:34:38 Europe/Paris, but NO hardware certification or proven
performance improvement. Never merge as stable on software results alone.

## Verified rel71.55 reference measurements

Pegasus kernel 7.3.0-3-cachyos on 2026-10-23:
- Real automatic **deep S3** entered 14:05:02.096+02, exited
  14:11:54.796+02 (~6m53s asleep).
- Native idle resume begins 14:11:54.823+02.
- Detached GPIO264 HIGH10/LOW150 short reset logged 14:11:55.824+02.
- RAM-only pre-S3 clean background verified with full six-zone early finger
  at 14:11:59.077+02; prewarm READY at the same time.
- Total prewarm ~4.254 s; first physical touch detected 14:11:59.675,
  score 14 at 14:12:00.736 against unchanged matcher threshold 7;
  first-touch success at 14:12:01.748.
- Two unconditional one-second sleeps in idle resume prewarm consume a
  combined two seconds, even if MCU restart and TLS are already responsive.

## QA optimization

With explicit environment GXFP_S3_FAST_PREFLIGHT=1, the first detached
S3 preflight uses 200 ms + 200 ms settle windows. Theoretical fixed-wait
saving: 1.6 s; **this is not a measured hardware improvement**. Normal
installation without the flag remains byte-for-byte equivalent in sleep
duration and retry behavior.

If fast preparation fails, close transport, abandon provisional TLS and
background, then retry once using the original 1 s + reviewed full GPIO264
300/600 ms reset + 1 s before allowing the standard Claim fallback.
Never fast-reset with open SPI/IRQ, reuse failed ImageBase, bypass PAM,
relax the match score 7, or replay accepted GET_IMAGE under a degraded TLS
session. This still needs in-vivo S3/first-touch regression tests.

## Certification gate

Build and source-safety tests passed in CT700, including native libfprint
compilation; reproducible Arch package built in CT700 Docker, SHA256
90373bc965a4bb4a3afec2a05a50049010f34f5aaa1cdb3089c97258d9e4db85.
All 12 CI checks were green at time of activation. It was copied and its
hash independently verified on Pegasus before pacman -U. Native systemd
override /etc/systemd/system/fprintd.service.d/80-gxfp51a0-fast-s3-qa.conf
sets GXFP_S3_FAST_PREFLIGHT=1; fprintd restarted automatically by the
package upgrade and is active; it reports Environment=GXFP_S3_FAST_PREFLIGHT=1.
Installed Arch package is rel71.56, kernel 7.3.0-3, KDE PLM unchanged.
No OS reboot or lock manager restart was performed. Installer invoked the
ordinary pacman/Snapper before-and-after snapshots.

Known-good EXACT rel71.55 previously installed package pinned root-only
to /var/lib/gxfp51a0-qa-rollback/rel71.55.pkg.tar.zst
SHA256 f1432414c19fd13bf5abbde9b8b713b3a606e18af89dcdb85d0c6334e5a14deb.
Separate original KDE+rel71.49 recovery remains available and --check works.
To revert **only** the driver timing trial while preserving all KDE/PAM,
first delete the above override, reload systemd, and reinstall the saved
rel71.55 package via the distro native pacman; never auto-reboot the machine.

Hardware measurement boundary for rel71.56:
2026-10-23T14:34:38+02:00. Historical successful S3 and LOCK measurements
before this belong to rel71.55, NOT this version. On Pegasus, run supervised
S3 repeats including active verification, wake with finger already down,
finger-off cold background, 3 LOCKs, reboot, negative-finger and password
fallback; log actual resume latency and compare with rel71.55.
Do not merge or label production ready without those tests.

Independent KDE lockscreen measurement: the existing KScreenLocker 6.8
greeter takes ~1.7-1.8 s to request the fingerprint reader; reducing this
requires a separate review of upstream PAM worker readiness and should
not use a blanket QML autostart across suspend (battery regression risk).

## 2026-10-24 cold-boot hardware test

Pegasus kernel 7.3.0-4, libfprint rel71.56. Cold boot reported by owner, first pose rejected (score 4/7 at 12:41:44), second pose accepted (score 7/7 via strict Pixel Rescue at 12:41:49). Login accepted at 12:41:50. Cold boot 0/1 first-touch successes. This was not an S3 test, so S3 acceleration remains unverified. Keep score threshold 7, do not claim hardware certification.
## 2026-10-24 hardware S3 regression — rel71.56 FAST QA REJECTED and rolled back

- Owner: natural suspend and resume; many physical finger placements before
  unlock. Kernel confirms real `PM: suspend entry (deep)` at
  13:23:50.260+02, exit 15:11:13.860+02; duration ~107 minutes.
- fprintd native resume began 15:11:13.936 in
  `QA_FAST before_reset_ms=200 after_reset_ms=200`.
- Driver performed the short detached GPIO264 reset at 15:11:14.136;
  `RESET_TRACE GPIO264 300/600 boundary (spi_fd=0)` followed at
  15:11:14.535 during cold preparation. Review SPI descriptor state,
  and do not mistake this for a safe speed improvement.
- Three `CALIBRATION_LIFT_REQUIRED` events at 15:11:17.593,
  15:11:23.735 and 15:11:35.431. Two of the intervening frames were
  discarded as `background contaminated by touch`; calibration retries
  took the resumed device through ~27.725 seconds of preparation.
- `NATIVE_S3_PREFLIGHT_DURATION mode=QA_FAST ready=1 elapsed_ms=27725`
  at 15:11:41.662. The retry/fallback on explicit failure did NOT
  trigger because the slow preflight ultimately returned `ready=1`.
  This is a latency-gate weakness, not a proved false accept.
- First recorded biometric finger after readiness was matched once:
  DETECTED 15:11:45.505, score 10/7 at 15:11:46.550,
  terminal success 15:11:47.455. User's several earlier placements
  occurred while the sensor was still calibrating.
- Contrast: previous rel71.55 full 1s+1s S3 preflight needed just
  4.254 seconds (Oct 23 14:11:54.823 -> 14:11:59.078), and
  authenticated on first recorded/user-reported placement, score 14/7.
  Hardware and elapsed sleep duration differed; the sample size is
  insufficient to claim the speed option alone caused every delay, but
  rel71.56 FAST plainly FAILED the speed and UX certification gate.
- Remediation COMPLETED 2026-10-24 15:15 CEST: on Pegasus,
  removed `/etc/systemd/system/fprintd.service.d/80-gxfp51a0-fast-s3-qa.conf`,
  downgraded from rel71.56 to checksum-verified saved rel71.55
  with native pacman; Snapper snapshots 1834/1835 created automatically.
  Service active since 15:15:28, `Environment=` empty, package
  32/32 files intact, daemon startup completed 15:15:34.
  No power cycle and no changes to PAM/KDE/kernel/fingerprint templates.
- **DO NOT MERGE PR #17 OR REDEPLOY rel71.56 FAST on Pegasus.**
  Its test code can remain as research evidence, not a release candidate.
  Human verification of LOCK and subsequent S3 on rel71.55 is pending.
  Threshold 7 and password fallback remain unchanged.
