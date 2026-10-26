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
