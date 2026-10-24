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
