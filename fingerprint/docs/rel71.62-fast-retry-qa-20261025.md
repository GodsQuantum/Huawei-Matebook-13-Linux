# GXFP51A0 rel71.62 — opt-in fast retry, 25 October 2026

STATUS: QA only. Not certified on real hardware. Based on rel71.61 source, but previous score-cache experiment stays disabled.

Actual rel71.61 Pegasus first pose scored 5/7 and was rejected; after finger release the driver attempted background refresh on the stored ~16-minute-old ImageBase. That refresh was discarded when the finger returned at 02:02:09.986, wasting approximately 996ms. The second physical pose scored 7/7 with the existing strict photometric rescue.

GXFP_FAST_RETRY_QA=1 enables immediate MCU rearm on a retry after a scored failure, only if TLS, warm context, native production readiness, FDT and background are intact, background is not dirty, and first-S3 TLS recovery is not pending. Otherwise the original stale-background refresh path is unchanged. With no QA flag all old behavior stays in place.

This only targets second-press delay. It does NOT improve first-touch recognition, allow multiple scored frames per physical pose, weaken matcher threshold 7, alter enrollment, or skip mandatory image cleanup. Normal fprintd/PAM/KDE integration, password fallback, PMK, GPIO and kernel remain unchanged.

Four new source-safety tests ensure opt-in and guards. Full native libfprint compilation and software baseline PASS (2026-10-25). Isolated Arch build, cross-distro CI and physical validation must still be completed. No fake unlock speedup claims without measured logs. Rel71.55 remains the actual installed Pegasus fallback until QA deployment.

Before production: five first-touch LOCKs, three real deep S3, three cold boots, three reboots, wrong-finger rejection, password fallback, in-flight Verify suspend. No remote forced suspend/reboot. Fingerprint templates and raw biometric images must remain private and unchanged.
