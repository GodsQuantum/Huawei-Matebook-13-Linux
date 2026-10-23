# rel71.52 - native S3 resume-first TLS/ImageBase prewarming (QA)

**DO NOT MERGE MAIN OR TAG STABLE. Hardware S3 first placement NOT CERTIFIED.**

## Observed hardware failure on rel71.51
Pegasus Linux S3 deep 23 Oct 2026:
- 01:40:10 suspend, 01:40:24 wake; NATIVE_S3_PARK 0x60 acked.
- Resume open 01:40:27, native detached SPI reset and TLS state prepared.
- 01:40:34, :38 and :42 the first 3 user placements occurred during
  ImageBase calibration (FDT 6/6, 6/6, 4/6); reference was rejected.
  First actually scored capture at :49 matched 7/7, done at :50.

## New candidate
libfprint calls GXFP51A0's idle resume vfunc; it now schedules the
existing cold prepare/TLS/clean ImageBase on a GTask worker, and completes
the libfprint resume only after the worker has closed its transport.
The suspended flag blocks competing Verify until ready. Failure
(including a resting finger) safely falls back to cold Claim: no
contaminated background is accepted, no threshold reduction and no
password-authentication bypass.

A user touching *during* the background calibration still needs to lift
the finger until a clean no-finger ImageBase is available. The system must
not misleadingly treat early touches as failed biometric matches.

## Verified
- Six added source regression tests RED on rel71.51, GREEN on rel71.52.
- Research suite and native libfprint v1.94.100 source/patch/Meson C build
  PASS on isolated LXC700. No live sensor I/O, kernel or GPIO mutations.

## Missing physical proof
The previous firmware logs contained idle S3 park but no NATIVE_S3_RESUME
callback marker; it is uncertain if fprintd calls this resume path in the
exact sleep/wake sequence on Pegasus. A real human-triggered S3 with
rel71.52 and timestamped journal is mandatory before claiming any speedup.
Current installed rel71.51 and rollback are kept untouched until that
controlled test. No automatic sleep/reboot.
