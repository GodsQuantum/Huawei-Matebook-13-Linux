## CURRENT 2026-10-25: rel71.60 failed real LOCK first-touch (2 poses, 7.885s). Rel71.55 restored at 00:10, fprintd active with no QA environment. Historical 71.60 installed notes below are superseded; PR #22 NO-GO. Recovered evidence in docs/rel71.60-s3-prepark-reference-20261024.md and canonical HARDWARE_TRIALS.csv.
## CURRENT 2026-10-24 23:58 CEST — Pegasus rel71.60 QA INSTALLED

Supersedes earlier rel71.55 installed and rel71.59 rollback entries
below (those records remain historical).
Branch qa/gxfp51a0-rel71.60-presleep-reference-20261024,
GitHub draft PR #22, source commit 6825cb4.
Verified source compilation, local regression and 12 GitHub
CI checks PASS. Isolated Arch package SHA256
c38d0e43973d01cd1938fe70e2d4ea7e264f9674aab28bdc4f8f8ea3ea5aa4e3.
Native Pacman installed rel71.60 (34 files 0 altered)
with systemd Env GXFP_S3_PREPARK_FRESH_QA=1,
Snapper 1860/1861. Package hook 35-systemd-udev-reload
timed out after 120s because udev worker for unrelated
sdb blocked; Pacman otherwise committed, subsequent hooks
ran, and db.lck released. No interference with sdb.

fprintd active, native GXFP51A0 ST411 recognized,
right-index-finger enrolled intact.
Read-only desktop doctor new in /usr/bin/gxfp51a0-desktop-native-doctor:
native KDE lock PAM, PLM fingerprint service, Polkit configured
and password fallback preserved; sudo unchanged.
No reboot, sleep, global PAM or GUI intervention.
Pinned rollback still rel71.55, SHA256
f1432414c19fd13bf5abbde9b8b713b3a606e18af89dcdb85d0c6334e5a14deb.

IMPORTANT: rel71.60 first physical LOCK and real S3 still UNTESTED;
never claim production readiness or performance improvement until
the user's touch reports and exact timestamped fprintd/kernel logs
are collected and scored against historical Hall of Fame.
Test expected opt-in S3_PREPARK_FRESH_QA clean_new_reference
and age before vs after S3. No automatic host reboot/suspend.
Refer to docs/rel71.60-s3-prepark-reference-20261024.md
and docs/NATIVE_DESKTOP_INTEGRATION.md.

## CURRENT: 2026-10-24 23:21 CEST rollback to rel71.55 following rel71.59 real S3 FAIL; 71.59 LOCK first touch PASS. Previous installed-rel71.59 note below is historical only.
## 2026-10-24 22:55+02 — rel71.59 now INSTALLED ON PEGASUS

This supersedes ONLY the older 'staged not installed' state below.
Once arch-update released pacman's lock, installed rel71.59 using
checksum-pinned native pacman -U. Snapper 1856/1857. 32 files,
0 altered. fprintd PID 93272 active since 22:55:17, initial
native setup finished 22:55:25 (~7.7s), confirmed single
GXFP51A0 SPI sensor and original right-index-finger enrollment.
Live fprintd process environ GXFP_S3_FIRSTTOUCH_QA=1.
One-time startup SPI ACK retries raised timing 100->300%, so
init/performance stability must be measured in future LOCK/S3.
No host reboot, desktop restart, sleep, kernel/PAM/PMK changes.
Preserved pinned rel71.55 rollback SHA256 f1432414c19fd13bf5abbde9b8b713b3a606e18af89dcdb85d0c6334e5a14deb.
Do not treat fprintd-list success as a hardware first-touch pass.
FIRST physical LOCK under 71.59 is PENDING human finger placement.
When result comes, fetch Pegasus fprintd journal, record timestamps,
score and latency, update docs/HARDWARE_TRIALS.csv and
docs/HALL_OF_FAME.md, run tests, commit and push.
More details in docs/rel71.59-s3-firsttouch-qa-20261024.md.

## 2026-10-24 rel71.59 — staged, NOT installed on Pegasus

Latest draft GitHub PR #21, branch
qa/gxfp51a0-rel71.59-s3-firsttouch-20261024.
Native build PASS and all 12 GitHub CI checks PASS at d4f8713.
Arch package SHA256
d3ab9879c3b3203394ff9ff296092e926524346824e5f84c1af1d42a10f78397
is staged on Pegasus in /tmp/gxfp51a0-rel71.59-qa.pkg.tar.zst.
An attempt at 22:47 CEST did not upgrade the package because a separate
arch-update had pacman -Syu running from 22:22 with db.lck held.
Do NOT kill Pacman or delete its database lock. Previous override
restored: installed version rel71.58, active fprintd with
GXFP_S3_STALE_REBASE_QA=1. No restart, reboot or suspend by us.
rel71.55 known-good package still pinned for rollback, SHA256
f1432414c19fd13bf5abbde9b8b713b3a606e18af89dcdb85d0c6334e5a14deb.
When the *separate* system update has finished, resume the exact QA
installation and hardware certification from the doc
docs/rel71.59-s3-firsttouch-qa-20261024.md; do not skip version-specific
Hall of Fame and physical first-touch tests. PR remains draft.

## ORIGINAL_HOF_RESTORED_20261024 — mandatory source-of-truth

On Oct 24 the earlier mini-ledger of 12 tests was found INCOMPLETE.
The real pre-existing archive was recovered at CT700
CT700 private Pegasus project: research/hall-of-fame/FINGERPRINT_DRIVER_HALL_OF_FAME_2026-10-13.json
SHA256 4545cf38f7d9a33c25d6d7226e5a4ba56bfaa75f6d3f2aef369cd96d7368ff5e,
with 31 version/configuration snapshots, 125 private Git references
and 14 historical rankings. DO NOT delete or overwrite the original.
Its public redacted JSON and table are in
fingerprint/docs/LEGACY_HALL_OF_FAME_20261013.json/.md.
The separate workspace contains 29 rel71.x candidate directories,
index: docs/LEGACY_CANDIDATES_INVENTORY.md. Candidate folder != tested.
Canonical modern measured trials are in docs/HARDWARE_TRIALS.csv
and auto-rendered docs/HALL_OF_FAME.md.

At 20:15 rel71.58 deep S3, after 7.489s prewarm, scored 2/7,3/7,2/7
before finally 21/7 at 20:16:12 (wake->success 34.206s). At 21:32
deep S3, first physical touch quality rejected, next finger 14/7,
wake->success 10.145s. At 21:43 another real short S3 returned
first scored match 9/7 wake->success 9.829s but user reports
second-physical-placement S3; their report could refer to
21:32 or 21:43. Therefore never classify 21:43 as
first-touch success. rel71.58 FAILS current S3 robustness gate.
The opt-in stale rebase code was not activated at 20:15 since
background age <20sec; initial diagnosis was too narrow.
No automatic reboot or sleep; do not declare production.
Record all new trials across branches against this baseline.

## 2026-10-24 Pegasus current QA and Hall of Fame

CAUTION: the older rel71.30 reference below is historic. This Oct24
reference overrides it for the ongoing Pegasus QA branch.
Installed on Pegasus after 19:27 CEST: libfprint-goodix51a0 rel71.58,
KDE PLM 6.8.1, kernel 7.3.0-4-cachyos,
GXFP_S3_STALE_REBASE_QA=1. No rel71.56 FAST option.
The 2026-10-24 19:33 LOCK is first-touch PASS score 11/7,
press-to-terminal 1.902s; subjective "medium". S3 rel71.58 is NOT tested.
The previous rel71.55 had deep S3 first-touch only 2/3; cold login
rel71.56 first-touch 0/1. No production certification.

Required EVERY subsequent trial: add exact timestamp/score/durations
with human placement attestation to docs/HARDWARE_TRIALS.csv, update
the relevant version evidence, regenerate docs/HALL_OF_FAME.md
using scripts/gxfp51a0-hall-of-fame.py --write, run --check and
make -C fingerprint verify, commit AND PUSH. Keep all trials
version-scoped; never count a previous version's test in the new one.
No untested optimization may displace the best-known-safe version.

Branch qa/gxfp51a0-rel71.58-s3-stale-rebase-20261024, draft PR #19.
Source verification and 12 CI gates PASS, but hardware gate PENDING.
The package is rollback-safe to pinned rel71.55:
 /var/lib/gxfp51a0-qa-rollback/rel71.55.pkg.tar.zst
If necessary remove the dedicated 80-gxfp51a0-s3-stale-rebase-qa.conf
fprintd override, reload systemd and pacman -U rel71.55; no reboot.
Do not restart login manager or modify GPIO112, matcher threshold or
firmware. Use SentinelX only for remote device access.

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
