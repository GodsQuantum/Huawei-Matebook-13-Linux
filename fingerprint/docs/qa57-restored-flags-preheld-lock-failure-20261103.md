# QA57 2026-11-03 14:44:39–14:45:08 — preheld first-touch LOCK regression

Source: live Pegasus journalctl -u fprintd -o short-precise; operator reports multiple failures before success. Active package qa57-1, stock fprintd 1.94.5-2.1, both HOF flags GXFP_DECOUPLE_ACK_PACING_QA=1 and GXFP_ROTATION_RESCUE_QA=1, PID189748.

- 14:44:39.130256 FAST_READY 246ms, preheld finger already present (touch0x3f, mean237). ImageBase age311229ms >20s. WARM_REBASE deferred14:44:39.232879 with live finger, retaining old clean reference.
- DETECTED_HOLD14:44:39.601925, score3/7 at14:44:40.285590; capture576ms, match101ms, cleanup566ms ok1.
- Finger lifted; CLEAN background refreshed at14:44:43.568488 in849ms mean354. Next held14:44:44.048921 scored3/7 despite the fresh ImageBase (capture600ms cleanup560ms). Next14:44:47.569602 scored3/7. So old reference alone cannot explain repeated failures.
- After: typed FDT response missing with ACK only at14:44:51.226594, strict recovery and no image; 14:44:53.528574 scored3/7, 14:44:56.594703 scored3/7 and FDT cleanup failed, 14:45:02.058059 scored4/7 with 1000ms GET_IMAGE missing ACK/TLS and cleanup failed.
- Last detection14:45:06.739719: raw3/7, independent same-frame strict +12-degree rotation score8/7 with corroborating pixel metrics, effective threshold7 at14:45:07.406334. FDT postauth cleanup typed response missing14:45:08.007124, authentic biometric score preserved for positive terminal14:45:08.007519.
- Last contact→success 1267.800ms; first detected→success 28.406sec. EIGHT DETECTED_HOLD events, human counts reported only multiple. First-touch FAIL. This is not an HOF positive sample. First contact did NOT crash the sensor; initially it prevented background refresh and relied on clean-but-old ImageBase. Persistent failure after fresh reference suggests placement/orientation or template coverage, NOT proven, and intermittent FDT typed reply loss further destabilizes.
- This outcome DISQUALIFIES QA57 as a fully reliable production candidate even after restoring its original HOF runtime flags. No unapproved reenroll, security weakening, raw image dump, or new QA version installed.
