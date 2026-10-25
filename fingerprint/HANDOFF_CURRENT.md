## 2026-10-25 — rel71.63 S3 early-partial QA source IN DEVELOPMENT

Pegasus is running rollback rel71.55 with active native fprintd, original right-index-finger enrolment, no experimental flags. This branch qa/gxfp51a0-rel71.63-s3-early-partial-20261025 is based on rel71.55 source and contains ONLY opt-in new GXFP_S3_EARLY_PARTIAL_QA behavior: allow an independently double-verified 4-5/6-zone early finger to use authenticated RAM-only clean pre-S3 image reference under existing 12h-age and genuine S3 guards. The real failure was 0x3c at 21:06:12 after natural S3, while historical gate required 0x3f; 4 full calibration attempts then consumed 37.625s and S3 authentication failed. Threshold stays 7, existing SIGFM and necessary cleanup unmodified.

Software QA and native build / isolated Arch / cross distro CI / real S3 hardware gates must ALL pass before promotion. No remote forced reboot/S3, no auto enrollment, no global PAM, PMK, kernel or GPIO112 changes. Canonical current Hall of Fame including rel71.62 S3 FAIL and rel71.62 after-S3 LOCK PASS migrated into this source branch as docs only, NO failed source patches copied. Read docs/rel71.63-s3-early-partial-qa-20261025.md.

## CURRENT 2026-10-25 21:18 CET — rel71.62 S3 NO-GO and VERIFIED rel71.55 rollback

Real Pegasus currently runs rel71.55 again (pacman verified 32/32
intact; active native fprintd PID 198098, empty experimental
Environment), Snapper 1870/1871; no reboot/forced S3.
Owner natural deep S3 rel71.62 failed after multiple contacts:
S3 wake 21:06:08, four touch-contaminated reference attempts,
no valid biometric Verify before its close, prewarm 37.625s.
Distinct subsequent LOCK 21:07 first physical finger PASS, score 9/7,
finger->terminal 1.956s, greeter->terminal 4.843s, subjectively
very slow. Two other genuine S3 cycles had long initialization and
a first 4/7 rejection, documented in
docs/rel71.62-fast-retry-qa-20261025.md. Canonical hardware
CSV/HOF record S3 FAIL and LOCK PASS separately. PR #24 must
remain DRAFT, hardware NO-GO, no merge. Do not misclassify
successful LOCK after S3 as S3 success.
Pursue early-finger S3 session calibration with strict threshold 7;
user enrolment and Linux native fprintd/PAM preserved. No automatic
host suspend/reboot. Use SentinelX via Cloud9 for authorized SSH.

## CURRENT 2026-10-25 17:36 CET — Pegasus rel71.62 QA INSTALLED

Supersedes earlier note claiming rel71.55 currently installed. Pegasus currently runs libfprint-goodix51a0 1.94.100.goodix51a0-71.62 with GXFP_FAST_RETRY_QA=1 (ONLY) in active fprintd, original enrolled right-index-finger intact, package 32 files/0 altered. All 12 GitHub CI PASS for source ae27dd6. SHA256 917502e83a55569236a3a4fe36e4b7916001aacc51b9aa52bc9ac71ea59d953b. Snapper 1868/1869. Native rollback rel71.55 SHA256 f1432414c19fd13bf5abbde9b8b713b3a606e18af89dcdb85d0c6334e5a14deb verified. No reboot or suspend, global PAM/kernel/PMK/GPU changes.

REMAINING HARDWARE GATE: user first physical LOCK trial (one/two/multiple, perceived speed). Read exact fprintd journal for score, READY, DETECTED, release, FAST_RETRY_QA and positive terminal; update HARDWARE_TRIALS.csv and HALL_OF_FAME.md. Reject on any regression; restore pinned rel71.55 if necessary. PR #24 DRAFT, not to merge until hardware and wrong-finger/S3/cold-boot/reboot pass. Candidate only reduces an unnecessary second-press rebase and does NOT fix first image quality. Detailed docs/rel71.62-fast-retry-qa-20261025.md.

## CURRENT 2026-10-25 17:25 CET — rel71.62 fast retry QA source, NOT installed

Pegasus physically powered and verified installed rel71.55, fprintd active,
original right-index-finger enrollment intact; no experimental service
Environment. Previous rel71.61 failed first touch (5/7 rejected, second 7/7),
rollback rel71.55 confirmed, Snapper 1866/1867.

CT700 now has QA rel71.62 opt-in GXFP_FAST_RETRY_QA=1, designed solely to
skip an approximately 996 ms stale background recalibration during
second-physical-press recovery when TLS/FDT/production state intact.
Matcher threshold stays 7, first pose behavior unchanged.
The 4 focused tests and full native software baseline compiled and PASS.
DO NOT claim first-touch improvement; this is a second-press recovery
latency experiment and does not fix original first score 5/7.
The former rel71.61 score-cache feature remains present but OFF by default;
never enable both flags together without separate A/B evidence.
See docs/rel71.62-fast-retry-qa-20261025.md.
Before any deployment ensure package SHA256, CI, rollback and owner physical
LOCK after install; no host reboot/suspend, no PAM changes.

## CURRENT 2026-10-25 02:17 CEST: rel71.61 first-touch NO-GO, rel71.55 restored

Real first LOCK rel71.61 user 2 poses, first 5/7 rejected, second
7/7 pixel rescue accepted; first finger->terminal 7453ms, second
finger->terminal 1761ms. ImageBase 958665ms old, 1s attempted
retry refresh discarded due quick retouch. Installed rel71.55 restored
02:16:58, active fprintd/no QA Environment, package 32/32 intact,
snapshots 1866/1867. PR #23 remains draft NO-GO. Evidence in
docs/rel71.61-score-cache-qa-20261025.md and complete hardware CSV.
Previous installed-rel71.61 entries are historical, not current.
No forced reboot/suspend, use SentinelX only. Keep all Hall of Fame
and pre-Oct13 31-config evidence intact.

## CURRENT 2026-10-25 01:46 — Pegasus rel71.61 QA INSTALLED

This replaces the older note saying rel71.55 is currently installed.
On Pegasus now: libfprint-goodix51a0 1.94.100.goodix51a0-71.61,
fprintd PID 130421 active, GXFP_VERIFY_SCORE_CACHE_QA=1 in ACTUAL
service process Environment. Device GXFP51A0 ST411 enumerated with
right-index-finger enrolled, 32/32 package files intact.
Native package installation Snapper 1864/1865; no host reboot, no
forced S3, no global PAM or kernel/PMK/GPIO112 change.
12/12 CI software green for source commit d3541e1; isolated Arch
package SHA256 4ffbb16eee7a440374a7f72fdec4b8ef534d1963be570bb97385f1704e2ed389.
Rollback pinned rel71.55 SHA256
f1432414c19fd13bf5abbde9b8b713b3a606e18af89dcdb85d0c6334e5a14deb.

CANDIDATE STILL QA ONLY. Measured synthetic 20-view SIGFM comparator
saving ~192ms CPU cannot certify actual KDE unlock. Human physical
LOCK rel71.61 one-placement latency/score is NOT measured yet.
Need user physical test; then capture fprintd logs and append
an exact rel71.61 trial to docs/HARDWARE_TRIALS.csv, regenerate
docs/HALL_OF_FAME.md, commit/push. If failure, restore rel71.55
before any more experimentation. Keep PR #23 DRAFT.
Doc: docs/rel71.61-score-cache-qa-20261025.md.
No automated reboot or forced S3.

## CURRENT 2026-10-25 — rel71.61 opt-in score-cache QA development

Live Pegasus is still production fallback rel71.55, not rel71.61.
Source QA branch qa/gxfp51a0-rel71.61-score-cache-20261025
derived directly from research/gxfp51a0-rel71.55-firsttouch-20261025,
without any rel71.58/.59/.60 S3 modifications.
Changed ONLY verification scoring reuse (fixed threshold 7,
one image per pose, worker-owned exact FpPrint pointer guard and
mandatory cleanup). Flag GXFP_VERIFY_SCORE_CACHE_QA=1, OFF by
default. Other desktop/PAM/kernel/firmware/PMK/GPU settings remain
untouched.

Reused canonical HALL_OF_FAME.md, HARDWARE_TRIALS.csv and historical
evidence files from prior 71.60 QA documentation commit 66d26ff;
this is a docs-only migration, NOT an import of failed 71.60 code.
Full 2026-10-25 real CPU synthetic benchmark at
docs/rel71.61-score-cache-qa-20261025.md and
research/eval/bench-verify-score-cache.py: 20 views median double
386.2613ms, single 194.1066ms, ~192.1546ms saved purely in CPU
matcher (NOT proven full unlock improvement).

Native libfprint build and software suite must pass at final commit,
then isolated Arch package SHA256 and 12 cross-distro GitHub CI tests.
Only afterward install reversible QA onto Pegasus and request one
human normal first-touch LOCK. No remote reboot, suspend, forced login,
enrolment, PAM or personal biometric material extraction.
Keep pinned rel71.55 package for rollback, do not merge to main until
real first-touch/S3/boot security gate passes.

## 2026-10-25 rel71.55 optimisation research — no code change

Pegasus runs pinned rel71.55 and must NOT be switched to the rejected
71.58/71.59/71.60 QA builds. The research branch based on the rel71.55
certification source is
research/gxfp51a0-rel71.55-firsttouch-20261025.
Read docs/RESEARCH_REL7155_FIRSTTOUCH_20261025.md and the latest
historical Hall of Fame (commit 66d26ff on the failed 71.60 QA branch).
NEW Oct22 SAME SENSOR real-SIGFM benchmarks suggest offline testing
20 vs 25 distinct enrolment views, while retaining fixed threshold 7
and validating many unrelated impostor fingers.
Do not adopt RootSIFT, score fusion, multiple scored draws per pose,
or lower threshold based on sibling-model claims.
Instrument capture vs mandatory TLS cleanup as separate bottlenecks;
no user biometric images/templates ever in the public repo.
No driver code or Pegasus configuration was altered by this research.
Continue only with controlled QA and user-supervised hardware trials.

# Public status — GXFP51A0 / ST411 fingerprint driver

**Default:** [rel71.30 recommended preview](https://github.com/GodsQuantum/Huawei-Matebook-13-Linux/releases/tag/matebook13-rel71.30).

**Last fully cold-boot/deep-S3 validated rollback:** [rel71.24](docs/validated-checkpoint-71.24.md).

On the reference MateBook 13, rel71.30 completed enrollment using the **native KDE graphical interface**; eight observed verification captures passed at the unchanged threshold 7. A separate wrong-finger control rejected an unenrolled finger (scores 2, 3, 4) before the enrolled right index succeeded (16). These are **single-machine results**, not a population-level false-acceptance measurement.

Ubuntu 24.04/26.04 source builds pass; Ubuntu 26.04.1 native build and fprintd ABI compatibility also pass. A real Ubuntu fingerprint unlock and MX250 GPU test have **not** yet been completed. Cold-boot/deep-S3 testing of rel71.30 and testing on an independent GXFP51A0 device remain open.

## Start here

- [Driver-only installation, all supported distributions](DRIVER_ONLY.md)
- [Current driver README](README.md)
- [Technical evidence and remaining acceptance criteria](docs/candidate-71.30.md)
- [Hardware safety and privacy requirements](docs/safety.md)
- [Previous investigations](docs/history/)

There is no firmware flashing, automatic enrollment, or redistribution of fingerprint templates or machine-specific keys. Internal personal handoffs and development materials are excluded from the public repository.
