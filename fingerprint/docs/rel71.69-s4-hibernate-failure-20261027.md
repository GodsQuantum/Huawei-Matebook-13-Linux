# Rel71.69 — true S4 failure on 27 October 2026 (OPEN)

This is a real user-visible login availability failure on Pegasus, Huawei MateBook 13 2021, Goodix GXFP51A0 ST411. Do not claim S4 reliability. Evidence: operator report + local kernel/fprintd journal; no raw capture, personal machine addresses or keys.

- 17:41:42 local KDE PowerDevil requested hibernate, kernel confirms S4 entry at 17:41:47. A native proven clean pre-sleep reference was saved at 17:41:44.
- 17:54:03 actual S4 resume, same fprintd process, no OS reboot. 17:54:09 / :19 / :30 / :35 finger-like FDT touch=0x3f and ADC mean 200–238 against true clean ~355; rejected background. Contact is not proof that the user's finger was physically placed.
- 17:54:41 native resume prewarm FINALLY READY, **38.69 seconds** after wake. 17:54:43 FAST_READY83ms then VERIFY_READY; at 17:54:45 Verify closed with **zero fingerprint detection, images, scores or match**. User reports biometric unlock unavailable. Exact count of finger placements unknown.
- Another S4 resume at 17:19:09 had extended calibration delays and did eventually reach positive biometric score20/7 at 17:19:49; physical first-placement count unknown.
- Prior S4 at 14:57 with operator-confirmed first pose passed; previously misclassified as LOCK in historical HOF ledger; corrected to S4.

Suspected logic hole: gx_s3_try_early_finger_reference is called only before initial calibration image. If finger-like contact occurs DURING capture, the driver rejects that frame and loops in clean-finger-off waits, without retrying the saved RAM-only age-bounded clean pre-sleep reference. This is a testable hypothesis, not a certified remedy. A lab candidate can reuse the previously proven clean snapshot only with its existing full 6-zone contact, age, FDT and strict TLS image matcher checks, never accepting a contaminated background. Nothing new installed on the reference machine.

Tracking https://github.com/GodsQuantum/Huawei-Matebook-13-Linux/issues/32 . rel71.69 is flagged experimental / prerelease. Do not remove password login. Repeated S4, S3, cold BOOT and wrong-finger validation still needed.
