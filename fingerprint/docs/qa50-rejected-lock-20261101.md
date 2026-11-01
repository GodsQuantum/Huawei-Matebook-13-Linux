# QA50 — hardware LOCK FAIL, 2026-11-01 03:57 CET

User reported multiple physical finger placements without unlock. Actual Pegasus fprintd logs confirm ≥5 finger detections and no positive biometric match in the observed sequence. QA50 software gates had passed.

FAST_READY 52ms, touch0x00. ImageBase age1048568ms, 20s limit. QA50_REBASE_TOUCH_ARMED stage=after 03:57:03.897449, after safely discarding a finger-contaminated background. VERIFY READY 03:57:03.903084; FIRST_HELD_POLL_QA 1ms first probe invoked; DETECTED_HOLD 03:57:04.139771 = **236.687ms**. FDT touch0x3f 6 zones and valid 0x32 typed replies.

First usable image frame575ms, SIFT5ms, matcher151ms, score **3/7**, cleanup565ms. Second pose image574ms score3/7 cleanup566ms. Third pose image576ms score4/7 cleanup566ms, after a fresh background finally became available. Later Verify transaction experienced an incomplete second typed FDT response and executed safe bounded transport reset/TLS recovery. Further scores4/7; no success verified. At least five complete finger detections observed; exact physical pose count unknown.

Compared with QA49 successful first-touch LOCK at 03:10: READY→DETECT362.673ms, capture591ms, DETECT→positive1327.775ms, score primary4/7 rescued7/7. QA50 improves READY→DETECT **125.986ms** but fails first-touch reliability. Different finger positions, ImageBase ages and controller pacing mean no proof the code change caused the low scores; QA50 is nonetheless rejected as unsuitable for production.

**Rollback actually performed:** original QA49-1 Arch package SHA256 85abc09b57d5eee781320ca7a598df388d6bb73cdeb27a575863f0a1e8b0fb9a installed on Pegasus, package 32/32 intact, .so SHA256 2f195a4d10d9959432ada0faecf99a8dcb644f62a6f03d5716518de50f4fc75f, fprintd active and TLS success, no-mouse KDE patch PASS, right-index enrolled. Snapper pre2026/post2027. PR #50 closed unmerged. No biometric images or templates in this document.
