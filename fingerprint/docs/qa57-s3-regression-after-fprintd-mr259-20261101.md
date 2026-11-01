# QA57 / fprintd MR259 — five detected contacts on true deep S3 (2026-11-01)

Feedback: S3 : bcp de poses. Evidence: Pegasus precise fprintd and kernel logs, via SentinelX Cloud9 SSH. No biometric material kept.

- Version during trial: QA57-1 driver unchanged; local upstream fprintd MR259 backport 1.94.5-2.2 installed.
- Kernel deep S3 22:27:56.652392 to 22:28:02.796647. Native S3 park idle=1 attempted=1 ack=1 warm=1. Native resume began 22:28:02.810906, prewarm 3530ms success, FAST_READY 83ms. No idle=0, ReleaseDevice BUSY, or Cannot run while suspended. Thus the MR259 active-Identify suspend fix was NOT exercised.
- Contact 1, 22:28:09.499958: FDT preimage 2/2 valid twice (61ms); image rejected by contrast-quality gate, FDT postcapture cleanup command 0x32 ACK1 DATA0 (22:28:10.974865), fail-closed reset and secure TLS rebuild.
- Contact 2, 22:28:13.406732: frame571ms, matcher best4/7, postcapture FDT cleanup ACK1 DATA0 again (22:28:14.667990), full MCU/TLS reset.
- Contact 3, 22:28:16.703433: frame572ms matcher4/7, cleanup success.
- Contact 4, 22:28:21.491356: frame569ms matcher4/7, cleanup success after clean calibration.
- Contact 5, 22:28:24.691301: frame570ms, raw score4 and QA54 rotation rescue angle -12deg score8 effective7/7, cleanup 549ms success; AUTH positive 22:28:25.855739.
- First detected contact -> positive 16355.781ms rounded 16356; native resume -> positive 23044.833ms rounded23045, including user interactions. Two FDT failures exclusively in post-capture cleanup 0x34 -> 0x20 -> 0x50 -> 0x32; valid 13 of 15 typed responses. QA57 image timing remained good, around 570ms; quality contrast and weak matcher scores are separately observed. Never accept ACK alone or weaken threshold/TLS.
- Comparison: previous QA57 deep S3 (21:49) with stock fprintd was first touch success, direct 21/7, FDT 3/3 valid, prewarm3538ms, 6609ms resume->positive.
- Mitigation performed: rollback fprintd to original signed CachyOS 1.94.5-2.1, SHA256 59701283ce2aeeb4b20be98dc08e4c5e5c39a117db49f676f7b491b8760c1bcf. Verified 270 files/0 altered, daemon active and enrolled right-index-finger retained. QA57-1 driver unchanged. MR259 candidate retained on GitHub PR65 Draft and archive Cloud9, not merged. This rollback removes the changed service from next A/B; does NOT prove it caused existing intermittent FDT failures (they also occurred on stock daemon).
- Next investigation: narrowly instrument postcleanup FDT IRQ/wire semantics and warm reference quality, not merely lengthen timeout (QA56 200ms found no useful late reports). No driver patch was installed during this recording.
