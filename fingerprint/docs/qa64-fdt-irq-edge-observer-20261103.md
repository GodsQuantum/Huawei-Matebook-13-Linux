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

## 2026-11-03 ~21:29 CET — QA64 field verdict: S3 regression, ROLLBACK to QA63 COMPLETED

- Human feedback: first genuine S3 needed **multiple physical finger placements**; several LOCK tests **one physical placement each** (exact LOCK count not provided); second genuine S3 **two placements**. Compare preceding QA63 successful 2/2 real S3 first-physical-placement, 3/3 LOCK first placement. **QA64 fails S3 first-placement acceptance** despite stable LOCK; not better than QA63 and should NOT be merged into main.
- Kernel evidence actual S3 deep: entry 21:06:24 → exit 21:27:26; entry 21:28:17 → exit 21:28:36. `fprintd` resume 21:27:26 cold TLS/background 6476ms, prewarm total 7386ms, ready 21:27:34, Claim FAST_READY 89ms at21:27:36, successful verified positive score12/7 at21:27:40 (roughly 14s after kernel wake). Second resume21:28:36 cold TLS/background 7613ms, total8524ms, ready21:28:45, FAST_READY85ms, positive score14/7 at21:28:47 (about 11s post wake). QA63 Hall-of-Fame S3 prewarm ~2626ms, wake-to-positive ~5597ms. Comparisons are sample-bound and not definitive causal proof of event-mode slowdown.
- QA64 success images were **image1/3** after successful detected hold with cleanup ok1; this counts only valid software detections after readiness, NOT the user's earlier physical placements. During second S3 FDT live touch=0x3f by prewarm completion. Existing FDT-probe retry stage=ack once in each resume; no strict `FDT_DOWN_PROOF incomplete` nor `FDT_IRQ_EDGE_WATCH_QA` output for these trials, so the observer never had a missed 0x32 frame to analyze. No improvement mechanism exercised; cause of slow prewarm not proven.
- QA64 package was reverted (Pacman normal downgrade) to exact QA63-1 from offline verified local cache SHA256 `553c9fc2611d86f345eca1ad96f6ada501e9a4ea98371267a9ce7f1042f41b3c`. Snapper root 2096 PRE / 2097 POST. Pegasus post-rollback QA63-1 package 32/32 files 0 modified, native libfprint loaded ELF SHA256 `e482850a6f13aa03700303c3a52210e46f2550e1fc39d62c5c4a0c3bd1df3f4d`, fprintd1.94.5-2.1 active PID248737, QA64-only edge-watch flag absent; `gpioinfo -c gpiochip0 48` shows input (no rising-edges), fprintd-list retains right-index-finger. No reboot, no re-enrollment or custom services.
- Keep QA64 as historical **not-promoted experimental branch** for offline examination of GPIO v2 impact. QA63 remains local/GitHub main HEAD `309978d7c521faf016adab068042de84514880ff` and currently installed on Pegasus. No further physical tests required to decide rollback. No active background jobs.
