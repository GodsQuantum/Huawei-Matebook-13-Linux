# GXFP51A0 — evidence for real hardware certification

The driver is NOT certified until the **same installed package** passes
cold BOOT, reboot, true deep S3, repeated first-placement KDE LOCK, wrong-finger
rejection, and password fallback on actual hardware under human supervision.

## Passive evidence collection

Run `python3 fingerprint/scripts/gxfp51a0-hardware-evidence.py --json`
as a user authorized to read the system journal.

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
