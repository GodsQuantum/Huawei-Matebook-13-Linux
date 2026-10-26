# rel71.67 — FDT-down proof + post-ACK TLS delivery diagnostics (EXPERIMENTAL)

## Directly observed problem
On genuine deep S3 2026-10-26 22:25 (rel71.66) the user needed multiple poses and had no biometric unlock. S3 clean ImageBase and initial TLS prewarm completed, but the first user GET_IMAGE received a command ACK and **never delivered an authenticated TLS image** within 4s. Subsequent recovery and calibration could not complete. rel71.65 failed the initial TLS handshake after its short-reset path; changing short/long reset did not solve capture reliability. Both candidates rejected, GitHub PR #27 and #28 CLOSED. rel71.64 restored and remains the only deployed baseline; rel71.64 is *not* S3-certified.

## Targeted protocol bug found in reviewed rel71.64 code
In gx_send_plain_drain(): for FDT-down (opcode 0x32), the implementation already knows Windows ST411 sends **two plaintext replies** (ACK and a subsequent FDT state report), and waits up to 100ms for the second. But after the wait it unconditionally returns success for all non-GET_IMAGE commands even with **zero or one** reply, or no valid ACK, so the following GET_IMAGE can be launched before the image gate was proven armed. This is a real invalid acceptance state found by source audit; the missing FDT response has NOT been shown as the causal factor in the particular rel71.66 S3 event because fprintd previously did not log the counts at warning level.

## Patch (natively inside libfprint only)
- Opt-in GXFP_FDT_DOWN_PROOF_QA=1. Only for opcode 0x32, require both independent plaintext replies AND a parsed successful ACK; if absent, log sanitized reply count/ack and request existing whole-session recovery. Never retransmit an uncertain accepted FDT command nor an accepted GET_IMAGE.
- On exhausted existing 4s TLS-image IRQ wait, log only counts of GPIO48-high observations, SPI frame read errors, stray plaintext frames and attempts. No extra device transfers, GPIO operations, polling, key disclosure, raw TLS record or biometric image dump. Normal success path identical except counter increments.
- Preserve TLS-PSK/AES-GCM authenticated record decryption, first-scored-frame policy, threshold 7, original enrolled right-index, strict ImageBase clear proof, all existing cancellation and password fallback. This starts from known rel71.64 *source*, not failed rel71.65/rel71.66 features. Experimental flag OFF by default. Avoid tests that change kernel, SPI driver, GPIO112, GPU or PAM.

## Verification / acceptance gates
- New focused source/semantics test was RED on baseline then 6/6 GREEN with patch.
- Full source/software, native libfprint v1.94.100 Meson/Ninja compile and Arch package + multi-distribution CI are required before first QA installation.
- Hardware claim NOT proven by source tests. First perform LOCK physical contact, then human-attested natural deep S3. Check proof log, TLS IRQ counters (only if timeout) and recorded first physical pose. Reject/rollback immediately on unexpected LOCK/S3 regression.
- No forced S3/reboot. Keep SHA-pinned rel71.64 rollback c3df31e8dd06bfc778c661a023333a632f2824b4e841bbdf524ea1c4aa46e932 and rel71.55.


## 2026-10-26 23:11 CET — QA package built/installed, hardware acceptance still open
- Native Meson/Ninja libfprint-v1.94.100 build PASS, new unit/source tests 6/6, full software safety build PASS.
- GitHub draft PR#29 12/12 CI checks PASS including Arch, Alpine, Debian, Fedora, Ubuntu 24.04/26.04, openSUSE, KDE native/PAM and software baseline (commit c4c2350).
- Isolated disposable Arch base-devel container made package 1.94.100.goodix51a0-71.67 (SHA256 08681c32c1234b0fae0f6f61017d50df18f75bc1528a62fdb3d864b300e4d09c). Package transferred over existing Pegasus Cloud9 NFS after identical hash; ephemeral transfer deleted. Keep checksum-pinned package in CT700 /home/arezki/Dev/temp/ for rollback reproducibility; deleted 61MiB disposable Arch staging.
- Pegasus pacman transaction SUCCESS, Snapper root PRE1930/POST1931, pacman-Qkk 32 files /0 altered. Native fprintd ACTIVE PID311522, ONLY GXFP_S3_FRESH_BG_QA=1 GXFP_FDT_DOWN_PROOF_QA=1 set; no long-first or cleanup-settle QA. Right-index enrolled intact as shown by fprintd-list. Historical rel71.64 pinned rollback SHA256 c3df31e8dd06bfc778c661a023333a632f2824b4e841bbdf524ea1c4aa46e932 verified; rel71.55 retained. No reboot/suspend forced, no kernel/GPU/PMK/PAM change.
- fprintd initialization 23:10:07/09 logged 2 failed TLS handshakes with digest check failed before service started 23:10:14. Similar startup retries existed under rel71.64; do NOT claim the FDT change remedies initial TLS handshakes. Authentication hardware LOCK and true S3 have NOT YET BEEN TESTED in 71.67. Need owner's real pose counts, exact fprintd/kernel logs, warning FDT_DOWN_PROOF or TLS_FRAME_TIMEOUT_DIAG if applicable. If regression, revert to rel71.64 automatically.
- Public PR remains DRAFT until real S3 first-touch and negative-finger gates; strict fingerprint threshold7 unchanged.
