# QA62 experimental — stale ImageBase normalization shadow, non-authorizing

- Extends QA61 with one opt-in RAM-only diagnostic when a valid finger frame was already decrypted and ImageBase age>20s.
- From the SAME frame used for ordinary fingerprint verification, compute a local 9x9 mean-detrended ridge image, run SIGFM extract and compare to the enrolled views; log numeric metadata only: STALE_SHADOW_QA age_ms primary_score shadow_score shadow_points decision=primary_only.
- No second GET_IMAGE, no raw fingerprint frame/template stored or logged, no auth decision based on shadow score. The ORIGINAL score governs exactly the same one-frame Verify/Identify, 7/7 threshold, cleanup, fixed 3-press budget, password fallback.
- Local detrending is a hypothesis based on imaging normalization in fingerprint literature. It is NOT proven for ST411, can be slower and can have shadow scores below baseline; do not promote to real matcher until multiple genuine + wrong-finger negative traces and latency evidence pass.
- QA62 flag GXFP_STALE_LOCAL_SHADOW_QA=1 active only on candidate. If flag OFF, QA61 behavior and performance identical.
- This experiment is strictly to identify a safe route to one-touch recognition after a user places finger during an ImageBase refresh; no 2s user delay should be required.
