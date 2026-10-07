# Candidate checkpoint — GXFP51A0 rel71.20

**Date:** 2026-10-06
**Base:** rel71.19 cleanup candidate
**Status:** candidate, not promoted

## Purpose

rel71.20 removes the remaining score-conditioned same-press biometric retry.

Recent Goodix small-sensor work independently demonstrated why this matters: taking the best score across repeated captures of the same physical press changes a fixed-threshold decision into a maximum over several draws and can increase false-accept probability. A retry is legitimate only when the frame is rejected for image quality **before it is scored**.

rel71.19 live testing illustrated the convenience side of the old policy: one physical pose first scored 5/7, then a second image from the same held finger scored 9/7 and passed. rel71.20 deliberately stops doing that.

## New invariant

One physical press may produce up to three sensor frames, but **at most one usable frame is scored**.

```text
capture frame
→ contrast quality gate
→ keypoint quality gate (>=25)
→ score exactly once
→ score >=7: accept this pose
→ score <7: require lift/reposition/new physical pose
```

If contrast fails or the extracted frame has fewer than 25 keypoints, the frame is discarded before matching and RetryCaptureIMG may request another frame while the finger is still down.

Once a frame passes both quality gates, that score is final for the physical pose. There is no 5–6 near-threshold exception and no best-of-N score retry.

## Explicitly unchanged

- match threshold: **7**
- FAST/BRIEF/SIGFM matcher
- template v4
- existing 20-view enrollments
- three-physical-pose outer Verify/Identify budget
- PMK/cache handling
- TLS lifecycle
- ImageBase freshness
- FDT transport/drain behavior
- GET_IMAGE semantics
- pre-enumeration spidev recovery
- GPIO264 reset
- boot prewarm
- no periodic keepalive
- no firmware flashing
- no GPIO112

## Build/test gates

- targeted quality-only retry tests: PASS
- full research suite: PASS
- `make -C fingerprint verify`: PASS
- source manifest: PASS
- reproducible libfprint v1.94.100 build: PASS
- release biometric dump hook: ABSENT
- active sensor I/O during build/verify: NONE
- GPIO/MMIO/firmware actions during build/verify: NONE

## Package

```text
libfprint-goodix51a0-1.94.100.goodix51a0-71.20-x86_64.pkg.tar.zst
SHA256 e997cdba60f8f89148965808e4a1415b2c006a4a0185618d7a53842b7b73dac1
```

Packaged libfprint:

```text
SHA256 4a0b67839405e57dff8abd08d96cb3598fe57b5b0d2efeaa199f75d7b547997c
SONAME libfprint-2.so.2
```

Prestart recovery helper remains byte-identical:

```text
SHA256 ed08bf62951cf64fb5f962f01764338be97faeac2d3dcdbd4013cd2b4fe4fe80
```

Binary audit:

- `decision=final-for-pose`: PRESENT
- contrast quality rejection before scoring: PRESENT
- keypoint quality rejection before scoring: PRESENT
- `MATCH_FUSION`: ABSENT
- old low-score same-press policy marker: ABSENT
- exported symbols vs live rel71.19: IDENTICAL
- `ldd -r`: PASS

## Validation required

Do not promote rel71.20 until:

1. install/restart/prestart recovery succeeds;
2. cold preparation and warm FAST_READY remain healthy;
3. three ordinary KDE locks are tested;
4. logs confirm that every usable physical pose produces only one score;
5. only then run the manual deep-S3 gate.
