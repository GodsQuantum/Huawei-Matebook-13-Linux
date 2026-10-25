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
