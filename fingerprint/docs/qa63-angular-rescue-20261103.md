# QA63 — bounded additional orientation coverage, experimental 2026-11-03

## Observed need
At 14:44–14:45 Pegasus original QA57 failed multiple pre-held genuine contacts. Scores remained 3/7 even after clean background reference. Final successful touch had raw3/7, but validated same-view +12 degree rotation led to score8/7. This indicates rotation sensitivity, but does not prove all preceding failures are rotation-related. Several strict FDT ACK-only failures were also present.

## One software variable
- Exact QA57 source branch fd89baa. The original rotation angles (-12,+12,-6,+6) always run first. If these all fail, explicit opt-in GXFP_WIDE_ROTATION_QA=1 tries -18 and +18 degrees, on the SAME decrypted frame and inside SIGFM 20-degree cap.
- Maintains primary baseline>=3 and <7, fixed threshold7, independent same enrolled view photometric overlap, ZNCC and agreement gates, no score fusion or additional authorized image per physical press. Identify untouched.
- Packaging reproduces original QA57 HOF runtime flags GXFP_DECOUPLE_ACK_PACING_QA=1 and GXFP_ROTATION_RESCUE_QA=1, plus new opt-in GXFP_WIDE_ROTATION_QA=1; there is no dependency on a separate manual /etc drop-in.
- No FDT, TLS, raw image, enrollment, S3/S4, PAM or cleanup code changes. No new service or periodic timer.

## Safety and certification
- Synthetic 80x64 features test: genuine self-score128, unrelated synthetic score0, +18 then -18 recovers70 (>7); out-of-bounds 21-degree rejects. Not a real-world biometric FAR test.
- Extra angles may cost CPU when no match and increase the set of accepted orientations. Wrong-finger hardware rejection and repeated first touch LOCK/S3/S4 required before promotion.
- This change does not resolve FDT typed response loss or force the background to refresh while a finger is already present.
- NO production readiness or faster-than-HOF claim without actual Pegasus evidence.
