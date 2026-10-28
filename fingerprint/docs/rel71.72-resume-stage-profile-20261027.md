# rel71.72 LAB — S3/S4 stage profiling (not stable)

Source branch qa/gxfp51a0-rel71.72-resume-profile-20261027. Current installed on Pegasus is rel71.71-B until a separately verified QA package changes that.

## Grounded timings
At 23:02 on 27 October 2026: operator-attested S4 first physical press success, score8/7. Native prewarm4.826s, including reset+settle0.910s and subsequent full TLS/background3.915s. Extra long GPIO264 reset300/600 at 23:02:21.957 after the early short reset23:02:21.258, followed by FDT ACK retry. Cause unresolved: DriverState no ACK, PMK boot staging/cache miss, FW probe fallback or TLS retry. Sensor already automatically prewarms in gx_dev_resume before KDE fingerprint Claim.

## Source change
Independent GXFP_RESUME_STAGE_QA=1 enables metadata-only warn-level logs identifying each long-reset cause and exact stage durations: DriverState, FW A8, context, PMK cache hit/miss, TLS configuration and handshake by attempt. No PMK/key bytes, raw images or fingerprint templates logged. **No protocol, reset, TLS, security, latency or timing behavior changed.** OFF by default.

## 2026 upstream comparisons
- Sigfrodr/libfprint-goodixtls (https://github.com/Sigfrodr/libfprint-goodixtls): sibling GXFP5187 verified that shortening a required inter-command30ms pause to10ms failed19/19 times, and too-short encrypted-record draining could lock up the MCU. GTask workers for long TLS/capture are already used in this driver.
- szlukabence/goodix-fingerprint-spi-linux (https://github.com/szlukabence/goodix-fingerprint-spi-linux): exact GXFP51A0 active-HIGH GPIO264 reset and SPI_CS_HIGH requirements; standard secure TLS-PSK, MCU and target interoperability; no proven shortcut to session initialization.
- berkekbgz/libfprint-goodix-spi (https://github.com/berkekbgz/libfprint-goodix-spi/blob/main/re/PARITY.md): ACK and next response can share one IRQ window; over-reading destroys next header. Never blindly skip GET_IMAGE cleanup or drain.
- Thermal or UI retry storms are separate from time-to-ready; do not add duplicate pollers or user-space helper services.

## Next test, only human-initiated
Observe first-touch normal LOCK and a natural S4 with QA profiling. If reset_reason=driverstate_silent consider one **separate, default-off** read-only A8 readiness experiment before slow fallback. If reset_reason=pmk_staging inspect owner/mode of existing secret cache without reading key content. If reset_reason=tls_in_process_retry inspect stage/handshake failure: a slightly longer initial readiness delay might actually be faster than paying for a second hard reset. Never assume removing a long reset is safe until actual traces show it is unnecessary. Keep rel71.71-B and rel71.70-A SHA-pinned rollback.

This branch cannot be called stable until multiple S4 first-touch, S3, LOCK, cold boot and negative-finger tests have passed. No remote suspend/reboot.
