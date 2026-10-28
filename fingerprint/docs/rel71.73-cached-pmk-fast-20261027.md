# rel71.73 QA — eliminate needless second MCU reset after secure PMK cache load

State: source laboratory only until QA installation, and never stable without repeated real S4 / LOCK / S3 physical trials. Based on rel71.71-B behavior and rel71.72 stage profile.

## Real hardware evidence
At 23:29:38 during standard fprintd cold startup on Pegasus under rel71.72, metadata-only stage logs prove: DriverState ACK in11ms; firmware A8 in13ms; **secure PMK cache hit=1**, immediately followed by **reset_reason=pmk_post_stage** and GPIO264 full hardware reset (300ms HIGH/600ms LOW). After that target config and authenticated TLS 1.2 handshake both succeeded on attempt 1 (config1142ms, handshake66ms). This same second full reset was visible in earlier S4 23:02:21.957 and is a likely contributor to excess time.

The old code performed post-PMK reset unconditionally whenever PMK was loaded, whether it came from fresh MCU boot staging F2/E4 or the existing verified root-owned cache. Factory staging requires resetting MCU to clear its temporary boot state. Merely reading a trusted local file does **not** change MCU staging, so this post-stage reset is redundant for that cache path.

## Source fix
Experimental GXFP_PMK_CACHE_SKIP_RESET_QA=1 skips the second 300/600ms reset *only* if gx_pmk_cache_load already validated regular file, owner/euid, permissions0600 and exact 48-byte length, setting self->psk_from_cache=TRUE. It retains all config, D0 REQTLS, mandatory full TLS-GCM handshake and authenticated image/matcher threshold7. All initial device reset and firmware read, DriverState, S3/S4 prewarm, genuine factory PMK staging and its post-staging reset, PMK freshness fallback and transport error recovery remain unchanged.

This switch defaults **OFF** in the public source, is individually reversible and does not weaken TLS, PAM, enrollment or target sensor security. If cache is absent/invalid or fresh F2/E4 staging is needed, prior reset/A8 path runs byte-for-byte unchanged. If cached PSK fails handshake, original bounded TLS retry and fresh staging fallback run. No secret bytes, templates or encrypted records are written into logs.

## Benchmarks and acceptance
Prediction: removing one full reset (300/600ms) plus one A8 round-trip should save around one second, assuming no retry regression. The theoretical saving is not a measured result until a *hardware* cold fprintd initialization with this flag and another natural S4 pass. Compare stage=pmk_cached_skip_post_reset, stage=tls_attempt config_ms/handshake_ms, full cold context time, RESUME_PERF_QA wake-to-ready and real first-touch LOCK/S4/S3. If TLS fails or images regress, roll back immediately to SHA-pinned rel71.72/71.71 package.

Offline RED-then-GREEN test test_pmk_cache_fast_boundary_qa.py, complete native make verify and no-sensor test suite: PASS on CT700. Real user physical tests: PENDING. PR remains DRAFT. No forced user hibernation or reboot.
