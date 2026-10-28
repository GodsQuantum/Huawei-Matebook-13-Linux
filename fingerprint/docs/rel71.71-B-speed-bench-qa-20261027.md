# GXFP51A0 QA 71.71-B — lower latency, not yet certified

## Observed 71.70-A hardware baseline, 27 October 2026

- LOCK operator-attested **first pose**, subjectively slow. At 20:34:31 FAST_READY 53ms, image from DETECTED_HOLD 20:34:32.356 to scorer 20:34:33.083: frame577ms, SIFT5ms, match143ms. Worker image scored7/7. Cleanup completed at 20:34:33.644 (about560ms after initial match), final positive 20:34:33.757. Finger detection to terminal approx1.400s.
- Real S4 operator-attested **second pose**; not a first-touch success. Native resume 20:53:04.622, prewarm 5.493s = reset/settle2.160s + cold TLS/background3.332s. Crucial strict S4_MIDCAPTURE_SAVED_REFERENCE observed; new S4 branch activated. FAST_READY83ms, finger DETECTED_HOLD20:53:12.448, first usable image scored7/7 at20:53:13.468 (frame893ms, SIFT3ms, match123ms). Cleanup got no GET_IMAGE ACK/TLS in its first 1000ms window, retried and succeeded20:53:15.323, terminal20:53:15.500. Cleanup itself cost **1.855s**, explaining why apparent unlock was slow. There was only one scored physical press in the software log: the user's earlier physical placement most likely happened during prewarm. Do not claim it matched at first pose.

## 71.71-B candidate enhancements (source-only until real QA)

1. **Remove duplicate 1:1 and 1:N score computations.** The worker already computes the final first-quality-image match against held template/gallery. Pass that integer score, candidate index, and 1:1 view counts across the completed GTask; verify that the Identify gallery remains in the original order. No extra score or quality gates added or relaxed, no replay, no change to threshold7 or TLS-GCM.
2. **Keep mandatory cleanup.** It drains the encrypted image/NAV/FDT sequence to maintain protocol and TLS counter alignment. Add metadata-only CAPTURE_PERF_QA cleanup_ms and per-step command/timing logs; never emit raw encrypted or biometric material. Do not bypass GET_IMAGE after a timeout, because an accepted command could deliver a stale TLS record.
3. **Independent experimental faster S4/S3 prewarm:** earlier rel71.70 code has GXFP_RESUME_FAST_SETTLE_QA (250/500ms vs historic1000/1000ms around same native reset): an estimated 1.25s less passive waiting. Needs real S4/S3 test; this setting is *not* proof of reliable power sequencing.
4. Preserve rollback packages on Pegasus, original encrypted matching, GPIO timing, enrollment and native password fallback. The source branch and GitHub PR#33 remain DRAFT; build artifacts are QA packages only, not a stable rel71.71 release.

## Hardware certification required

Physical LOCK first-touch, genuine S4 with early and normal finger contact, genuine deep S3, and repeated trials. Record ms stage timings and actual operator pose counts separately. Only ship as stable after sufficient repeatability. No automatic reboot/suspend.


## Actual 71.71-B hardware — 27 October 2026 22:35 and 22:38 CET

User: LOCK 1 physical pose, natural S3 2 physical poses. S4 not tested: user cannot interrupt ongoing work; never auto-hibernate or reboot.

- LOCK 22:35:29: FAST_READY52ms; score 15/7 on first image. User-attested first pose success. Detection22:35:30.395620, first frame571ms, SIFT3ms, match119ms, mandatory cleanup559ms, positive22:35:31.649984. Finger→positive1.254s. Previous rel71.70-A 1.400s, a single observed ~146ms improvement.
- ACPI-kernel confirmed true deep S3 (not s2idle/S4). Clean RAM snapshot and acknowledged native park at22:38:15. Resume22:38:19.843832; SHORT RESET with fast_settle=1: reset/settle910ms, cold TLS/ImageBase1493ms, automatic PREWARM_READY22:38:22.248, total2403ms. Early finger-like sensor contact FDT6/6, mean221 detected by native snapshot reuse at22:38:22.247 (before any new biometric Claim). Source gx_dev_resume/gx_resume_preflight_thread already primes transport, TLS and background automatically before fingerprint Claim: NO new service needed.
- KDE/fingerprint greeter Claim only later: FAST_READY72ms at22:38:25.213, VERIFY_READY22:38:25.248, first pose DETECTED_HOLD22:38:25.620. First encrypted image rejected at22:38:26.392 by strict contrast quality gate, BEFORE scoring. Finger was not detected when RetryCaptureIMG probed at22:38:26.700; mandatory cleanup744ms, rearmed. Second physical pose DETECTED_HOLD22:38:27.729, capture772ms, SIFT5ms, score109ms = 11/7, required cleanup675ms, positive22:38:29.292. Second pose detection→success1.563s. True wake→success9.448s includes greeter delay and first rejected image.
- Different-mode warning: rel71.70 previous 5.493s prewarm trial was S4, not directly comparable with current S3 2.403s. Earlier rel71.69 actual S3 prewarm was5.153s, vs2.403s here (~53% reduction across individual trials, not a stability guarantee). S4 rel71.71 remains untested.
- KDE logged Worker not ready yet while native prewarm was running at22:38:20, then requested fingerprint about3s after sensor prewarm finished. This could be separate UI/PAM setup delay; no evidence a second custom prewarm service would improve it.
- IMPORTANT: First failed S3 press was a contrast/hold or finger-off issue, not score below threshold7, therefore lowering the matcher threshold is unjustified. Do not accept rejected images, weaken TLS, or cache fingerprint authentication from before PAM Verify. Better future study: acquire and align FDT state/quality timing without modifying pass gates.

## S4 first-touch at 23:02 — 27 October 2026

Operator attests one physical pose, a little slow. Genuine ACPI S4 hibernate resumed at 23:02:20. Native prewarm starts23:02:21.003, fast GPIO reset23:02:21.258, then unexpectedly another **full GPIO264 300/600 reset** logged23:02:21.957 (source contains both PMK staging and TLS retry paths; the trigger remains unknown). Reset/settle910ms, cold TLS/background3915ms, total native prewarm **4826ms** (READY23:02:25.835). FDT initial ACK retry before ready. Native warm FDT FAST_READY83ms at23:02:27.339 with early sensor contact touch0x3f. Verified finger DETECTED23:02:27.562905. Finger image frame894ms + SIFT3ms + matcher93ms -> score8/7; four mandatory cleanup steps755ms (no timeout), terminal positive23:02:29.310581. Finger→positive1748ms and wake→positive8308ms, real first human press. Observations are single-trial, not reliable multi-S4 certification. Avoid unsafe reduction of SPI delays, PMK or encrypted cleanup.
