# rel71.74 QA - authenticate already-held finger during S3/S4 wake

State: LAB SOURCE ONLY until packaged and real trials. After rel71.73 the operator reported S3 at least five physical touches and S4 five to six touches. Actual kernel deep S3 and hibernation S4 confirmed. Finger-matching quality was not the root cause.

## Hardware evidence
At S3 23:54 CET: automatic prewarm saw live 0x3f finger contact and 4.740s wake preparation. First authenticated image matched score 15/7, above unchanged threshold 7. The 0x32 FDT post-image cleanup returned one reply instead of two despite a good ACK; mandatory cleanup was attempted and the TLS session marked desynchronized. Existing code discarded this ALREADY validated image, requiring a new physical press, full GPIO/TLS recovery and repeated CALIBRATION_LIFT_REQUIRED while the real finger remained on sensor.
At subsequent real S4 23:57:04: earlier warm session failure had left native suspend with warm=0; the old code explicitly discarded prior clean ImageBase. On waking, finger 0x3f covered sensor and the driver kept asking for finger removal. Automatic prewarm took 18.379 seconds. Finger scores 2/7 and 3/7 preceded 12/7 success; operator estimated 5 to 6 human poses.

## Two default-OFF QA changes
GXFP_HELD_FINGER_ROLLING_BG_QA=1: while the sensor is proven finger-OFF, background from normal successful TLS ImageBase calibration and four clean FDT samples is independently copied into RAM before subsequent biometric session errors. Prior trusted RAM-only background is preserved at S3/S4 park even if the warm TLS/transport context is already invalid, and is strictly age-bounded by CLOCK_BOOTTIME (12h). When waking with a finger already placed, the driver attempts extra read-only live FDT proofs before entering finger-OFF loops. The existing full six-zone genuine contact and ADC evidence are mandatory before restoring this old no-finger baseline. Actual fresh fingerprint capture and encrypted TLS-GCM + image quality + fixed matching threshold 7 still run ONLY after a real PAM Verify request. Never learn a touched background and never reuse an earlier authenticated fingerprint image.
GXFP_POSTAUTH_CLEANUP_DEFER_QA=1: the normal encrypted post-image cleanup is still executed. If that cleanup fails AFTER the worker has ALREADY decrypted/authenticated a new image and computed a positive score >=7, preserve this cryptographic/biometric evidence, report the existing successful Verify or Identify to PAM, and mark force_cold_reset so the dead TLS session cannot be reused on any subsequent Claim. For negative scores, enrollment, absent/rejected images, missing or failed TLS-GCM authentication, or cancellation, behavior is original fail-closed and complete transport recovery. No cleanup step is silently omitted.

## Hard physical limit
A brand-new sensor with no prior independently validated no-finger baseline cannot derive an accurate reference while a finger covers the glass. In that exceptional case the user must briefly lift or use password fallback. No authentication before the PAM Verify request and no biometric template/PMK exposure.

## Software validation
Dedicated test_held_finger_resume_qa.py initially RED then GREEN, 5 safety assertions. Full native libfprint make verify passes on CT700 without touching production sensor. Real LOCK, natural S3, real S4 are pending on this source, and it MUST remain experimental with rollback. No agent-triggered suspend/reboot.


## Deployed QA state (28 October 2026 local, 00:15 CET)

Arch package libfprint-goodix51a0 1.94.100.goodix51a0-71.74 from source commit c796136, SHA256 20c9ce0ee5548c033cba7619076a0ad16b8bf77e635b41a4fb1881407a27ab09. Built only in CT700 Arch container. Installed on Pegasus over existing rel71.73 via pacman -U, Snapper pre1958/post1959. ACTIVE fprintd PID627240, pacman -Qkk 32/32 files intact, enrolled right index preserved. Production service flags include BOTH independent QA repairs, other tested rel71.73 settings unchanged.

Native automatic cold initialization logged PMK cache hit+TLS attempt1 good, verified clean ImageBase at 00:15:30 (NATIVE_S3_CLEAN_REFERENCE_SAVED age206ms RAM-only). This proves the new rolling-cache hook is exercised in cold fprintd startup, but NOT that first touch after a real S3 or S4 is fixed; physical operator tests are pending.

Previous SHA-pinned Pegasus rollback rel71.71-B speed package SHA 6a34c70bc6001b7c6d7f2213ae56b239cd8371d9e811c584edb4bb54b59a8314, and rel71.72 SHA 0018d1fba0d13f12c8c078f870dff29b7ed0d610dcd6d872beea2a85ad38cd67 preserved. No forced power cycle or re-enrollment. To undo QA: sudo pacman -U --noconfirm /var/lib/gxfp51a0-qa-rollback/rel71.71-B-speed.pkg.tar.zst
