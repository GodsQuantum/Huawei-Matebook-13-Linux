# GXFP51A0 rel71.64 — native fresh ImageBase at idle S3 park (QA)

Date: 2026-10-25. STATUS: DEVELOPMENT QA, not certified on real post-S3 hardware. Pegasus remains on installed rel71.63 until isolated package, cross-distro CI and reversible installation gates.

## Evidence, not conjecture

2026-10-25 22:55 deep-S3 test under rel71.63: before SLEEP command the driver saved a 1,971,164-ms-old ImageBase (~32m51s), used it on wake because finger was already down with FDT 0x3f. Native prewarm took 4668ms, but FIRST S3 image failed the STRICT contrast quality gate before any biometric score. Second physical pose scored 15/7 and unlocked. The new rel71.63 partial-contact code was not exercised (all six zones present). Ordinary LOCK had scored 15/7 on first finger in 1869ms.

## New rel71.64 optimization

The best prospect is to avoid exporting a STALE pre-sleep reference into the post-S3 sensor. GXFP_S3_FRESH_BG_QA=1, during the native idle libfprint suspend callback and BEFORE issuing sleep 0x60, only if native FDT proves sensor finger-OFF, current TLS is retained warm/valid, and ImageBase is stale >20s, requests one existing gx_warm_validate ImageBase refresh. It reuses the exact native trusted GET_IMAGE over authenticated TLS and proven clean FDT before and after, then requires a further independent post-refresh FDT clean sample, confirmation that timestamp strictly increased, and freshness below the current 20s bound. Any missing, failed, inconsistent or contaminated data flips pre_sleep_clean=FALSE and explicitly DISABLES saved-reference reuse: the normal verified finger-off cold calibration remains fallback after S3. Normal SLEEP ACK is still sent; no external daemon, hook or extra scheduling.

This patch does NOT touch GX_MATCH_THRESHOLD=7, per-view rescue, enrollment, no-finger image contrast threshold, legitimate / impostor matching, SPI/TLS keys, PMK, PAM, native fprintd or the Linux kernel. New QA is OFF by default; the previous GXFP_S3_EARLY_PARTIAL_QA flag is NOT automatically enabled. Evaluate flags separately to distinguish causes.

## Technique comparison 2026-10-25

External contemporary Goodix reference projects (Sigfrodr/libfprint-goodixtls, szlukabence/goodix-fingerprint-spi-linux) confirm that real clean ImageBase/FDT, native libfprint thread workers and a contrast gate BEFORE keypoint scoring are appropriate for 80x64 / tiny-image devices. Lowering contrast gates or collecting repeated biometric scores until one wins would enlarge impostor success opportunity. Without a private real-image genuine/impostor corpus, do NOT change matcher sensitivity, feature-extraction constants, or enrollment count 20->25. No new automatic template capture.

## Verification still required

- Five new source safety regressions verify clean FDT, opt-in only, refresh before SLEEP, unchanged ACK, conditional fail-closed snapshot and unchanged biometric threshold.
- Full make -C fingerprint verify including native libfprint build, full historical safety suite and canonical Hall of Fame.
- Isolated Arch package and SHA256, all GitHub distro and KDE/PAM CI.
- Real natural S3: verify NATIVE_S3_* logs show presleep refresh succeeded, clean reference fresh in milliseconds, prewarm and FDT ready, first physical fingerprint quality and score, wrong finger rejected, PIN/password fallback. Measure suspend-delay introduced (expected additional authenticated background image capture, potentially around a second) and ensure it does not delay/abort S3. Additional cold-boot/LOCK cases; no forced reboot or S3.
- On any startup, S3, first-touch or security regression, return to pinned rel71.55 (not 71.63, which is still S3-uncertified) and record full failure evidence.

## Hall of Fame guard

Existing natural S3 rel71.63 is first-touch FAIL (two poses) despite eventual success, while ordinary LOCK rel71.63 is first-touch PASS. This is kept in the canonical CSV. The new rel71.64 must never be marked tested/accepted simply because its source compiles.

Source context: https://github.com/Sigfrodr/libfprint-goodixtls and https://github.com/szlukabence/goodix-fingerprint-spi-linux
