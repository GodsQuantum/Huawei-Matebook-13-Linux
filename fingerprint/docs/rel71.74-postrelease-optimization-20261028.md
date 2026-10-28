# GXFP rel71.74 postrelease QA: executed behavioral controls, 28 Oct 2026

Production rel71.74 and Pegasus binary are unchanged. The following tests are run on CT700 without SPI, power management, biometrics or network changes.

## Compiled-source characterization
- FDT test compiles and executes the ACTUAL extracted goodix51a0.c calibration loop with scripted probe returns, not a search for source keywords. Six cases: four clean accepted; one clean/three missing ALSO accepted; two clean/two touched ALSO accepted; four missing rejected; four touched rejected; four low ADC rejected. On current source even one of four clean replies is enough to cache a no-finger baseline, contrary to the 4-clean comment. The driver still requires fresh authenticated TLS fingerprint and matching threshold7; this is a reliability gate issue, not an authentication bypass.
- Postauth cleanup test compiles and executes ACTUAL decision code from the capture worker. Ten cases include positive score >=7 plus subsequent failed FDT cleanup: keep the completed biometric result, mark TLS cold reset; no acceptance if score negative, image missing, no Verify/Identify claimant, feature disabled, or transport desync not recorded; successful cleanup keeps original probe and pacing. Also checks callback cancellation precedes result handling. Caveat: extracted code with fake dependencies is not full asynchronous SPI/TLS/SSM integration.

## Latency optimization candidate (NOT implemented)
Rel71.71 sample LOCK first pose: 1254ms, image571ms + SIFT3ms + matcher119ms + mandatory cleanup559ms. S4 first pose:1748ms, image894ms + SIFT3ms + matcher93ms + mandatory cleanup755ms. The cleanup recipe is FDT_UP, GET_IMAGE with required TLS-GCM image record, NAV, FDT_POST_DOWN, then a final gap sleep (gx_capture_gap_us) after the last command. Last gap may be redundant but has no hardware proof; its maximum theoretical saving is only tens of milliseconds. Do NOT remove mandatory GET_IMAGE, skip TLS records, lower score threshold7 or remove this pause until real A/B first-touch hardware trials.

## Next gates
Keep QA branch isolated, full native make verify and GitHub CI, then collect actual FDT 4-probe statistics before a stricter baseline rule: increasing minimum good replies could recreate 15-second finger-off loops. KDE/NFS freezer must be fixed separately before any real S3/S4 evidence. No automatic suspend or reboot.
