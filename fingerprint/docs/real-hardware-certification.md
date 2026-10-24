# GXFP51A0 — evidence for real hardware certification

## Durable Hall of Fame and regression gate

Original historic record: [31 pre-existing version/configuration entries](LEGACY_HALL_OF_FAME_20261013.md) and [29 additional candidate workspace folders](LEGACY_CANDIDATES_INVENTORY.md) (folders are not successful trials). The complete private original remains in the CT700 Pegasus workspace.

Current canonical hardware ledger: [HALL_OF_FAME.md](HALL_OF_FAME.md)
and its machine-readable [HARDWARE_TRIALS.csv](HARDWARE_TRIALS.csv).
The metrics are attributed to the exact installed version and kernel
resume/login boundaries, with human first-placement reporting and
pointers to written evidence. A fast FDT probe is not a full unlock.

After each physical test: measure log timestamps, append evidence to the
driver-version report and CSV, regenerate Hall of Fame with
python3 fingerprint/scripts/gxfp51a0-hall-of-fame.py --write,
run it again with --check, execute make -C fingerprint verify, and
push all updates to GitHub. This is an **offline developer workflow**:
nothing is scheduled, installed as a watcher on Pegasus, or runs
without explicit interaction.


The driver is NOT certified until the **same installed package** passes
cold BOOT, reboot, true deep S3, repeated first-placement KDE LOCK, wrong-finger
rejection, and password fallback on actual hardware under human supervision.

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
