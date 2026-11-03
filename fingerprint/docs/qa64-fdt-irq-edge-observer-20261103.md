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

## Actual Pegasus QA64 package / hardware smoke (2026-11-03, ~20:48-20:54 CET)
- Experimental branch commit: 947bd31e44786b71e1e6e3b3ab296a1550869dcf, GitHub remote branch published; GitHub/main QA63 SHA 309978d unchanged.
- Native Arch makepkg on Pegasus EXIT0 with 116/116 libfprint targets and no biosensor IO during build. Package SHA256 efd3cdd6dfde94bd37f4c5048e7667f725c9e2653259575a20703aad67e20cba; installed native libfprint SO SHA256 1dfbcfe2b9ed64411a31a9cecb834946bb18f3cccc650fcf15f8d97097d4f3fc, exact package extraction match.
- First pacman -U directly from Cloud9 NFS was refused to root due to NFS root-squash; this did NOT install QA64. The fail-safe command reinstalled the unchanged SHA-verified QA63 package (Snapper root 2092/2093). Corrected by copying QA64 package to Pegasus local /tmp, SHA256 checked both as normal user and sudo/root, and installed with local pacman -U, Snapper root 2094 PRE / 2095 POST. 32/32 package files intact, fprintd stock 1.94.5-2.1 active, enrolled right-index-finger present, no enrollment changes, password preserved.
- Verified on live GPIO cdev during a bounded 8s fprintd-verify smoke: gpioinfo reports gpiochip0 line48 input edges=rising consumer=goodix51a0-irq. The smoke was intentionally cancelled without a physical finger (timeout rc124, NOT a fingerprint authentication trial); fprintd remained active and ReleaseDevice errors were absent in subsequent logs.
- QA64 package saved in Pegasus /var/cache/pacman/pkg with matching SHA256 for offline reinstall; QA63-1 stays alongside it for rollback. NFS canonical candidate archive retained. Build and smoke temporaries cleaned.
- Physical LOCK/S3/S4 first-touch performance and whether an actual ACK-only event produces a rising IRQ remain UNVERIFIED. Never claim the observer repaired an FDT miss before an actual hardware FDT failure and post-watch log.
