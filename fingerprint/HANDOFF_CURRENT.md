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
