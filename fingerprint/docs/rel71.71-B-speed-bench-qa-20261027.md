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
