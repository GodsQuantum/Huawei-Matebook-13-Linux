# GXFP51A0 rel71.55 - live-cleared S3 RAM reference (QA)

Not a hardware-certified release, do not merge to main.

Real Pegasus suspend 23 October: NATIVE_S3_PARK warm=1 at 11:05:42,
native idle NATIVE_S3_RESUME_BEGIN at 11:13:24, PREWARM_READY at 11:13:35,
but CALIBRATION_LIFT_REQUIRED after finger landed while preparing.
rel71.54 had a 90-second image reference lifetime and emitted no
NATIVE_S3_CLEAN_REFERENCE_SAVED (the pre-S3 image was over 90 s old).
The 8-minute S3 interval itself was also longer than that lifetime.

rel71.55 requires a LIVE six-zone FDT finger-off proof immediately before
the native acknowledged 0x60 sleep packet. Only if that probe succeeds
and the existing ImageBase/FDT is valid, clean and under two hours old
is its ImageBase copied to RAM. Never persisted; invalidated if no
acknowledged S3 park. The saved copy can be attempted once for FULL 6/6
finger contact within 12 hours total pre-age + deep-sleep elapsed.
If stale/partial/touched-before-suspend, use the complete normal
ImageBase calibration. The saved background never constitutes a
positive fingerprint; actual image quality and strict score threshold
7 still apply, and only a validated enrolled finger can authenticate.
After finger lift the background must be refreshed by the normal
warm-rebase path.

Source and native build + Arch makepkg + CI required.
Actual S3, BOOT, reboot, first-touch and wrong-finger physical
certification required. Keep root-owned rel71.49+stock PLM rollback.
No remote suspend/reboot or kernel/GPIO112/PMK modifications.
