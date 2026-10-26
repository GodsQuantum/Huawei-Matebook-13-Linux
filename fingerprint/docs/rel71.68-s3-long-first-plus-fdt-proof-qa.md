# rel71.68 — combine first S3 long reset with strict rel71.67 FDT proof (EXPERIMENTAL)

## Why this specific combination
rel71.67 true S3 23:39 CET with short-first reset needed an extra long detached reset after first post-resume TLS handshake FAILED digest check; first successful Verify 13/7 at wake+13.129sec, prewarm9.665sec, user estimates ~2 physical poses uncertain. Hall of Fame rel71.55 best wake-to-success 6.951sec, prewarm4.254sec, but 2/3 first touch passes.

rel71.66 true S3 was HARDWARE NO-GO: long first reset brought up TLS and accepted saved clean ImageBase at wake+5.844sec, but subsequent accepted GET_IMAGE never delivered authenticated TLS image, no biometric unlock; rel71.66 lacked rel71.67 FDT proof.

rel71.67 source audit demonstrated gx_send_plain_drain() returned TRUE for FDT_DOWN(0x32) with fewer than two expected plaintext replies and/or missing success ACK; opt-in GXFP_FDT_DOWN_PROOF_QA=1 fixes this unsound acceptance state. Under real rel71.67 S3 GET_IMAGE image was authenticated after fallback. CAUSAL LINK to rel71.66 timeout is NOT proven.

## rel71.68 single isolated change
Starting rel71.67 source, reuse the reviewed rel71.66 opt-in GXFP_S3_LONG_FIRST_QA=1 to select existing detached GPIO264 300/600ms pulse on ONLY first genuine S3 boundary with spi_fd and irq_fd closed. Default short-first remains when opt-in disabled. Normal LOCK, cold boot, ordinary protocol resets and post-failure recovery byte-for-byte unchanged. Keep rel71.67 strict FDT proof, TLS GCM record validation, no accepted GET_IMAGE replay, presleep clean reference and matcher threshold7. Right index enrollment, password fallback, PAM and kernel unchanged. No GPIO112.

New long-first source 4/4 and strict FDT source 6/6 passed, native libfprint verify complete; require independent Arch build and GitHub multi-platform CI 12/12 before QA installation. QA natural user S3 must validate exact pose count and timing; rollback to pinned rel71.67 or rel71.64 on failure. Never claim first-touch S3 success before physical test. PR DRAFT. No forced reboot/sleep.
