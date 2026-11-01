# QA49 — Fast initial FDT poll when a full finger is already held

Status: experimental, **OFF by default**. Base QA42 with KDE47 unchanged. No changes to enrollment, ImageBase 20s freshness, TLS-GCM, FDT typed replies, thresholds, PIXEL_RESCUE or score-conditioned recapture.

## Measured reference
- 2026-11-01 02:04 natural LOCK first physical pose succeeds, FDT already touch0x3f during FAST_READY.
- FAST_READY 246ms, VERIFY_READY 02:04:13.139218, DETECTED_HOLD 02:04:13.500521 (**361ms** READY-to-detection).
- Finger capture **574ms**, image+SIFT+match **702ms**, cleanup **566ms**; DETECTED_HOLD to positive **1269.654ms**, score primary4/7, already-existing PIXEL_RESCUE score7/7.
- Current GLib g_timeout_add(100) waits 100ms before first FDT poll, confirmed in official docs https://docs.gtk.org/glib/func.timeout_add.html. This is a possible ~99ms to save **before** detection, NOT 99ms from DETECTED_HOLD to positive.
- The existing first FDT probe may itself consume tens of milliseconds; 361ms is NOT wholly attributable to GLib's timer.

## Gated change
Opt-in environment GXFP_FIRST_POLL_HELD_QA=1. When a healthy warm Claim independently detects all six touch zones and a mean below prior finger-off floor, set a one-use hint. Only on first ordinary Verify, dispatch a **single** 1ms initial FDT probe; if it does not identify contact, hand off to original 100ms recurring timer. No fast handling on partial contact, next press, repeated polls, cold unproven path, cancel, or sleep invalidation. No spin loop, background daemon or periodic timer. Native sensory FDT proof and strict SIGFM + threshold7 still mandatory. Code review must check no re-entrant callback or forgotten GLib source.

## Physical A/B required
Test only after native build gates, install via Arch package preserving verified QA42 rollback SHA. On Pegasus, compare natural one-touch LOCK from held finger without moving mouse; compare FAST_READY, VERIFY_READY-to-DETECTED_HOLD, frame_ms, DETECTED_HOLD-to-positive, score, FDT transport failures and initial-touch success versus QA42 record and 02:04 trace. S3/S4 before permanent production promotion. If first-touch worsens, immediate rollback (QA42 + KDE47).
