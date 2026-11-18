# GXFP51A0 — evidence for real hardware certification

The driver is NOT certified until the **same installed package** passes
cold BOOT, reboot, true deep S3, repeated first-placement KDE LOCK, wrong-finger
rejection, and password fallback on actual hardware under human supervision.

## Observed, not fully certified — Pegasus, 18 November 2026

These checks are **after deployment of the opt-in KDE Plasma 6.8.3 v12
lockscreen workaround** on the reference Pegasus laptop. They do not
change the fixed biometric acceptance threshold of 7.

| Kernel-confirmed deep S3 entry → exit (Europe/Paris) | fprintd recovery | First scored placement | Eventual verified match |
|---|---|---|---|
| 12:05:55 → 12:27:01 | Native S3 prewarm ready 12:27:05; FAST_READY 86 ms | 4/7, rejected | 11/7 at 12:27:17, after repositioning |
| 13:02:25 → 13:22:42 | Native S3 prewarm ready 13:22:46; FAST_READY 84 ms | 6/7, rejected | 13/7 at 13:22:52, after repositioning |

Both suspends have kernel **PM: suspend entry (deep)** and **PM: suspend
exit** events and a post-resume libfprint positive result. Both show that
the sensor and KDE authentication can recover after a genuine deep-S3
cycle; **neither was a first-placement success**. These two cycles
preceded the right-index guided re-enrollment later that afternoon.

Separately, after that guided re-enrollment, five of six logged,
independent right-index test placements scored at least 7 (one scored
3 and needed repositioning). They are not a false-acceptance study.

**Still open:** the required third deep-S3 cycle with in-progress
verification, five consecutive first-touch LOCK successes, real cold
boot/reboot gates, repeated S4 hibernation, unknown-finger controls,
and measured suspend power/battery impact. Password fallback stays
available. Do not mark the current driver + KDE v12 combination as
hardware-certified or merge an experimental fix solely based on CI.

## Passive evidence collection

Run `python3 fingerprint/scripts/gxfp51a0-hardware-evidence.py --json`
as a user authorized to read the system journal.

When different builds were installed during the same boot, filter at the
exact installation instant to avoid mixing evidence from older versions:

    python3 fingerprint/scripts/gxfp51a0-hardware-evidence.py --since 2026-10-23T11:44:05+02:00 --json

Use the installation timestamp from the package manager. A sleep starting
before that instant is excluded even if its resume is later. The JSON
analysis_since field records the selected filter, or null without it.
This prevents false attribution, but does not prove the installed binary
was built from the QA commit.

The utility reads only the current boot's fprintd and kernel journal; it
does not modify GPIO, SPI, kernel, services, PAM, PMK, firmware, or sessions.
It never initiates sleep or reboot, and contains no persistent task. It emits
only event counts, S3 timestamps, score/threshold and readiness timing. It
never exports biometric frames, templates, hostnames or raw journal lines.

For offline synthetic/log input use:
`python3 fingerprint/scripts/gxfp51a0-hardware-evidence.py --fprintd-log FPRINTD --kernel-log KERNEL --json`.
Journal input format: `journalctl -b -o short-iso`. Do not commit real logs.

## Interpretation

- `clean_reference_saved`: native idle park acknowledged with a clean
  finger-off FDT and bounded previous ImageBase.
- `native_resume_called`: a native libfprint resume callback actually ran.
- `prewarm_ready_seconds_after_wake`: time from resumed kernel to a
  prepared SPI/TLS/background context.
- `cached_reference_used`: RAM-only strict FDT-gated reference reused;
  this is **not** an authenticated fingerprint by itself.
- `calibration_lift_required_count`: sensor was touched during calibration.
  It is NOT the number of physical fingerprint placements.
- `matched_score` and `successful_match_seconds_after_wake`: first
  verified score and timing. These do not establish first-touch success.
- `hardware_certification` always remains `PENDING` until human validation.

## Hardware release gates

1. Five consecutive LOCK first-touch unlocks with password fallback.
2. Three real deep S3 cycles, including an in-progress verification;
   capture first-touch outcomes, no fprintd claim leak and recovery timing.
3. Three actual cold power-offs and power-ons, biometrics at login manager.
4. Three warm reboots, biometrics at login manager.
5. Wrong/unregistered finger reliably rejected, password works.
6. No sensor wedging, measured wake power/timing, known-good packages and
   rollback verified.

A positive score alone—even 7/7—is not a hardware certification.
