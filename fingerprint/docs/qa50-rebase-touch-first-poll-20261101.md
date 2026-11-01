# QA50 — Touch at warm ImageBase refresh

QA49 real LOCK 03:10: FAST_READY touch=0x00; 1,704,255ms-old ImageBase refresh rejected due to a newly arrived finger at 03:10:00.100770. VERIFY READY 03:10:00.106255; DETECTED_HOLD 362.673ms later. QA49 fast initial probe did not trigger.

QA50 transfers a single-use early-probe hint from either WARM_REBASE touch-deferral or FDT-confirmed finger during background capture, only when six touch zones and mean below the finger-off floor both pass. The finger-contaminated background stays rejected. QA49 one-shot probe at 1ms and 100ms fallback remain. Matcher threshold 7, TLS, FDT, KDE and capture recipe unchanged.

Compare QA42 02:04 READY-to-DETECT361.303ms and DETECT-to-success1269.654ms, QA49 03:10 READY-to-DETECT362.673ms and DETECT-to-success1327.775ms. New QA50 must pass a real LOCK first touch and hardware timing comparison; keep QA49/QA42 rollback. No improvement claimed before measurement.
