# QA56 — FDT 0x32 late-report rescue (2026-11-01)

## Evidence and hypothesis

On Pegasus QA55, during ordinary LOCK 15:57 and S3 15:58, a number of FDT-down `0x32` exchanges reported `replies=1 ack=1 valid_data=0`; they led to strict fail-closed MCU/TLS reconstruction and `warm=0` at the next native S3 park. A matching pass was sometimes lost to post-capture cleanup, despite the image being good. A 2020 MateBook 13 user independently measured 70/72 valid reports, normally 60–88 ms after ACK, with 2 missing by ~120ms. Source: https://github.com/GodsQuantum/Huawei-Matebook-13-Linux/issues/6#issuecomment-6504393291

This evidence does **not** establish whether missing reports never arrive or arrive later than 100ms. Longer waits are justified **only as a bounded A/B experiment**, not as a permanent default or a reason to skip strict proof.

## QA56 change

- Based on QA54 canonical GitHub main; QA55 cancel changes **not** included.
- Environment `GXFP_FDT_DOWN_LATE_REPORT_QA=1` (OFF by default). This flag is effective only together with `GXFP_FDT_DOWN_TYPED_REPLY_QA=1`.
- For command `0x32` **after an ACK**, wait at most **200ms instead of 100ms** for the *distinct* checksum-valid FDT/touch report tagged `0x32` or `0x36`. With the usual 61ms response, no added delay. Missing, malformed, unrelated, duplicated ACK-only and over-200ms responses remain fail-closed, same MCU/TLS recovery. No replay of `0x32`, no acceptance based on ACK alone.
- Log sanitized `FDT_TYPED_REPLY_QA ... window_ms=200` and `FDT_LATE_REPORT_QA accepted=1 ack_to_data_ms=N` only if >100ms. No raw touch data or biometric material.
- No changes to capture gaps, TLS, native resume, enrollment/Identify, FDT threshold, matcher threshold7, QA54 rotation recovery, or normal default 100ms behavior.

## Validation

- Real C function extracted and executed in deterministic IRQ harness: 15 tests PASS. Late 180ms report fails at 100ms, succeeds with both flags; 280ms or absent report still fails; invalid/duplicate reports rejected; normal 0x32/0x36 data unchanged.
- CT700 `make verify` including native libfprint1.94.100 required before hardware trial.
- Hardware gate: QA54 checksum-identical package retained. After QA56 install, evaluate true LOCK/S3 with user physical press counts and journal timestamps: all `0x32` late reports, missing rate, `FAST_READY`, `DETECTED_HOLD→terminal`, baseline freshness, scoring, TLS resets. Reject/revert if more delays, false accepts or new failures. No forced hibernation or automatic enrollment.

## QA56 package and Pegasus no-finger smoke (2026-11-01 16:20 Europe/Paris)

- CT700 full `make -C fingerprint verify` PASSED with libfprint native v1.94.100 compile and regression tests.
- GitHub PR #59: https://github.com/GodsQuantum/Huawei-Matebook-13-Linux/pull/59 (DRAFT); CI **12/12 PASS** for source commit `08e6be9`. Do not merge until measurable physical FDT benefit, genuine first touch, wrong finger and sleep reliability.
- Arch package compiled with `makepkg` on Pegasus using identical committed source, `1.94.100.goodix51a0.qa56-1`, built 2026-11-01 16:19. SHA256 **8acdefbc58ed216f6ad6259622f495d7e0856b6c4e0f5a36feb25127e5030dfd**, archived Cloud9 `private/candidates/pr56-fdt-late-report-20261101/libfprint-goodix51a0-1.94.100.goodix51a0.qa56-1-x86_64.pkg.tar.zst`.
- Installed on Pegasus via pacman; 32 files / 0 altered; `fprintd` active; enrolled right-index-finger preserved; active overrides include original QA54 `GXFP_ROTATION_RESCUE_QA=1` and QA56 `GXFP_FDT_DOWN_LATE_REPORT_QA=1` in `/etc/systemd/system/fprintd.service.d/70-gxfp-qa56-fdt-late.conf`.
- QA54 exact fallback still verified on Pegasus at `/tmp/gxfp-qa55-20261101/libfprint-goodix51a0-1.94.100.goodix51a0.qa54-1-x86_64.pkg.tar.zst`, SHA256 `d3fe51eb826b009e80612a88e48642c0ef3d47c80fe37a29ef15e3fa88e1b174` (also Cloud9 archived).
- A **no-touch** fprintd-verify was opened and interrupted with SIGINT; `FAST_READY 52ms` (16:20:41.014154), `VERIFY_TRACE READY` at 16:20:41.076992, `CLOSE_TRACE retained complete warm context` at 16:20:46.998133. No biometric match/image capture happened. This proves baseline Claim and cancellation availability but **NOT** FDT-late rescue benefit.
- Rollback QA54: disable/remove only 70-gxfp-qa56-fdt-late.conf; pacman -U the verified QA54 package; daemon-reload, restart fprintd and verify enrollment. No reboot or re-enrollment required.
- Pending hardware: one genuine LOCK physical trial under QA56, then S3. On each report immediately extract exact journal with DETECTED_HOLD→positive terminal, score/rotation, FDT_TYPED_REPLY_QA times and late rescue counts. Revert if extra waiting/reset, no-match or wrong-finger accept. Do not declare QA56 Hall of Fame champion on synthetic/no-touch test.
