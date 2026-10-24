# GXFP51A0 rel71.61 — verify score cache, 25 October 2026

STATUS: QA, not certified on Pegasus. Derived from rel71.55, not from failed S3 experimental branches rel71.58–71.60.

## Change

The original verification matches the first quality-valid authenticated finger image against all enrolled views inside gx_capture_auth_same_press, then repeats the same matcher computation on the same immutable feature frame in gx_capture_done.
With GXFP_VERIFY_SCORE_CACHE_QA=1, the main callback reuses the first score and view counts. It checks that the score exists and the FpPrint identity pointer matches the worker-held enrolled template; otherwise it fails closed. The opt-out path stays unchanged.
The threshold 7, one scored image per finger placement, gallery Identify, 20 enrolled views, image-quality gates, authenticated capture cleanup, MCU/TLS and KDE PAM are not modified. The result is delivered only after mandatory cleanup.

## Reproducible CPU measurement, CT700

Run: python3 fingerprint/research/eval/bench-verify-score-cache.py

| Synthetic matcher metric (20 views, 25 rounds) | Median |
|---|---:|
| Old two matcher passes | 386.2613 ms |
| New one matcher pass | 194.1066 ms |
| Saved matcher CPU | 192.1546 ms |
| CPU comparator speedup | 1.99x |

Deterministically generated synthetic 80x64 frames only; zero real fingerprint material. Double-pass and cached scores are identical. This covers the core SIGFM matcher only, not template photometric rescue, SPI/TLS/image capture, cleanup, KDE or full unlock time. It is not a FAR or FRR benchmark or proof of a 192ms real unlock improvement.

## Verification

- Fixed the forward declaration to agree exactly with the seven-argument gx_capture_auth_same_press definition, rehashed source manifest.
- Five source-safety tests: opt-in only, template identity protection, fixed threshold 7, one scored capture, mandatory cleanup before result; build tests pass.
- Full make -C fingerprint verify including a native libfprint build: PASS on CT700.
- Arch package pkgrel 71.61, portable identifier rel71.61-qa1. CI and real hardware still needed.
- Existing production fallback is rel71.55. Do not reduce threshold, re-enrol the user's stored finger, change PAM, GPU, PMK, kernel, GPIO112, or trigger host reboot or S3 remotely.

## Hardware gates

At least five 1-pose KDE LOCKs, three actual deep S3, three cold boots, three reboots, active Verify during suspend, wrong-finger and password fallback. Compare actual DETECTED_HOLD -> scored frame -> cleanup -> authentication callback latency to rel71.55. Store per-version first-touch observations in canonical HARDWARE_TRIALS.csv and regenerate HALL_OF_FAME.md. Promoting based only on synthetic CPU timing is forbidden.

20 vs 25 enrolled views and S3 image-quality upgrades are separate research tracks requiring private held-out multi-person impostor/genuine datasets. They are NOT resolved by this candidate. Never upload biometric templates, images or secrets to GitHub.
