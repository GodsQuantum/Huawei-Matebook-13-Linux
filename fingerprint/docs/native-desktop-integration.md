# Native Linux desktop integration

GXFP51A0 is implemented as a normal libfprint device. There is no
GXFP51A0-specific KDE or GNOME user interface.

```text
GXFP51A0 hardware
    ↓
libfprint goodix51a0 driver
    ↓
fprintd D-Bus service
    ↓
KDE / GNOME / PAM / command-line clients
```

## Driver contract

The production driver exposes the standard libfprint lifecycle:

- `open` / `close`
- `suspend` / `resume`
- `enroll`
- one-to-one `verify`
- one-to-many `identify`
- `FP_SCAN_TYPE_PRESS`
- enrollment-stage count
- `FP_FINGER_STATUS_NEEDED`
- `FP_FINGER_STATUS_PRESENT`

- standard retry and terminal match/no-match results
- cancellable operations

`identify` matters when several fingers are enrolled: fprintd can satisfy
`VerifyStart("any")` from the gallery instead of forcing a desktop frontend to
know which template to choose.

## Lifecycle policy

Enumeration and biometric preparation are deliberately separate.

`probe` is passive with respect to the sensor protocol: it checks only that
the host transport can be opened and closed. It never resets the MCU, starts
TLS, captures a background image or calibrates FDT.

A real `open`/Claim owns sensor preparation. It starts from a deterministic
MCU boundary, performs bounded TLS/background/FDT preparation, and reports a
protocol error if that preparation cannot be completed. It never reports an
open device while the capture context is known to be unusable.

A validated warm context may survive an ordinary Claim/Release for low latency,
but never blindly survives a power-state boundary. Native libfprint
`suspend`/`resume` callbacks invalidate it during active use, and an
idle-suspend fallback detects elapsed sleep from the
`CLOCK_BOOTTIME - CLOCK_MONOTONIC` delta at the next Claim.

Current public production packages do **not** install an external sleep hook,
resume worker or periodic keepalive. The driver's resume callback invalidates
stale state and the next Claim rebuilds TLS/background/FDT, so the first
post-S3 use may still be slow or fail if the finger touches during calibration.
Reliable eager, cancellable native recovery after real deep S3 remains an open
hardware validation requirement, not an already-shipped feature.

## Authentication latency

A scored verification/identification result is reported to libfprint
immediately. Waiting for the physical finger to be lifted is cleanup and must
not delay PAM or the login manager.

The driver never retries a usable biometric capture merely because its score
was close to the acceptance threshold. Score-conditioned retries multiply
false-accept opportunities. Only unusable image-quality captures are retryable.

## Desktop policy

PAM/login policy belongs to the distribution and desktop. The driver and public
installer do not edit `/etc/pam.d/*`.

A correctly configured desktop sees only the standard fprintd device and
properties: `scan-type`, enrollment stages, `finger-needed` and
`finger-present`.


## Authentication policy, fail-closed

Run password and fingerprint in **independent PAM workers** when the
desktop supports concurrent authentication; serialising `pam_fprintd`
inside `system-auth` can delay password entry.

Do **not** combine a fingerprint-only stack containing `pam_fprintd`
as `required` (especially `-auth required`) with `pam_permit.so`.
Linux-PAM treats `PAM_IGNORE` as ignored, not a failure; if other
required modules return success the biometric stack can succeed without
a positive fingerprint match. This was reproduced with real Linux-PAM
using a temporary `pam_start_confdir` policy and `pam_debug.so`.
Use `[success=done default=die] pam_fprintd.so` as the final
**mandatory biometric decision** after preliminary required checks
and omit `pam_permit.so`. Always keep the independent password service.

The included KScreenLocker compatibility PAM template is fail-closed,
but it is not installed by default on every distro. Review the *actual*
PAM files and the active greeter before promoting a desktop profile.

KScreenLocker 6.8.0 may fail to show the fingerprint *hint* even when
fingerprint authentication works (KDE bug 527171). The stock Plasma
Login Manager 6.8.1 PAM service does not automatically start separate
biometric verification (KDE request 506025). Do not apply the historic
Plasma Login Manager **6.7** binary/QML patch package to **6.8**.
