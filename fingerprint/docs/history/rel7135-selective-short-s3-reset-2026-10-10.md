# rel71.35 native selective short S3 reset — 2026-10-10

EXPERIMENTAL, NOT YET VALIDATED UNDER HUMAN POST-S3. No other service.

## Actual preceding rel71.34 field tests

At 15:17, right-index unlocked KDE once on first physical pose, MATCH 7/7.
KDE initiated genuine deep S3 at 15:17:32, awake again at 15:17:36.
Native 0x60 SLEEP was ACKED (attempted=1 ack=1 warm=1).
On wake, first native TLS session failed at 15:17:46. Existing full
SPI-closed/long-GPIO/reopen recovery failed again at 15:17:56. Driver never
reached ready, no biometric match score was possible.

Earlier genuine deep S3 12:52-13:00 under the same rel71.34 did restore TLS
but user finger matches only 4/7, 4/7, 3/7. Separate image-quality issue.

## Historical mechanism and code change

rel71.24 archived pre-enumeration SPI recovery used GPIO264 explicit
inactive LOW -> active HIGH10ms -> LOW, released request then settled 150ms.
The generic current long HIGH300/LOW600 MCU reset was never individually
validated on every deep-S3 recovery. The failed rel71.21 experiment proved
that blanket SPI close/reopen on protocol-defined resets fails TLS:
this experiment DOES NOT repeat that mistake.

Inside libfprint driver ONLY: added gx51_reset_gpio264_short via GPIO
character device v2, explicit initial LOW -> HIGH10ms -> LOW -> release
request -> 150ms settle. gx_reset_detached_s3_boundary selects short pulse
only when true S3 flag is set and the SPI and IRQ FDs are BOTH closed.
Cold Claim and outer whole-session recovery use this conditional pulse.
If short pulse fails, fallback to old long reset.
Normal Goodix protocol and TLS retry resets remain unchanged.
Flag is cleared only at full TLS + FDT + calibration production readiness.
No sysfs spidev rebinding under a live FpContext, no helper, timer, hook
or service. Enrolled fingerprint, PMK cache, match threshold7 unchanged.

## Evidence and live software QA

LXC700 pinned native libfprint build PASS, SHA256 source manifest PASS,
full research test suite PASS; isolated Arch makepkg and actual contents
audit PASS. No auxiliary GXFP units in the package.

Package SHA256:
83f1f3f735b7fe26908795736a8f90628f130ec4d7ea4eb9384c6befc52a9ade

On Pegasus rel71.35 installed with pacman -U, Snapper 1551/1552;
right-index enrollment preserved, existing fprintd active, Bitwarden PIN
and Polkit unchanged. First own-user native DBus Claim (no fingerprint)
succeeded ~8 s, driver retained production_ready=1 at 15:27:49.

This tests cold baseline only, NOT the new S3-specific logic.
Next: manually lock KDE and try RIGHT INDEX. If that normal lock
succeeds, manually initiate KDE deep S3, wake, try same index.
Read actual kernel S3 / ACK / native short pulse / TLS / matcher score.
Do not automatically sleep/reboot user's machine.
