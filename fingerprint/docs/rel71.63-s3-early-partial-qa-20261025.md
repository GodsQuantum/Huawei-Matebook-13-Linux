# GXFP51A0 rel71.63 — S3 partial-contact recovery QA (2026-10-25)

STATUS: DEVELOPMENT QA ONLY, not proven on real post-S3 hardware. This branch is based directly on the rel71.55 source. Neither the rejected rel71.61 Verify score-cache nor rel71.62 fast retry changes have been imported.

## Measured failure being addressed

Natural genuine deep S3 wake at 21:06:08 CET. A verified finger-off ImageBase had been saved at preceding S3 park at 18:41:30, age 670133 ms. At S3 wake, FDT detected partial finger contact (0x3c, 4 of 6 zones, mean 285), but gx_s3_try_early_finger_reference only accepted exactly 0x3f (6/6). The driver therefore waited for complete finger-off calibration and repeatedly discarded 4 touch-contaminated backgrounds at 21:06:19, 21:06:27, 21:06:33 and 21:06:40. Native prewarm READY only after 37.625 seconds; S3 Verify closed without successful image or positive authentication. This is a prewarm state-machine problem, not an unsuccessful SIGFM matcher score.

## New candidate behavior

Only if the process has GXFP_S3_EARLY_PARTIAL_QA=1, the sensor is in its genuine pending first S3 TLS lifecycle, and an age-bounded pre-sleep proven-clean ImageBase was preserved, support genuine 4-5/6 zone early contact rather than requiring all 6/6. The primary FDT frame must be >=4 zones and demonstrate >=50 ADC mean fall relative to the saved finger-off baseline, be below the absolute FDT threshold minus 30, and be below the saved FDT activation floor. A SECOND independent real FDT read after the existing 50ms calibration interval must also prove >=4 active zones, the same >=50 ADC mean fall and both absolute/relative checks. Any contradiction reverts to the established conservative finger-off calibration. Completely absent saved data, expired snapshot, transport failure, background contamination or an ordinary non-S3 operation never enable this path. Six-of-six contact uses the historical unchanged path without a second read.

Reuse of ImageBase is NOT biometric acceptance: it only permits the ONE genuine finger image to be evaluated through existing preprocessing, strict SIGFM/per-view photometric rescue and score >=7, followed by required cleanup. No new captured image is registered as baseline and no auto-enrollment, score fusion or multiple scored draws per physical press exist. Native fprintd/PAM/KDE stays in use. ImageBase retains its actual original age and is refreshed at a future safe finger-off boundary.

The previous clean snapshot is RAM-only, captured ONLY after acknowledged idle S3 park, previous clean finger-off FDT confirmation, no dirty state, with pre-sleep reference age at most 2h and CLOCK_BOOTTIME total reference age at most 12h. No saved user fingerprint template, raw biometric frames, PMK or TLS secrets are published in tests or GitHub.

## Release gates

Five source-safety tests and synthetic predicate examples PASS before native build. Run native libfprint verify including the full existing regression suite, verify 9 Hall of Fame tests, validate isolated Arch package and SHA256, then 12 GitHub distro/KDE/PAM CI checks. No hardware upgrade unless rollback rel71.55 is present and checksum-pinned. After staged install, wait for naturally occurring true deep S3; NEVER force suspend/reboot. Owner must verify first-physical-finger behavior. Collect actual native FDT, early-branch logging, time from PM suspend exit to ready, finger to positive terminal, wrong-finger rejection, normal LOCK and cold boot. Update entire hardware ledger and compare to rel71.55. This candidate is NOT certified without real S3 evidence.

## History preserved

Historical rel71.62 S3 FAIL at 21:06 with 37.625sec prewarm and separate rel71.62 LOCK 9/7 success (1.956sec DETECT->positive) are recorded in canonical HARDWARE_TRIALS.csv. Rel71.60/.61 failures are retained. Do NOT count a subsequent LOCK as an S3 PASS. Rel71.55 remains currently installed on Pegasus after verified rollback Snapper 1870/1871.


## 2026-10-25 22:22 CET — installed on Pegasus, awaiting real hardware S3

Native pacman installation of libfprint-goodix51a0 rel71.63 completed at 22:22:32 CET after the user's unrelated pacman -Syu completed and /var/lib/pacman/db.lck disappeared. SHA256 of the isolated Arch QA package 4c67d31538bed8dc6a6e8ce82f984afc41f0b7caea83959d2ecb4b52f3aaee77 was verified both before transfer and on Pegasus. Pinned rel71.55 rollback SHA256 f1432414c19fd13bf5abbde9b8b713b3a606e18af89dcdb85d0c6334e5a14deb was verified.

Snapper root pre/post 1886/1887; pacman package 32 files/0 altered. Native fprintd active PID 256412 since 22:22:19 CET with exactly GXFP_S3_EARLY_PARTIAL_QA=1. No other experimental cache/retry flags, no host reboot, no forced S3, no global PAM or kernel/GPU/PMK changes. Standard fprintd-list returned GXFP51A0 SPI ST411 with original right-index-finger enrollment intact. Pacman free.

All 12 GitHub checks passed on source commit ff514a3, including native libfprint source, KDE 6.8.1, Arch, Fedora, Debian, Alpine, openSUSE, Ubuntu 24.04 and Ubuntu 26.04.

**Hardware gate remains OPEN:** no physical first-touch verification after a genuine natural S3 on rel71.63 has occurred yet. Do not claim S3 fixed, no FAR/FRR proof, no production merge; PR #25 remains DRAFT. When owner naturally resumes from deep sleep and tests first press, inspect scores, prewarm and exact timings; record in docs/HARDWARE_TRIALS.csv and regenerate HALL_OF_FAME.md. If regressions, restore checksum-pinned rel71.55 without reboot.
