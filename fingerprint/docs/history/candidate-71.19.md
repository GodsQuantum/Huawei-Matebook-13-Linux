# Candidate checkpoint — GXFP51A0 rel71.19

**Date:** 2026-10-06
**Base:** validated rel71.18
**Status:** candidate, not promoted

## Purpose

rel71.19 is deliberately a **production-path cleanup only**.

rel71.18 proved that the experimental cross-view `MATCH_FUSION` diagnostic can inflate wrong-finger candidates even though it was not used for the authentication decision. Keeping that diagnostic in the hot path therefore had no production value and added extra per-view mask work and logging.

## Exact behavioral change

Live authentication now calls the historical per-view matcher directly:

```text
20 enrolled views
→ gx_sift_match() on each view
→ best single-view score
→ fixed threshold 7
```

The live `gx_score_probe_against_print()` path no longer:

- allocates a fusion mask;
- calls `gx_sift_match_mask()`;
- unions RANSAC inlier keypoints across enrolled views;
- logs `GXFP51A0 MATCH_FUSION`.

The mask-capable matcher primitive is intentionally retained for **offline research tools only**.

## Explicitly unchanged

- SIGFM threshold: **7**
- FAST/BRIEF/RANSAC matcher itself
- template v4
- existing 20-view enrollments
- PMK/cache handling
- TLS lifecycle
- ImageBase freshness
- FDT transport/drain behavior
- GET_IMAGE retry semantics
- pre-enumeration `spidev` recovery
- GPIO264 reset policy
- boot prewarm
- suspend/resume source behavior
- no periodic keepalive
- no firmware flashing
- no GPIO112

## Build / test gates

- full research suite: PASS
- `make -C fingerprint verify`: PASS
- source manifest: PASS
- reproducible libfprint v1.94.100 build: PASS
- release biometric dump hook: ABSENT
- active sensor I/O during build/verify: NONE
- GPIO writes during build/verify: NONE
- MMIO writes during build/verify: NONE
- firmware actions during build/verify: NONE

## Package audit

Package:

```text
libfprint-goodix51a0-1.94.100.goodix51a0-71.19-x86_64.pkg.tar.zst
SHA256 f0f883804f4cd52e091cc8020454a105e565f01caec864699d3b8f22ce52fcc1
```

Packaged libfprint:

```text
SHA256 8741f072fe9f424cc79cb895a1499433663ac1e218ceb28147f42c55200950e2
SONAME libfprint-2.so.2
```

Binary audit:

- `MATCH_FUSION`: ABSENT
- `ldd -r`: PASS, no missing/undefined symbols
- exported symbol set vs live rel71.18: IDENTICAL
- prestart recovery helper: PRESENT
- boot prewarm: PRESENT
- KDE compatibility helper in package: ABSENT
- periodic warm keepalive in package: ABSENT
- external system-sleep hook in package: ABSENT

## Validation still required

Do not promote rel71.19 until:

1. package installation / fprintd restart succeeds;
2. warm FAST_READY remains in the expected ~82–83 ms range;
3. at least three ordinary KDE locks succeed reliably on first/second pose;
4. no `MATCH_FUSION` appears in runtime logs;
5. then validate deep S3 manually.
