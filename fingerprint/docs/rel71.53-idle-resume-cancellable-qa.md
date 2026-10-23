# rel71.53 - Native idle-S3 cancellable fix (QA)

Status: software candidate; NOT hardware certified; no stable release.

23 October 2026 physical S3: deep suspend 08:19:18-08:19:23, then
6-7 fingerprint placements. Sensor finally matched 9/7 at 08:19:59.
Repeated libfprint fpi_device_get_cancellable assertion with ACTION_NONE
occurred in idle resume prewarm. Seven touches contaminated potential
ImageBase captures and were correctly rejected.

Change: gx_action_cancellable_if_active is now the only direct caller
of fpi_device_get_cancellable. It returns NULL for idle ACTION_NONE,
avoiding libfprint assertions, and returns actual cancellable for active
fingerprint actions. S3 resume begin/ready markers moved to warning
level for reliable journal coverage. Background quality gates, strict
match threshold 7, PAM password fallback, and S3 recovery policy
are unchanged.

5 targeted tests initially failed against rel71.52 and now pass.
Build/native libfprint research suite, Arch makepkg and GitHub Actions
required before next physical test.

This fix does NOT establish first-touch S3 success. A separate clean
pre-sleep reference strategy and measured physical tests are needed.
No remote suspend/reboot, kernel/firmware/PMK change, or merge to main.
