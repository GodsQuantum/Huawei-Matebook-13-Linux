# QA66 — reuse previously verified clean S3 reference during calibration

Exact baseline QA63 main 309978d7c521faf016adab068042de84514880ff.
QA65 300ms reset change intentionally not carried forward: QA63 timing remains.
Field root cause QA65 S3 second test at 23:49:45: full touch 0x3f while
calibration awaited finger-off; 8s lost before discarding touched background
and restoring saved clean RAM image at 23:49:53.

QA66 opt-in GXFP_S3_CALIB_CONTACT_REUSE_QA=1 applies solely to two native
S3 finger-off waits. With saved valid clean RAM reference and full 6/6
finger detected, re-prove the original age-bounded S3 reference with a
SECOND independent live FDT before leaving calibration early. Failed proof,
partial contact, absent or stale saved reference all retain strict clean
dwell and contaminated-image rejection unchanged; never accept touched
ImageBase. TLS/PMK and encrypted image+quality+matcher7 unchanged. No
GPIO edge mode, firmware, re-enrollment or external services.

Native tests and real Pegasus trials must pass before promotion to main.
QA63 package rollback SHA256
553c9fc2611d86f345eca1ad96f6ada501e9a4ea98371267a9ce7f1042f41b3c.

## QA66 — 2026-11-04 ~00:03 CET — experimental driver INSTALLED on Pegasus; physical field measurements pending
- Source exactly QA63 main 309978d7c521faf016adab068042de84514880ff. Isolated branch qa/gxfp-qa66-s3-finger-during-calibration-20261103 source commit e2174aec0fd7f2f5ebc109209c5e34d6d9e90030, verified pushed GitHub; remote main remains QA63. Worktree private/experiments/qa66-s3-finger-during-calibration-20261103 clean after commit.
- QA66 fixes QA65 observed S3 long prewarm (touch during calibration at 23:49:45). Only two initial native S3 clean-anchor and preimage dwell loops opt-in via GXFP_S3_CALIB_CONTACT_REUSE_QA=1, full6/6 FDT+independent second clean-S3-reference proof, return only on success. If not proven, unchanged strict clean-dwell, rejection of touched ImageBase and timeout. QA63 250+500ms S3 reset retained; no QA65 shortening, QA64 GPIO edge; unchanged TLS/PMK, FDT and matcher threshold7. Defaults disabled in source, enabled in experimental package service.
- CT700 full make -C fingerprint verify-no-build PASS, includes new 4 QA66 source tests, 3 Huawei QA66 install tests, no sensor writes. Native libfprint 1.94.100 compile 116/116 PASS, all artifact gates PASS. Actual Pegasus makepkg --cleanbuild EXIT0, candidate Cloud9 private/candidates/qa66-s3-calib-contact-20261103/libfprint-goodix51a0-1.94.100.goodix51a0.qa66-1-x86_64.pkg.tar.zst SHA256 a569672c672d1e450502150d791ab847caf327346eb226504b2cf9f5dc12a81f. ELF native SHA256 9884da20b6932576495d10da6fe73d385059a112bf6c253fea49f3cac3635a27.
- Pegasus package QA63-1→QA66-1 installed, Snapper root PRE2102 POST2103. pacman -Qkk 32/32 files unmodified, fprintd active PID286262, /proc/PID/maps confirms new library, fprintd-list preserved right-index-finger; real service environment has GXFP_S3_CALIB_CONTACT_REUSE_QA=1. GPIO48 still input only. No reboot, custom daemon, firmware changes or biometric re-enroll. QA66 package cached /var/cache/pacman/pkg, SHA verified. Root-readable QA63 cache SHA256 553c9fc2611d86f345eca1ad96f6ada501e9a4ea98371267a9ce7f1042f41b3c checked for fast rollback. Build temporaries removed from Pegasus.
- Hardware field test QA66 still pending: human needs LOCK+genuine S3, finger placed immediately on unlock/repeated presses as needed, report presses. Then inspect journal for QA66_S3_CALIB_CONTACT_REUSED, RESUME_PERF_QA stage/cold/total, first-press score and actual wake→success. Compare QA63 2/2 S3 first-press, 3/3 LOCK first-press, prewarm3550ms, wake→success5597ms. If S3 regression, immediate verified QA63 rollback. No merging QA66 into main without positive real tests.
