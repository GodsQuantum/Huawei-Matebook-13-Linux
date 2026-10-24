# 2026-10-24 rel71.58 — real deep S3 hardware evidence

Installed version on Pegasus: 1.94.100.goodix51a0-71.58,
fprintd with GXFP_S3_STALE_REBASE_QA=1. Kernel 7.3.0-4-cachyos.
These sessions are all POST INSTALL; do not mix with rel71.55.

## S3 at 20:15:37.851+02 (after 19:58:08.931 deep entry)

Native resume begins 20:15:37.876, prewarm ready 20:15:45.365:
7.489 seconds. First FDT Claim FAST_READY 82 ms.
Verified physical image scores 2/7 at 20:15:56.706, 3/7 at
20:16:01.673, 2/7 at 20:16:05.535 — three failed scored
presses. Subsequent image rejected on quality (no score) after
touch lifted; finally the next scored image achieved 21/7 at
20:16:11.183 and terminal 20:16:12.057.
Wake to unlock 34.206s. This is demonstrably NOT first-touch.
No GXFP_S3_STALE_REBASE_QA refresh-wait log occurred after
those initial misses: the ImageBase was younger than 20s, so
the new conditional code skipped recalibration. It does not
solve every root cause of poor post-S3 recognition.

## S3 at 21:32:18.990+02 (after 20:41:20.712 deep entry)

Native prewarm 21:32:19.037 -> 21:32:23.996:
4.959 seconds. Live early finger FDT=0x3f; saved RAM-only
ImageBase aged 4,598,865 ms (~77m); refresh deferred with
finger present. First detected contact 21:32:24.514,
image REJECTED BEFORE SCORING by contrast-quality gate
21:32:25.432; finger no longer detected for quality recapture
21:32:25.732. A second scored finger detected 21:32:27.179,
accepted 14/7 at 21:32:28.235, terminal 21:32:29.135.
Wake to auth 10.145s. At least two FDT-detected contacts,
so this was not a one-touch unlock.

## S3 at 21:43:20.848+02 (after 21:43:16.973 deep entry)

Short ~3.875 second real deep suspend.
Native prewarm 21:43:20.860 -> 21:43:25.956:
5.096 seconds. Live early finger FDT=0x3f;
old verified RAM-only ImageBase aged 5,260,729 ms.
ImageBase warm rebase attempted at 21:43:27.280 and
discarded 21:43:28.489 because a finger touched the sensor.
First SCORED image (which is NOT proof of first physical
placement) scored 9/7 at 21:43:29.736, terminal
21:43:30.677. FDT recorded the scored contact at
21:43:28.695 -> success 1.983s. Wake -> auth 9.829s.

User message at ~21:43 states S3 unlocked on second physical
placement. **Exact wake association is uncertain** because
two S3 cycles occurred between 20:41 and 21:43.
Do not attribute a first-touch PASS to this short S3 on
the basis of its first logged scored image: user-reported
second-touch behavior wins over log-level guesswork.
The 21:32 two-contact failure is independently visible in logs.
The latest 21:43 cycle is conservatively labeled unknown
until precise user-to-cycle correlation.

## Diagnosis and release decision

Experimental rel71.58 is **not hardware S3 certified**.
The stale-ImageBase guarded rebase requires an old background
at the moment of retry; 20:15 three low biometric scores with
a freshly calibrated background bypassed the opt-in logic.
21:32 suffered an early-finger low-quality frame and
a delayed fingerprint availability. First-touch regression
gate fails despite passing all static CI.
Do not merge the PR, reduce threshold 7, skip TLS cleanup,
or falsely claim this build is more reliable/faster.
No automatic reboot/suspend/greeter restart was performed.
