# QA42 — hardware trials after reboot, 31 October 2026

Physical placements are reported by the operator. Internal driver
counter 1/3 counts DETECTED holds, not necessarily all physical placements.

Environment: same QA42 installed binary SHA256
8adcaa2d5c18b55045967d0574ca6920fb28687cff96b58d2494c9f041d33131
as Oct30 1.181s record, pacman-Qkk32/32. KDE/KWin 6.8.2,
full boot Oct31 21:19:33. Prior KWin and Plasma binary processes from
Oct28 were replaced by newly started ones Oct31 21:38:23.

## First login after reboot — PASS, 1 physical pose

21:38:22.075314 DETECTED_HOLD touch0x3f, 6 zones mean228/drop128;
score12/7 at21:38:22.731111, cleanup570ms; success21:38:23.302419.
Detection-to-success 1227ms. This is not elapsed time since startup.
Earlier repeated 30s VERIFY sessions in the greeter do not establish
earlier physical failures and are not counted as physical placements.

## First S3 resume — PASS, 1 physical pose

21:53:34.456728 pre-suspend parked; real S3 resume began
23:12:05.802775, clean reference prewarm ready23:12:11.301048
(5498ms from resume begin). FAST_READY85ms at23:12:13.604130
already sees touch0x3f. DETECTED_HOLD at23:12:13.855733
mean221/drop136, match score7/7 at23:12:14.919109,
cleanup764ms, positive23:12:15.683925.
Detection-to-positive1828ms, resume begin to positive9881ms
(includes human finger arrival time).

## Ordinary LOCK — FAILED first-touch, success on 4th pose

23:17:50.339307 FAST_READY84ms, ImageBase age340825ms >20s
freshness requirement. WARM_REBASE starts23:17:50.384838,
retries one FDT ACK and accepts clean ImageBase after1219ms at
23:17:51.603890 (idle355/floor331). VERIFY READY at
23:17:51.609537. DETECTED_HOLD at23:17:51.805924
just 196ms after READY (6 zones, mean228/drop126).
Image match score14/7 at23:17:52.836311, cleanup759ms,
positive23:17:53.595536. Detection-to-positive1790ms.

Operator explicitly reports FOUR physical finger placements, whereas
the driver shows one detected hold (physical press 1/3). The
three earlier placements were NOT detected and their exact timing
relative to the 1219ms warm rebase is unknown. This is a genuine
first-touch failure despite excellent matcher score14/7.
It is consistent with first touches during the not-ready refresh,
but causality has not been established with a human placement timestamp.
Do not present 1790ms as a successful first-touch performance record.

## Decision

Full reboot brought KDE processes back into consistency and QA42
detects actual finger again. Reproducibility not yet certified:
BOOT1/1, S3 1/1, ordinary LOCK still has failure. S4 not tested
after reboot. Keep byte-identical QA42 in place. Do not weaken
fresh background requirements, TLS-GCM, strict threshold7 or
enrollment. Investigate readiness display vs timing before
modifying the native driver.
