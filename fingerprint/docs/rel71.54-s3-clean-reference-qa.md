# rel71.54 - S3 early contact with last clean reference (QA)

**NOT hardware certified. Do not merge or tag stable.**

On 23 October 2026 the reference Pegasus S3 (deep) reliably woke but
rel71.52 needed six/seven finger placements and emitted libfprint
ACTION_NONE cancellable assertions. rel71.53 fixed the API assertions,
while retaining the existing finger-off-only ImageBase calibration.

rel71.54 additionally preserves the last known *clean* empty-sensor
ImageBase in **volatile RAM only** when an idle S3 park is ACKed and
the prior calibration is no older than 90 seconds. Old TLS state is
still invalidated and re-established after S3. If the first post-S3
FDT probe detects a FULL 6/6 finger contact, and actual suspend time
(CLOCK_BOOTTIME) + prior calibration age remain within 90 seconds,
the driver can use the previously clean background for that one
placement instead of falsely capturing the finger as ImageBase.

The current fingerprint capture quality, image scoring, enrollment,
and strict matcher threshold 7 are unchanged. A later clean sensor
rebase still happens; the code does NOT claim that a recycled old
ImageBase is freshly calibrated. Partial contact, long sleep, or
unknown calibration uses the established clean cold preparation
and may ask the user to lift their finger. Invalid backgrounds
remain forbidden. No fingerprint template or collected image is
persisted.

Verified in source only: five new S3 invariant tests, full libfprint
v1.94.100 native build and historical regression suite. Requires
real repeated S3, first-touch LOCK, wrong-finger rejection, cold BOOT,
warm reboot, fail-closed PAM and power tests on the MateBook.

The previous working rel71.51 remains installed on Pegasus until
both packaging and simulated Arch upgrade/rollback are successful.
No remote reboot or sleep; no kernel/PMK/GPIO112 modifications.
