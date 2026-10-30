# 2026-10-30 Pegasus GXFP51A0 QA42 — first-touch LOCK observation

Hardware: reference Huawei MateBook 13 2021 Pegasus, Goodix GXFP51A0 / ST411. Package `1.94.100.goodix51a0.qa42-1` (PR42 commit `b98b437`, built from rel71.74 `bacbcc3` + PR41 `dcdb043`), SHA256 package `be085fb58c33de003f49992912189ac0476be864e618d4a5a9848b59d408d3ae`. Systemd opt-in `GXFP_FDT_DOWN_TYPED_REPLY_QA=1`, original FDT 100ms budget and mandatory cleanup, TLS-GCM, matcher threshold7. Single human-reported **LOCK 1 physical placement**, perceived not necessarily fast; no manual reboot/suspend triggered. Journal: `journalctl -b -u fprintd.service --since '2026-10-30 01:44:20' --until '2026-10-30 01:44:36' -o short-precise` (private; only sanitized metadata reproduced here).

## Validated timeline (Europe/Paris, Oct 30)

| Time | Journal event |
| --- | --- |
| 01:44:27.010600 | FAST_READY 52ms (TLS/background retained) |
| 01:44:27.033272 | ImageBase 249214ms old, eager refresh begins |
| 01:44:27.747019 | FDT probe ACK miss; protocol timing 150%→200% (distinct from capture pacing) |
| 01:44:27.840558 | WARM_REBASE discarded because finger already present during background capture; retained reference used |
| 01:44:27.846118 | VERIFY READY |
| 01:44:28.208327 | Physical press 1/3 DETECTED_HOLD |
| 01:44:28.353490 | FDT reply: valid ACK + 20-byte data tag **0x32**, delta61ms |
| 01:44:28.549624 | Second FDT reply pair valid, tag0x32, delta61ms |
| 01:44:28.840132 | Frame572ms, SIFT1ms, matcher57ms, combined631ms; first image score13/7 |
| 01:44:29.358620 | Cleanup final FDT response valid tag0x32, delta61ms |
| 01:44:29.388751 | Cleanup548ms successful |
| 01:44:29.388879 | Final positive terminal score13/7, no release wait |

**Comparability:** DETECTED_HOLD→positive = **1.180552s**, rounded **1181ms** for Hall of Fame. Older best rel71.71 had 1254ms measured with the same software markers: **73ms faster** for this single sample (~5.8%). FAST_READY→positive = 2.378279s: includes **~835.5ms** FAST_READY→VERIFY READY (old ImageBase refresh attempted), then 362.2ms READY→DETECTED, then 1180.6ms DETECTED→positive. In particular finger had *already landed* by 01:44:27.840558, so physical skin contact→success may exceed 1.548s. The Hall metric is *software detection→terminal*, not literal finger touchdown or total KDE unlock latency. Subjective slowness is consistent with pre-detection work and mandatory cleanup.

**FDT assessment:** 3/3 actual 0x32 drains show ACK=1, valid_data=1, tag0x32, ACK→data61ms; no FDT_DOWN_PROOF or MCU desync observed in this LOCK, cleanup ok. This is an important improvement relative to PR40, which captured no frames on Pegasus due to hard-coded report type0x36. There are still fewer than 5 independent first-touch LOCK observations on QA42; not production-certified. Previous rel71.74 operator-reported LOCK 1 pose is not present with a timed detection→positive record in the canonical Hall CSV, so no numeric head-to-head rel71.74 comparison is possible. Preserve rollback rel71.74, and keep PR42 DRAFT pending repeated LOCK, true S3, S4, cold boot and wrong-finger tests.

**Optimization target without unsafe changes:** instrument/evaluate opportunistic ImageBase refresh when finger is already landing, especially time FAST_READY→VERIFY_READY, plus FDT probe ACK retries and full cleanup. Avoid disabling cleanup or relaxing TLS/matcher threshold7 merely for speed.
