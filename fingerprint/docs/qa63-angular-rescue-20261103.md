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


## 2026-11-03 — build and installation evidence (Pegasus)

- Candidate Git commit fc26910ab04ba5d11b9d1c0964437f8ecad39ba7; QA63 branch remains independent of main.
- CT700 software baseline 2026-11-03: SOFTWARE_BASELINE_EXIT=0 with FDT/TLS, capture, same-press single-image, S3/S4 simulations, legacy QA54 and QA57 rotation/safety suites, QA63 source contract, and synthetic FAST/BRIEF tests. QA63 source contract RED before change and GREEN after.
- CT700 native libfprint v1.94.100 116-target build complete EXIT0, Goodix object and ACPI ID, Identify, RANSAC, no release image dumping: PASS. No physical device touched by build.
- Pegasus native makepkg EXIT0, pkg libfprint-goodix51a0 1.94.100.goodix51a0.qa63-1, package SHA256 553c9fc2611d86f345eca1ad96f6ada501e9a4ea98371267a9ce7f1042f41b3c.
- Package installed on Pegasus, pacman QA57-1 -> QA63-1. Auto Snapper 2090 PRE, 2091 POST. Post-install health QA63_INSTALL_HEALTH_PASSED: 32/32 files 0 altered; fprintd 1.94.5-2.1 active PID199661 and maps /usr/lib/libfprint-2.so.2.0.0; right-index-finger enrollment preserved; original strictly-validated FDT and both HOF flags plus QA63 wide rotation present in actual systemd Environment. No reboot, re-enrollment, or new service.
- Reversible rollback SHA256-verified QA57-1 package in Pegasus /var/cache/pacman/pkg, SHA e0d5fde84daa42a89936076050781b455e483e7425b699ed9b5c0d3270aa7a16. QA63 package also cached and archived on Cloud9 at private/candidates/qa63-angular-rescue-20261103 with verified SHA256.
- Primary code branch pushed via authorized Pegasus gh to GitHub, no main merge and no HUAWEI installer changes.
- PENDING: actual human LOCK, preheld finger, unknown finger, deep S3, S4 and time-to-terminal. Do not assert speed or reliability improved yet; QA63 is a deliberately bounded A/B candidate, not a certified release. Preserve exact 7/7 threshold and wrong-finger security.
