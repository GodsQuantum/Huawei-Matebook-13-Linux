# QA candidate rel71.70 — S4 mid-capture saved background re-evaluation

Status: **LAB SOURCE BUILD ONLY** on CT700 as of 27 October 2026. NO Pegasus installation, NO kernel or PAM changes, NO claim of a fixed real S4. Base f04b63c (rel71.69 public pre-release).

## Real blocker
True S4 17:54:03: user-reported failed fingerprint unlock. Four repeatedly rejected active-FDT backgrounds touch=0x3f, 38.69s wake-to-prewarm, then verify briefly ready and closed without finger detected, image or score. This is not evidence of user finger contact. Prior S4 also showed delay but eventually authenticated.

## Code hypothesis and minimal fix
The GXFP51A0 native driver checks an independently verified RAM-only pre-sleep no-finger reference at the *start* of cold preparation. If contact occurs **during** the subsequent background capture, the driver rejects the contaminated sample and waits repeatedly for lift even if the proven pre-S4 snapshot remains available. This opt-in lab candidate, GXFP_S4_MIDCAPTURE_REUSE_QA=1, calls the same existing strict gx_s3_try_early_finger_reference helper **after rejecting** the new contaminated frame and before entering multi-second clean-lift waits. That helper requires saved reference and TTL under 12h, full six-zone live FDT touch, strong mean drop, and restores the old proven-clean image baseline; it never treats the touched new frame as idle. The following actual encrypted image still requires all quality and matcher gates, threshold7. If proof fails or option absent, the old behavior runs unaltered.

No new service, sleeps, GPIO reset, global PAM, fingerprint enrollment, native matcher, PSK or TLS bypass. The three known production runtime flags are unchanged; this additional QA flag defaults off. It is **not** enabled in production packaging.

## Current checks
- Regression source tests test_s4_midcapture_saved_background_qa.py (3 cases) and full make verify (includes native libfprint compile) PASS.
- Required next gate: genuine user-initiated S4 with *both* early finger-like contact and normal finger-off scenario, time-to-ready, first-touch success, detector/retry logs, plus ordinary LOCK and S3 to exclude regression. Do not auto-suspend Pegasus.
- Release rel71.69 remains marked experimental publicly; issue #32 open. Candidate PR must remain DRAFT until actual hardware validation.
