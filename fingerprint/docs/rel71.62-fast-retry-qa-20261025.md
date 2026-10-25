# GXFP51A0 rel71.62 — opt-in fast retry, 25 October 2026

STATUS: QA only. Not certified on real hardware. Based on rel71.61 source, but previous score-cache experiment stays disabled.

Actual rel71.61 Pegasus first pose scored 5/7 and was rejected; after finger release the driver attempted background refresh on the stored ~16-minute-old ImageBase. That refresh was discarded when the finger returned at 02:02:09.986, wasting approximately 996ms. The second physical pose scored 7/7 with the existing strict photometric rescue.

GXFP_FAST_RETRY_QA=1 enables immediate MCU rearm on a retry after a scored failure, only if TLS, warm context, native production readiness, FDT and background are intact, background is not dirty, and first-S3 TLS recovery is not pending. Otherwise the original stale-background refresh path is unchanged. With no QA flag all old behavior stays in place.

This only targets second-press delay. It does NOT improve first-touch recognition, allow multiple scored frames per physical pose, weaken matcher threshold 7, alter enrollment, or skip mandatory image cleanup. Normal fprintd/PAM/KDE integration, password fallback, PMK, GPIO and kernel remain unchanged.

Four new source-safety tests ensure opt-in and guards. Full native libfprint compilation and software baseline PASS (2026-10-25). Isolated Arch build, cross-distro CI and physical validation must still be completed. No fake unlock speedup claims without measured logs. Rel71.55 remains the actual installed Pegasus fallback until QA deployment.

Before production: five first-touch LOCKs, three real deep S3, three cold boots, three reboots, wrong-finger rejection, password fallback, in-flight Verify suspend. No remote forced suspend/reboot. Fingerprint templates and raw biometric images must remain private and unchanged.

## 2026-10-25 17:35 CET — Pegasus QA installed (hardware test pending)

- All 12 GitHub CI checks passed for candidate source commit ae27dd6, including native libfprint compile, Arch, Debian, Ubuntu 24.04/26.04, Fedora, openSUSE, Alpine and KDE 6.8.1/PAM.
- Isolated Arch base-devel package built correctly. SHA256 917502e83a55569236a3a4fe36e4b7916001aacc51b9aa52bc9ac71ea59d953b, independently reverified on CT700, Cloud9 and Pegasus.
- Native pacman upgrade from pinned rel71.55 to rel71.62 at 17:35:59 CET; Snapper pre/post 1868/1869. Hooks completed without error. 32 package files verified, zero modified.
- Actual fprintd PID 150976 active, Environment=GXFP_FAST_RETRY_QA=1, no other QA flags. Device GXFP51A0 SPI ST411 enumerated, right-index-finger enrollment preserved. Pacman unlocked, no host restart/suspend or PAM/kernel/GPU change.
- Pinned rel71.55 rollback package remains on Pegasus SHA256 f1432414c19fd13bf5abbde9b8b713b3a606e18af89dcdb85d0c6334e5a14deb.
- No human physical first-touch LOCK under rel71.62 has been observed. Therefore hardware gate remains OPEN. Owner must perform physical LOCK and report number of placements; read timestamped fprintd logs for whether FAST_RETRY_QA branch fired and actual release-to-retry timing.
- This candidate addresses second-press delay ONLY, not the score of the first image. Five consecutive first-touch LOCKs, real S3/cold boot/reboot, wrong-finger and password fallback still pending; PR #24 remains DRAFT.

## REAL HARDWARE NO-GO 2026-10-25 21:06 CET — natural deep S3

Owner reports natural S3 fingerprint failed even after multiple physical
finger placements. Kernel PM suspend entry (deep) at 18:41:30.877 CET;
low-level ACPI S3 resume and PM suspend exit at 21:06:08.856 CET.
Native fprintd resume begin 21:06:08.872419. Four attempts at clean
ImageBase failed due to early finger-on contact; calibration frames
were explicitly discarded at 21:06:19.170, 21:06:27.267,
21:06:33.403 and 21:06:40.960. Native prewarm READY only at
21:06:46.497 (~37.625 s after resume begin). Claim FAST_READY in
83ms at 21:06:46.585; Verify READY at 21:06:46.619, but no
DETECTED_HOLD, no scored biometric frame and no positive result.
CLOSE at 21:06:50.128. Thus THIS S3 is a real first-touch FAIL,
with no score (never invent 0/7). rel71.62 FAST_RETRY_QA was not
invoked on this S3 failure.

Also observed earlier today under rel71.62: genuine short S3
18:09:34-18:09:45, initial TLS image timeout and long detached reset
recovery, late positive score 11/7 at 18:10:10; and S3
18:26:37-18:30:12 with scored first finger 4/7 REJECTED at
18:30:36, second finger score 17/7 accepted at 18:30:45.
These are independently instrumented events, NOT additional
owner-attested first-touch S3 successes and not extra CSV successes.
KDE emitted Worker not ready and timed-out PAM-worker messages.

## SEPARATE ordinary KDE LOCK 2026-10-25 21:07 CET

Owner reports FIRST physical placement success, subjectively very slow,
following failed S3. Greeter appeared 21:07:10.653744; native Claim
FAST_READY 84ms at 21:07:12.071. Reference age 25949ms so warm
background refresh attempted; finger landed during the capture and
WARM_REBASE was discarded at 21:07:13.336.
First and ONLY physical DETECTED_HOLD at 21:07:13.540650,
6/6 FDT zones. Score 9/7 PASS at 21:07:14.588955, mandatory cleanup
ended 21:07:15.354135, terminal positive at 21:07:15.496803.
Finger DETECTED->positive 1956ms; greeter->positive 4843ms.
LOCK first-touch PASS must NEVER be counted as S3 success. No
fast retry after failed touch occurred during this successful LOCK.

## VERIFIED ROLLBACK 2026-10-25 21:18:24 CET

Real Pegasus downgraded rel71.62 -> checksum-pinned rel71.55 via
native pacman, with Snapper pre/post 1870/1871.
Pinned rel71.55 package SHA256
f1432414c19fd13bf5abbde9b8b713b3a606e18af89dcdb85d0c6334e5a14deb
was verified before installation. 32 total package files, zero
altered. Removed ONLY experimental rel71.62 fast-retry service override,
reloaded systemd and restarted fprintd. Active fprintd PID 198098,
Environment empty. No host reboot/forced S3, global PAM, enrolled
fingerprint, PMK, GPU or kernel modification. This prior fallback
rel71.55 itself has a known nonzero S3 first-touch failure rate.

FINAL HARDWARE GATE: rel71.62 NO-GO; PR #24 remains DRAFT, no merge.
Prioritize early-finger S3 calibration lifecycle with proper fail-closed
handling, not 2nd-finger score-cache or retry latency.
