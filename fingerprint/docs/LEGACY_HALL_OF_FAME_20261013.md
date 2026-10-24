# GXFP51A0 — Historical Hall of Fame restored (13 Oct 2026)

Redacted extract from the pre-existing private Pegasus ledger, not 31 independent first-touch trials.

Source SHA256: 4545cf38f7d9a33c25d6d7226e5a4ba56bfaa75f6d3f2aef369cd96d7368ff5e. Original 31 configs, 125 Git refs, 14 historical leaderboards. Full unredacted original remains in LXC700 Pegasus project.

| Version | LOCK | Deep S3 | Cold BOOT | Scores documented | Latency documented |
|---|---|---|---|---|---|
| rel20 | reported_not_independently_verified | unknown | unknown | [] | — |
| rel23 | mixed | fail | unknown | [] | warm Claim 100–120ms after >65s |
| rel24-rc1 | pass_repeated | fail | unknown | [] | 2020 stable 626ms mean capture |
| rel25 | mixed | fail | unknown | [] | — |
| rel48 | unknown | pass_once | pass_once | [["S3", 11], ["cold biometric", 23]] | — |
| rel59 | unknown | mixed | unknown | [["S3 after retries", 8]] | 7.8s to READY once |
| rel61 | unknown | mixed | unknown | [["deep S3", 20]] | 21.5s from resume to READY |
| rel62 | unknown | fail | unknown | [["bad post S3", 3]] | 17.37s resume to READY |
| rel63 | unknown | unknown | fail | [] | cold prepare 4.3s, PAM→READY 1.8s |
| rel66 | unknown | mixed | unknown | [["true S3", 7]] | 4.434s READY after wake in one conditional run |
| rel71 LLVM22 7.2.8-1 | unknown | pass_once | unknown | [["deep S3", 15]] | 7.77s wake→READY; ~9s wake→match |
| rel71 GCC 7.2.8-1 | unknown | pass_once | unknown | [["deep S3", 11]] | 17.44s wake→READY |
| rel71 LLVM23 7.2.8-2 | unknown | fail | unknown | [["post S3 weak", 3], ["post S3 weak", 4]] | — |
| rel71.18 | pass_repeated | mixed | unknown | [["LOCK", 8], ["LOCK", 11], ["LOCK", 12]] | FAST_READY 82–83ms |
| rel71.24 | pass_repeated | mixed | reported_not_independently_verified | [["LOCK genuine", 9], ["other enrolled finger", 8]] | FAST_READY 83ms |
| rel71.30 | pass_repeated | not_tested | not_tested | [["Pegasus verification 8/8", [7, 10, 7, 13, 9, 11, 11, 19]]] | FAST_READY 82–84ms; detect→decision ~0.8–1.2s 2020 |
| rel71.31 | unknown | fail | unknown | [] | — |
| rel71.33-34 | unknown | fail | unknown | [] | — |
| rel71.35 | pass_once | fail | unknown | [["normal", 7]] | — |
| rel71.36 | pass_once | mixed | fail | [["LOCK", 14], ["true S3", 26]] | ~25s wake→READY on successful S3 |
| rel71.37 | unknown | unknown | fail | [] | — |
| rel71.38 | pass_once | fail | unknown | [] | 3/3 startup 7.4/14.4/15.5s; Claims ~106ms |
| rel71.39 | unknown | fail | unknown | [] | — |
| rel71.40 | pass_once | fail | unknown | [["LOCK", 11]] | — |
| rel71.41 | pass_once | fail | unknown | [["LOCK", 8]] | — |
| rel71.43 | fail | unknown | unknown | [] | — |
| rel71.44 | pass_once | mixed | fail | [["LOCK first pose", 13], ["true S3 first SCORED pose", 8]] | 83ms FAST_READY; S3 +10s READY +11s match |
| rel71.45 | pass_once | fail | not_tested | [["LOCK first pose", 11]] | 82ms warm, 1.2s ImageBase rebase |
| rel71.46 | pass_once | fail | pass_once | [["BOOT 2026-10-13 16:02:06 first pose", 18], ["LOCK 2026-10-13 16:16:34 first pose", 7]] | boot fprintd cold prepare 16:01:36 to 16:01:57 (~21s, 2 transient TLS failures); warm FAST_READY 83ms at 16:02:01; first finger detected 16:02:05; accepted 16:02:06; PAM session 16:02:07; normal KDE LOCK 16:16:31 FAST_READY 83ms, WARM_REBASE 1222ms, READY 16:16:32, first pose 16:16:33, score7/7 ACCEPT at 16:16:34 |
| rel71.47 | pass_once | fail | fail | [["BOOT 18:01:04 press1 partial 3zones", 3], ["BOOT 18:01:10 press2 full 6zones", 5], ["BOOT 18:01:16 press3 6zones", 3], ["BOOT retry 18:01:27 press1 2zones", 3], ["BOOT retry 18:01:31 press2 4zones", 3], ["BOOT retry 18:01:35 press3 6zones", 4], ["LOCK 2026-10-13 18:17:33 first pose full 6zones", 13]] | — |
| rel71.48 | not_tested | not_tested | not_tested | [] | — |

## Historical top-performers and negative findings

- **best original cold and deep evidence**: rel48, tiny sample, cold score23 + S3 score11
- **latest current kernel first boot match**: rel71.46 first BOOT placement 18/7 2026-10-13 16:02:06, warm READY 83ms but cold start ~21s; repeatability not established
- **latest current kernel first normal lock**: rel71.46 first LOCK placement 7/7 2026-10-13 16:16:34; FAST_READY 83ms and WARM_REBASE 1222ms, repeatability not established
- **latest current kernel real s3 failure**: rel71.46 deep S3 16:28:37-16:28:59 FAIL; park ACK=1, postwake FDT touch0x3f contaminated calibration (no scored image) and KDE fingerprint unavailable after rearm; not robust
- **latest current kernel cold boot failure**: rel71.47 after true reboot 18:00:39; prewarm ready +15s, FAST_READY 83ms; six human presses two cycles score3,5,3,3,3,4 threshold7; password fallback success; kernel unchanged 7.2.9-2, CachyOS update unrelated image libraries. BOOT=fail.
- **latest current kernel normal lock success**: rel71.47 18:17:33 KDE LOCK FIRST pose 13/7, FDT touch0x3f 6 zones, FAST_READY82ms and clean WARM_REBASE1218ms. Later true deep S3 rel71.47 FAILED.
- **latest current kernel s3 rel7147 failure**: rel71.47 real deep S3 18:51:19-18:51:23 FAIL. 0x60 park ACK, postwake FDT0x3f, 8s first calibration timeout, detached full reset, two TLS digest failures and FDT0x3f persistent. No scored image. KDE rearm unavailable.
- **best single current kernel real s3**: rel71.44, score8 on 7.2.9-2, NOT robust
- **best single kernel experiment**: rel71 LLVM22 7.2.8-1, score15 and READY +7.77s
- **highest s3 score**: rel71.36 26/7, 7.2.9-1, slow25s, BOOT failed after kernel change
- **fastest conditional s3**: rel66 +4.434s READY, caveat MCU park one hour
- **best independent warm repeatability**: rel71.30 18/20 first pose and 20/20 by 2, S3 BOOT unknown
- **best hard Tls recovery source**: rel71.24, targeted spidev rebind before device enumeration, S3 mixed

IMPORTANT: pass_once means one recorded success, not repeatability; mixed/fail must not be rewritten as pass. This historical table is not a population-level FAR certification.
