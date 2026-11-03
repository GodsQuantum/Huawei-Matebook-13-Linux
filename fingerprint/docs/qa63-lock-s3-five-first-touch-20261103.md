# QA63 Pegasus — 3 LOCK + 2 genuine S3, all first physical pose, 2026-11-03

Hardware source: native Pegasus fprintd journal (ISO-precise timestamps) and user explicitly confirmed LOCK1 1 pose, LOCK2 1 pose, LOCK3 1 pose, S3-1 1 pose, S3-2 1 pose. Installed libfprint-goodix51a0 1.94.100.goodix51a0.qa63-1, stock fprintd 1.94.5-2.1 PID199661. Timings below are DETECTED_HOLD-to-verified-positive, not full display latency. No raw print or sensitive material stored.

| Trial | DETECTED_HOLD CET | Positive CET | Contact-to-positive | Score/7 | Secure cleanup |
|---|---|---|---|---|---|
| LOCK1 | 15:11:03.329488 | 15:11:04.506353 | 1176.865ms | 14 | 557ms ok1 |
| LOCK2 | 15:11:20.530517 | 15:11:21.689717 | 1159.200ms | 10 | 569ms ok1 |
| LOCK3 | 15:11:33.670268 | 15:11:34.915809 | 1245.541ms | 7 | 561ms ok1 |
| S3-1 | 15:16:05.473086 | 15:16:06.696146 | 1223.060ms | 7 | 572ms ok1 |
| S3-2 | 15:16:32.159235 | 15:16:33.370869 | 1211.634ms | 7 | 562ms ok1 |

Observations:
- LOCK1/LOCK3 correctly discarded a background capture after the finger arrived during refresh (stale clean original ImageBase retained), yet genuine scores14 and7 passed in first contact. LOCK2 was touched already and background update deferred; score10. This empirically shows contact can authenticate without waiting for a new finger-free background.
- True S3-1 sleep-boundary 15:11:56-15:12:03, native TLS/ImageBase prewarm 2862ms success. There was a long ~4-minute delay without a finger and repeated Verify Open/Close before one human contact at 15:16:05; score6/7 raw rescued by legacy +12-degree rotation to7. Do not treat resume-to-success ~4min as fingerprint hardware speed.
- True S3-2 sleep-boundary 15:16:23-15:16:31; native prewarm2626ms success and held finger already present during secure reinitialize. FIRST contact at15:16:32, legacy +12deg rotation raw3→7, positive15:16:33.370869, wake-to-success ~5597ms.
- All 5 successes used the unchanged baseline matcher and existing QA57 +12-degree rotation. QA63 NEW ±18-degree branch never engaged. There is no evidence yet that wider rotation improved first-touch recall; do not attribute this 5/5 to it.
- Five authenticated captures 572-582ms, score-computation 12-108ms, strict post-capture cleanup557-572ms, all successful. No mandatory FDT 0x32 ACK-only proof failure within these 5 biometric exchanges, though FDT manual retry on background capture remains observed.
- Historical fastest QA57 first-touch LOCK 1156.903ms. Fastest QA63 1159.200ms (+2.297ms), no new speed record.
- Current QA63: 3/3 LOCK 2/2 S3 first-pose; not sufficient for public production certification: standing policy demands 5 LOCK, 3 real deep S3 incl active Verify, 3 BOOT, 3 REBOOT, wrong-finger rejection, password fallback, and real S4. No cold boot, S4, unknown-finger data here. A synthetic unrelated image is NOT biometric FAR validation.
- QA57 and QA62 historical failed trials preserved. Main HUAWEI release remains QA59.2 pending release gates.
