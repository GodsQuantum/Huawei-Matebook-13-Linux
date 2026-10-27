# GXFP51A0 rel71.74 — reference-machine production (28 October 2026)

The owner elects to promote the existing rel71.74 QA driver to normal GitHub main, release, and the standard HUAWEI/fingerprint installer, notwithstanding incomplete repeat-S4 certification. This is a **reference-machine release**, not a guarantee for all models.

## What was actually validated
- rel71.74 installed on Pegasus with original QA Arch package, verified 32/32 files, enrolled right index, fprintd native startup and password fallback. User reported LOCK first physical pose, and a first-pose S3 result.
- Prior rel71.73 was poor: S3 at least 5 physical poses, S4 5–6. Live S3 image was positively TLS-authenticated with score15/7 but lost its result after subsequent FDT cleanup failure; next S4 discarded the saved no-finger ImageBase after an unhealthy TLS session and waited 18.379s with finger already covering sensor.
- rel71.74 preserves an independently proven clean RAM-only ImageBase (bounded to 12 hours) for real early-contact FDT before a new TLS-GCM image; it can preserve a completed positive match if subsequent cleanup fails while retiring the desynchronized TLS transport for later use. No cached biometric login, no relaxed score7, no changes to PAM/password.
- The later attempts at 00:31 (S3 deep then s2idle) and 00:33 (S4) did not actually enter suspend: Linux user-task freezer blocked 20s on KDE kded6 and kioworker in autofs_wait, while NetworkManager stopped Wi-Fi and /mnt/Cloud9 attempted to mount via NFS. Screen only locked. These attempts are NOT evidence of working S3/S4 under this release. S4 repeat reliability, cold boot, multi-machine and wrong-finger testing remain open.

## Production packaging and evidence
- Normal official install: fingerprint/install.sh => install-linux.sh => install-arch.sh on Arch/CachyOS. Version 1.94.100.goodix51a0-71.74, with same settings as the tested QA package.
- Reference options: GXFP_S3_FRESH_BG_QA, GXFP_FDT_DOWN_PROOF_QA, GXFP_SPI_CS_SETUP_QA, GXFP_S4_MIDCAPTURE_REUSE_QA, GXFP_RESUME_PERF_QA, GXFP_CAPTURE_PERF_QA, GXFP_RESUME_FAST_SETTLE_QA, GXFP_RESUME_STAGE_QA, GXFP_PMK_CACHE_SKIP_RESET_QA, GXFP_HELD_FINGER_ROLLING_BG_QA, GXFP_POSTAUTH_CLEANUP_DEFER_QA. Individually reversible via fprintd service drop-in.
- Full native libfprint verification and 12/12 GitHub CI on code candidate, then recheck after promotion for installer and packaging changes.
- Existing Pegasus exact QA package rollback: /var/lib/gxfp51a0-qa-rollback/rel71.74-held-finger-QA.pkg.tar.zst SHA256 20c9ce0ee5548c033cba7619076a0ad16b8bf77e635b41a4fb1881407a27ab09. Older rel71.71 and rel71.72 rollbacks are still pinned; preserve them.
- No extra service, sleep hook, firmware flash, enrollment reset, secret/biometric logging or global PAM modifications. Keep normal password authentication.

## Unfinished certification
Fix KDE vs NFS automount freeze separately without interrupting active sessions, then repeat real, natural S3 and S4 after first finger already held to prove first-touch. This release should not be described as a fully certified S4 fingerprint driver until repeated trials actually pass.
