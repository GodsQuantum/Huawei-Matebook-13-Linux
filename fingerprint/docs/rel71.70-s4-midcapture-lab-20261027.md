# QA candidate rel71.70 — S4 mid-capture saved background re-evaluation

Status: **QA PACKAGE INSTALLED ON PEGASUS** after complete CT700 native build on 27 October 2026. NO kernel or PAM changes, NO claim of a fixed real S4. Base f04b63c (rel71.69 public pre-release).

## Real blocker
True S4 17:54:03: user-reported failed fingerprint unlock. Four repeatedly rejected active-FDT backgrounds touch=0x3f, 38.69s wake-to-prewarm, then verify briefly ready and closed without finger detected, image or score. This is not evidence of user finger contact. Prior S4 also showed delay but eventually authenticated.

## Code hypothesis and minimal fix
The GXFP51A0 native driver checks an independently verified RAM-only pre-sleep no-finger reference at the *start* of cold preparation. If contact occurs **during** the subsequent background capture, the driver rejects the contaminated sample and waits repeatedly for lift even if the proven pre-S4 snapshot remains available. This opt-in lab candidate, GXFP_S4_MIDCAPTURE_REUSE_QA=1, calls the same existing strict gx_s3_try_early_finger_reference helper **after rejecting** the new contaminated frame and before entering multi-second clean-lift waits. That helper requires saved reference and TTL under 12h, full six-zone live FDT touch, strong mean drop, and restores the old proven-clean image baseline; it never treats the touched new frame as idle. The following actual encrypted image still requires all quality and matcher gates, threshold7. If proof fails or option absent, the old behavior runs unaltered.

No new service, sleeps, GPIO reset, global PAM, fingerprint enrollment, native matcher, PSK or TLS bypass. The three known production runtime flags are unchanged; this additional QA flag defaults off. It is **not** enabled in production packaging.

## Current checks
- Regression source tests test_s4_midcapture_saved_background_qa.py (3 cases) and full make verify (includes native libfprint compile) PASS.
- Required next gate: genuine user-initiated S4 with *both* early finger-like contact and normal finger-off scenario, time-to-ready, first-touch success, detector/retry logs, plus ordinary LOCK and S3 to exclude regression. Do not auto-suspend Pegasus.
- Release rel71.69 remains marked experimental publicly; issue #32 open. Candidate PR must remain DRAFT until actual hardware validation.

## Additional independent performance investigation

- GXFP_RESUME_FAST_SETTLE_QA=1 reduces the two historical artificial waits around the already-proven short detached reset from 1000/1000 milliseconds to **250/500 ms**, an unproven **1.25 s theoretical saving**. It preserves the exact GPIO264 HIGH10/LOW150 pulse and TLS, and defaults off. Requires real S3/S4 hardware verification; could regress on a slower power rail. Do not enable merely because CI passes.
- GXFP_RESUME_PERF_QA=1 emits only reset+settle, cold TLS/background and full-prewarm durations; GXFP_CAPTURE_PERF_QA=1 emits image transfer, SIFT feature extraction and matching times, never raw pixels or biometric templates. Both opt-in (slight log overhead). Neither makes any authentication decision.
- Source regression checks test_resume_fast_settle_qa.py and test_capture_performance_qa.py pass. The complete native compile and fingerprint verify PASS on isolated CT700.
- Hardware trial protocol, once QA package installed with verified rollback: (1) ordinary LOCK first physical press, (2) naturally entered S4, keep finger off sensor until asked; then repeat with early press, (3) naturally entered S3, (4) compare per-stage logging and only then decide whether faster settle is safe. Do not auto-hibernate/reboot or assume a software pass is a physical acceptance.


## Hardware trial phase A — deployed, NOT certified (27 October 2026)
- CT700 isolated Arch base-devel package built as 1.94.100.goodix51a0-71.70, SHA256 b38808c937356300d195c9eeaf948c52a3a5e83c5abb9e9352fb95684bd17b3f. Canonical private package: Pegasus/private/candidates/rel71.70-S4-rescue-A-20261027/.
- Pegasus pacman installed experimental 71.70; Snapper root pre/post snapshots **1944/1945**. Native fprintd restarted by pacman with new PID546427, active; 32/32 package files intact. Original enrolled right index remains registered, password fallback kept.
- Active service flags: GXFP_S3_FRESH_BG_QA=1, GXFP_FDT_DOWN_PROOF_QA=1, GXFP_SPI_CS_SETUP_QA=1; added QA GXFP_S4_MIDCAPTURE_REUSE_QA=1, GXFP_RESUME_PERF_QA=1, GXFP_CAPTURE_PERF_QA=1.
- Speed-setting GXFP_RESUME_FAST_SETTLE_QA remains **OFF**; do not conflate phase A S4 reliability with experimental phase B reduced 250/500ms delays.
- Portable CI of draft PR #33 **12/12 PASS** after second QA commit fd6dc0b. This is software qualification only.
- Real hardware LOCK, S4, S3 tests of rel71.70 **NOT DONE YET**; the human must touch the power button sensor and perform natural S4 (no agent-triggered sleep). On positive and negative outcomes inspect RESUME_PERF_QA and CAPTURE_PERF_QA journal lines and operator-attested physical poses before further speed tuning.
- Last-known-good rel71.69 rollback pinned at /var/lib/gxfp51a0-qa-rollback/rel71.69-final.pkg.tar.zst, SHA 38039bc3b745f2a2661c1c04ba3264f8792163d975f929679b6d7b3fd69a4ee4; also rel71.67 / rel71.64 retained. To revert: sudo pacman -U --noconfirm /var/lib/gxfp51a0-qa-rollback/rel71.69-final.pkg.tar.zst. Do not touch enrollment or PAM.
