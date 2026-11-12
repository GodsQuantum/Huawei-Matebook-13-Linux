# Rejected rel71.56 — hardware evidence (historical only)

Full QA branch and contemporaneous source: [draft PR #17](https://github.com/GodsQuantum/Huawei-Matebook-13-Linux/pull/17).
This trial is NOT part of the rel71.58 driver and is NOT a release candidate.

## 2026-10-24 cold boot at 12:41:23 CEST

First finger scored 4/7 at 12:41:44.846 and was rejected.
Second finger scored 7/7 (existing Pixel Rescue) at 12:41:49.652;
positive terminal at 12:41:50.492. Human reports TWO placements.
Native Claim FAST_READY 82ms is not total cold-boot preparation.
First-touch FAIL. Installed rel71.56 with QA_FAST=1
(the option only affects S3, not a normal cold start).

## 2026-10-24 real deep S3 at 15:11:13.860 CEST

Actual kernel deep S3, owner tried several placements before unlock.
Native QA_FAST S3 prewarm took 27,725 ms (from 15:11:13.936 to
15:11:41.662). Three calibration finger-off warnings and two contaminated
ImageBase captures. First recorded scored finger after prewarm was
detected at 15:11:45.505, scored 10/7, terminal 15:11:47.455
(1,950 ms from finger detection, 33,595 ms from wake).
The owner's early touches occurred while the sensor was still unavailable,
so this is first-touch UX FAIL, despite first *recorded* scored press passing.

Rejected and rolled back to rel71.55 at 15:15 CEST (Snapper 1834/1835).
Do not merge PR #17, claim speedup, or apply the fast option to Pegasus.
