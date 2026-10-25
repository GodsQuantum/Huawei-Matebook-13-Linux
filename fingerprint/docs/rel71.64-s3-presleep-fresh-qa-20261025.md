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

## Review 2026-10-25 of upstream suspend and image handling
Checked Pegasus native D-Bus logind InhibitDelayMaxUSec=5000000 (5s).
Official systemd doc: https://www.freedesktop.org/software/systemd/man/latest/logind.conf.html
Delay-inhibitor rules: https://systemd.io/INHIBITOR_LOCKS/
libfprint FpDevice suspend/resume lifecycle: https://fprint.freedesktop.org/libfprint-dev/FpDevice.html
Goodix SPI imaging references: https://github.com/Sigfrodr/libfprint-goodixtls and https://github.com/szlukabence/goodix-fingerprint-spi-linux
SIGFM small 80x64 matcher: https://github.com/goodix-fp-linux-dev/sigfm
Opt-in 71.64 fresh finger-off ImageBase is only allowed under native S3 suspend with clean FDT.
Follow-up safety review limited this new pre-sleep image refresh to 2.3 seconds, enforced inside plain drain, capture recipe, TLS poll and cleanup. On timeout, do not send SLEEP ACK into potentially desynchronized TLS; leave conservative cold S3 fallback. Deadline resets to zero before leaving presleep path.
Seven targeted safety tests plus full native libfprint source and Hall of Fame baseline PASS. Isolated Arch build and fresh exact-commit CI still required. No physical S3 first-touch proof; DO NOT MERGE/PRODUCTION CERTIFY.

## 2026-10-25 23:49 CET: real Pegasus rel71.64 QA install

Isolated Arch package SHA256 c3df31e8dd06bfc778c661a023333a632f2824b4e841bbdf524ea1c4aa46e932 reverified on CT700, Cloud9 and Pegasus. Native pacman install of libfprint-goodix51a0 1.94.100.goodix51a0-71.64 completed 23:49:33 CET without host reboot or forced sleep. Snapper pre/post root 1888/1889, package 32 total files / zero altered. Actual native fprintd active PID 276490 with only GXFP_S3_FRESH_BG_QA=1. Previous rel71.63 partial-finger QA flag removed. Standard fprintd-list enumerated Goodix GXFP51A0 SPI ST411 and retained original right-index-finger enrollment. Pinned rel71.55 rollback archive SHA256 f1432414c19fd13bf5abbde9b8b713b3a606e18af89dcdb85d0c6334e5a14deb still correct. Pacman lock absent.

Code hardening commit fc3f32e: native logind delay actual D-Bus cap 5,000,000 microseconds; maximum new pre-S3 image refresh attempt 2.3 seconds, explicit fail-closed no SLEEP ACK into uncertain TLS session, seven targeted tests PASS. Full libfprint native source verify PASS and exact driver commit cross-distribution GitHub CI 12/12 PASS. No changes to authentication score threshold 7 or contrast quality gate; no new daemon/hook/PAM/kernel/GPU/PMK.

IMPORTANT: Hardware first-touch after natural deep S3 is STILL UNTESTED under rel71.64. Neither faster presleep refresh nor single-press after wake are certified. User must report first actual natural S3 physical trial; inspect native journal for S3_FRESH_BG_QA refreshed ImageBase before SLEEP, then S3 clean reference saved and first real biometric capture score. A failed presleep refresh with no SLEEP command is a fail-closed event, NOT a success. If any regression, restore pinned rel71.55. PR #26 remains DRAFT until repeated genuine physical hardware/security gates pass.
