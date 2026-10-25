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
