# QA64 — optional GPIO48 kernel rising-edge observer (2026-11-03)

## Baseline and scope
- Parent: main QA63 SHA 309978d7c521faf016adab068042de84514880ff. QA63 is still GitHub main and Pegasus base/rollback.
- Reason: szlukabence's 2020 ST411 issue #6: 98/104 typed FDT 0x32 reports; 6/104 ACK-only; after those six ACKs the 1ms GPIO48 polling loop never sampled HIGH. Five misses followed post-NAV cleanup, one was precapture. A sub-ms IRQ is not excluded.
- Change: existing Linux GPIO v2 request for line 48 optionally adds RISING_EDGE, monotonic kernel timestamps, bounded event buffer (64); no second line owner. On opt-in request failure, automatically fall back to original level polling.
- Only upon ACK without data already rejected by strict 2/2 FDT proof: inspect buffered rising edges with up to 1000ms additional passive observation bounded by the S3 presleep deadline. Never issue SPI during watch; no acceptance after original deadline. Log count, first/last ACK-relative ms and sequence gaps only. Previous mandatory MCU/TLS reset always runs unchanged. Not yet a speed fix.
- Default source mode OFF unless GXFP_FDT_IRQ_EDGE_WATCH_QA=1; the QA64 experimental Arch/portable package opts in, the main QA63 package does not.
- Original 14 QA63 runtime flags, fixed matcher threshold7, TLS-GCM, fingerprint templates, resident fprintd and password fallback unchanged. No images, zone values, enrolled prints, keys or raw SPI logged.

## Evidence and outstanding gates
- Synthetic Linux event parser: sub-ms rising edge, 178ms late event, pre-ACK edge, falling/irrelevant event, sequence gaps, missing event and time bounds.
- Source safety contract: no SPI in observation, only strict failure branch, reset still mandatory.
- CT700 full source regression suite and native libfprint v1.94.100 compile (116/116) PASS; repeat after packaging edit.
- Success without a missing FDT report does not prove any improvement. Need ACK-only hardware evidence, physical first-placements and negative-finger / S3/S4 reliability for a production candidate.

## Reversibility
- QA64 Arch package: 1.94.100.goodix51a0.qa64-1, opt-in GPIO observer enabled solely for experiment. QA63 rollback package is preserved on Pegasus and Cloud9, SHA256 553c9fc2611d86f345eca1ad96f6ada501e9a4ea98371267a9ce7f1042f41b3c.
- Rollback via local pacman -U, no print re-enrollment, firmware flash, sleep hook or reboot.
