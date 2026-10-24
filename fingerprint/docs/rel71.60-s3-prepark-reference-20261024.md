# rel71.60 — pre-S3 clean reference (experimental opt-in)

Not production certified. The stable rel71.55 remains the rollback.

## Observation on Pegasus

2026-10-24 real S3, installed rel71.59: native suspend logged a
clean RAM-only ImageBase age 1,051,990ms (~17m32s) BEFORE
deep S3. At resume 21m49s-old reference and early finger
contact yielded scored 0/7. Synchronous clean finger-off waits
after wake repeatedly failed, adding seconds and breaking usability.
Normal later LOCK with 6m49s-old image scored 16/7, so age
alone is not a sufficient predictor.

## New 71.60 behavior (opt-in only)

GXFP_S3_PREPARK_FRESH_QA=1. Only on a real native libfprint
S3-suspend callback with sensor proven finger-OFF, refresh a
reference older than the regular 20s bound BEFORE MCU sleep.
Use existing authenticated TLS GET_IMAGE and require a
strictly newer calibration timestamp plus fresh finger-OFF
FDT. If refreshed image was contaminated, do not snapshot it.
The standard sleep command and ACK are unchanged.
No added work on initial BOOT, routine LOCK or a reference
already younger than 20s. No extra daemon, timer or hook.

Tradeoff: potentially ~1.2s extra during actual suspend entry,
none at the moment of the user's first post-wake biometric
capture, but physical timings remain UNVERIFIED.
Existing firmware and matcher threshold=7 unchanged.
Never enable rejected rel71.56, .58 or .59 QA options together.

Real LOCK, deep S3, cold BOOT, reboot, active Verify suspend,
wrong-finger rejection and password fallback remain untested
with this version. Do not merge until hardware certification.
