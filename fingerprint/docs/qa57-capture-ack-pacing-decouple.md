# QA57 — protocol ACK / biometric capture pacing decoupling

Source baseline: QA54 on GitHub main, distinct from QA55 cancellation and QA56 200ms FDT report window. This is an opt-in experiment only.

## Hardware evidence

Pegasus 2026-11-01 QA56 runtime: control-plane misses inflated timing_scale from 100% to 300%; capture spacing floor reached 250% or 75ms. Image frame ~900–923ms versus ~573ms in the early QA56 first-touch LOCK. These results are correlated, not a controlled causal A/B comparison. Two S3 post-image cleanup 0x32 exchanges had only ACK (1/2) even with 200ms wait. Longer timeout did not help.

## Implementation

GXFP_DECOUPLE_ACK_PACING_QA=1 (default off) removes the protocol timing scale-derived minimum gap from both cold context initialization and successful image retry adaptation. Actual image transport retries, capture desynchronization, clean-streak decay and their own local capture_gap_scale remain unaltered. Control ACK timeout handling, strict FDT proof, frame acquisition, TLS, biometric threshold 7, existing rotation rescue, PAM, S3 and enrollment are unchanged. The canonical QA54 100ms FDT window is kept.

## Hardware gate

Build and local C harness and regression suite before installation. Validate each LOCK/S3 actual journal vs QA54 record 1157.858ms, QA56 1193.204ms, success rates, cleanup, wake reliability and end-to-end latency. A build pass is not equivalent to hardware validation. Preserve SHA256-verified QA54 package and enrollment; rollback immediately upon regression. Never accept FDT ACK alone or relax threshold.

## 2026-11-01 19:22 — package and initial Pegasus evidence

- Source commit `d127c88` plus FDT test-harness commit `073023f`, PR #62 https://github.com/GodsQuantum/Huawei-Matebook-13-Linux/pull/62 (DRAFT). Original main baseline QA54 code with earlier QA56 trial documentation only.
- CT700 complete `make -C fingerprint verify` PASSED after adapting the C-only FDT pacing harness to exclude the newly inserted GLib-dependent helper; independent actual C helper test 4/4 PASS and all existing regressions PASS. GitHub CI commit 073023f 12/12 PASS.
- Arch package built with makepkg on Pegasus from committed branch at 19:19:59 CET: `libfprint-goodix51a0 1.94.100.goodix51a0.qa57-1`, SHA256 `e0d5fde84daa42a89936076050781b455e483e7425b699ed9b5c0d3270aa7a16`, archived Cloud9 `private/candidates/pr57-capture-pacing-20261101/libfprint-goodix51a0-1.94.100.goodix51a0.qa57-1-x86_64.pkg.tar.zst`. Former QA54 rollback package SHA256 `d3fe51eb826b009e80612a88e48642c0ef3d47c80fe37a29ef15e3fa88e1b174` verified on Pegasus.
- Installed QA57 via pacman (Snapper automatic root snapshots 2036/2037); 32 total package files 0 altered. Old `70-gxfp-qa56-fdt-late.conf` removed, `70-gxfp-qa57-pacing-decouple.conf` set `GXFP_DECOUPLE_ACK_PACING_QA=1` and other QA54 flags including `GXFP_ROTATION_RESCUE_QA=1` preserved. `fprintd` active and right-index-finger enrolled, no re-enrollment and no reboot.
- No-touch smoke `timeout 6 fprintd-verify developer`: opened device, listed right-index-finger and started Verify, timed out as expected (exit124); journal: **FAST_READY=51ms** at 19:21:33.997942, VERIFY READY at 19:21:34.847795, CLOSE retained warm context at 19:21:39.955897. In the same session protocol control timing still rose 150→200% after ACK miss, as expected; QA57 only removes the coupling to capture gap, not ACK adaptation. Actual finger image timing NOT YET measured.
- Hardware gate still pending: physical LOCK then S3, first physical press counts from operator, journal FDT replies, capture frame gap/timings, first-to-terminal, ImageBase refresh, S3 park warm context, negative finger. No 1st-touch or S3 production claims. Rollback by removing only QA57 drop-in, pacman-U exact QA54 package, reload+restart fprintd; preserve enrollment.
