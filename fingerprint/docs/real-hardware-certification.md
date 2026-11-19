# GXFP51A0 — evidence for real hardware certification

The driver is NOT certified until the **same installed package** passes
cold BOOT, reboot, true deep S3, repeated first-placement KDE LOCK, wrong-finger
rejection, and password fallback on actual hardware under human supervision.

## Critical lockout incident and rollback — 19 November 2026

The operator reported both fingerprint and password unusable after a
kernel-confirmed deep-S3 wake at 01:00:59 CET while the optional KDE Plasma
6.8.3 QML v12 was active. The operator forced a power-button reboot. The
surviving journal records low similarity scores, incomplete FDT down proof,
SPI/TLS recovery and kscreenlocker `Worker not ready` messages. The cause of
the lockout and the reason the password failed remain unproven.

Pegasus returned to stock Plasma 6.8.3 QML at 01:44 CET. Its verified SHA256
is `7a52a32c7dafe52c95f9e03042c4e34fdb539ebb6e997dfe2f7f773462b4416d`.
The rollback set `Fingerprint=false` and `LockGrace=0` and disabled the
pacman hook that reapplied v12. The integration helper keeps ordinary
`--apply` as a no-op. `--apply-experimental` is for supervised QA only.
`--remove` is reversible. The installed binary packages, native driver,
fprintd service, both templates, threshold 7, authenticated TLS and password
PAM configuration were preserved.

### Confirmed KDE behavior

The last-used authenticator still read `Authenticator=Fingerprint` after
the selectable fingerprint method had been hidden. The operator's
remembered state was backed up and changed to `Authenticator=Regular`
while Pegasus was unlocked. Keep `Regular` to avoid the known crash when
KDE tries to restore a remembered method that is now absent from the
selector. KDE marked the crash fixed for 6.8.4 in stable commit
[`8e02b5832aa9bb7da09f18cac40fecc7cf8c3c72`](https://invent.kde.org/plasma/kscreenlocker/-/commit/8e02b5832aa9bb7da09f18cac40fecc7cf8c3c72)
on 18 November. The installed 6.8.3 predates that fix.
[KDE bug #528042 and duplicate #528285](https://bugs.kde.org/show_bug.cgi?id=528042).

In 6.8.3, `Fingerprint=false` filters the explicit fingerprint descriptor
from the greeter's method selector. The `Regular` authenticator still
constructs an implicit fingerprint worker and starts it alongside the
password worker.
[Selector filter](https://invent.kde.org/plasma/kscreenlocker/-/blob/v6.8.3/greeter/pamauthenticatormodel.cpp#L62-68) ·
[Implicit worker](https://invent.kde.org/plasma/kscreenlocker/-/blob/v6.8.3/greeter/pamauthenticators.cpp#L133-142) ·
[Parallel start](https://invent.kde.org/plasma/kscreenlocker/-/blob/v6.8.3/greeter/pamauthenticators.cpp#L84-98).

The separate early password submission bug remains open. The 6.8.4
remembered-method fix addresses the crash described above.
[KDE bug #528356](https://bugs.kde.org/show_bug.cgi?id=528356) ·
[Plasma desktop merge request !4182](https://invent.kde.org/plasma/plasma-desktop/-/merge_requests/4182).
The grace-period greeter issue was also reviewed. Its role in the incident
remains unproven. [KDE bug #528681](https://bugs.kde.org/show_bug.cgi?id=528681).

### Observed password fallback after rollback

At 10:45:55 CET on 19 November, the kernel confirmed a deep-S3 resume.
The operator confirmed one successful password unlock on stock QML with
`Regular` selected. The exact password acceptance time was not recorded.

- At 10:45:56, the greeter emitted two `Worker not ready` messages. The
  password still worked in this cycle, so those messages alone cannot
  establish the cause of the earlier lockout.
- fprintd became ready at 10:45:59.401 after a reported total of 3,612 ms.
  At 10:46:28, one valid press was captured and rejected at score 6 with
  threshold 7. The journal recorded `cleanup_ms=547 ok=1`. The requester was
  not identified. The sample establishes one below-threshold rejection.

Repeatable password fallback and first-touch fingerprint unlock after
deep S3 remain unverified.

## Historical v12 trials — Pegasus, 18 November 2026

The following checks used the opt-in KDE Plasma 6.8.3 v12 lockscreen
workaround before its quarantine. The fixed biometric acceptance threshold
was 7.

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
and measured suspend power/battery impact. These historical v12 trials
do not certify the current stock-QML configuration.

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
