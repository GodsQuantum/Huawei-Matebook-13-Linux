# QA42 — failed LOCK trials before full KDE reboot, 30 October 2026

These were real operator-reported failures with unchanged QA42-1, not
QA44 or QA45. They occurred AFTER the successful QA42 LOCK at 01:44
and after Plasma 6.8.1 -> 6.8.2 was installed at 01:46, while
pre-upgrade KWin and plasmashell processes continued running from
October 28 (their original executable files were deleted/replaced).

## LOCK at 10:18

User: LOCK failure, multiple physical finger placements. FDT
FAST_READY51ms, ImageBase refreshed807ms with finger-off reference.
VERIFY READY10:18:03.371885, CLOSE at10:18:07.479587
(4.108s later). No DETECTED_HOLD, fingerprint image, matcher score
or successful authentication. Number of placements not accurately
recorded, do not infer from driver internal 1/3 indicator.

## LOCK at 10:26

User: failure. FAST_READY51ms, ImageBase refresh accepted806ms,
VERIFY READY10:26:25.669543. With targeted driver debug
G_MESSAGES_DEBUG, FDT manual probes reported idle mean353-355,
drop0-1 versus strict finger threshold above30, during 5.206s;
client cancelled10:26:30.875. No DETECTED_HOLD or capture.
Actual number of physical placements unknown.

## Interpretation

The two failures followed SUCCESSFULLY ACCEPTED fresh ImageBase;
the 01:44 success had an interrupted/discarded ImageBase when the
finger arrived during refresh. This suggests a temporal or sensor
state issue but does not prove the refresh causes failures. In
addition KWin/Plasma binaries were replaced after 01:44 but were
not reloaded until a full machine reboot October31.
Subsequent real LOCK after reboot passed only on the fourth
physical placement, whereas BOOT and S3 each succeeded first
pose. No source change is attributed here as certain cause.
Neither failure belongs in any first-touch record. Keep QA42
and distinguish physical finger count from internal 1/3 counter.
