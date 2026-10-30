# GXFP51A0 QA44 — failed real LOCK, 30 October 2026

Human result: LOCK failed after multiple physical placements; exact count unknown. This is a failed hardware trial, not a first-touch pass nor a timing record. The software never recorded DETECTED_HOLD, therefore no score or finger-to-authentication latency can be calculated.

Reference MateBook 13 / Goodix GXFP51A0 ST411. Installed QA44 package version 1.94.100.goodix51a0.qa44-1; metadata-only stage tracing flag GXFP_WARM_REBASE_STAGE_QA=1 and typed FDT flag GXFP_FDT_DOWN_TYPED_REPLY_QA=1. No failed QA43 fast-first flag. PR44 commit e0882d4.

## Journal (Europe/Paris)
- 09:44:47.498599 FAST_READY 51ms, FDT no-contact touch0x00, mean351 floor327.
- 09:44:47.509928 ImageBase age187940ms, stale refresh requested before Verify.
- 09:44:48.215737 FDT probe ACK retry1/3, timing scale150% to200%.
- 09:44:48.308111 WARM_REBASE accepted fresh background+FDT after 798ms, mean353 floor329.
- 09:44:48.313426 VERIFY physical press1/3 READY.
- 09:44:53.323016 CLOSE retained complete warm context, 5.010s after READY, without DETECTED_HOLD, image, matcher score or success.

Interpretation: No finger was detected by the software before the client ended the verification at 5s. This does not prove precisely when or how many times the finger physically landed. Driver polling limit is 300x100ms (about30s), so the 5s close is likely an upper-layer Verify cancellation or timeout; exact source not proven. The optional metadata-only WARM_REBASE_STAGE_QA info-level message did not appear in journal despite flag enabled; preexisting warm-rebase warning was present, establishing the 798ms total. No sub-stage measurements are claimed.

## Rollback (confirmed)
QA44 downgraded to QA42 version 1.94.100.goodix51a0.qa42-1, rollback archive SHA256 be085fb58c33de003f49992912189ac0476be864e618d4a5a9848b59d408d3ae. Installed library SHA256 8adcaa2d5c18b55045967d0574ca6920fb28687cff96b58d2494c9f041d33131; QA44 drop-in removed; typed32 flag retained. fprintd active, pacman files 32/32, device and right-index enrollment intact. Snapper #2008/#2009. No reboot, S3, S4, biometrics policy or PAM modifications.

QA44 NOT hardware certified; PR44 stays draft. Its source changes only stage logging and tests. A one-trial failure cannot establish causality. A further human QA42 LOCK can isolate whether missed finger detection also occurs without QA44.
