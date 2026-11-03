# GXFP51A0 — Architecture and reliability audit, 2026-11-03

Scope: Huawei MateBook 13 Pegasus Goodix GXFP51A0 ST411/GF_ST411SEC_APP_14115. Read-only analysis of hardware logs and HALL_OF_FAME.md, QA57 v QA62 sources and recent GitHub findings. Review perspectives: Superpowers systematic debugging and verification, EGM zoom-out/critique/implementation-readiness rubric (no isolated subagent review; do not claim a full Goldfish multi-agent review), Ponytail full code review, Karpathy minimal changes. No biometric image or template recorded.

## Mandatory invariants

- Immediate contact, including pre-placed finger, is the user goal; requiring a two-second wait is a diagnostic only, NEVER a solution.
- One recognized physical press by default, threshold >=7, strict TLS/FDT typed report, never ACK-only as finger authorization, no false acceptance, fprintd stock + KDE native PAM. Preserve index enrollment and credential/password fallback.
- No firmware flash, additional service, timer/polling process, unrequested reboot, raw biometric persistence, automatic reinstall of downgraded HUAWEI.
- Never promote a synthetic unit-test PASS as real hardware performance or reliable S3/S4.

## Zoom out: evidence against linear QA progression

- QA42 had 1/5 first-touch LOCK success. QA54 fastest 1157.858ms 1/1 LOCK. QA57 1156.903ms on 2026-11-01 19:45:19, direct9/7, image572ms, cleanup565ms, exactly one LOCK trial; genuine deep S3 21:49 one pose at1173ms with unchanged stock fprintd.
- QA57 under a different experimental fprintd MR259 later needed five contacts in deep S3, so no causal driver comparison was established; MR259 rolled back. QA60 1/3 first-touch; QA61 0/1; QA62 user multiple even though late positive. Latest QA62 scores actual primary4/3/2/2/2/7, alternative4/2/2/2/3/3: alternative rejected.
- Human presses are not identical to log DETECTED_HOLD count, and <10 repeatable matched trials do not establish a statistical reliability figure.

## Root cause trace

- After 19–40 minutes idle, ImageBase age >> GX_BG_FRESH_MAX_US 20sec. The existing fprintd Open uses FAST_READY in 52–84ms, then session thread blocks on gx_warm_validate, which takes ~0.8–0.9s to capture a no-finger reference. Finger arriving during this period makes subsequent FDT indicate touch, so candidate reference is correctly discarded. Authentication continues with a retained clean-but-old frame; genuine scores may collapse to 2–4/7. After a failed press, the worker repeats stale background refresh at every retry, often racing the next press, causing a self-reinforcing failure loop.
- Clean reference captured after sensor is finger-free did precede some successful subsequent attempts on QA60, but placement/rotation is confounding: do NOT assert the background was the only cause.
- Also seen ACK-only FDT missing typed data, GPIO IRQ line low; strict recovery is correct and must stay. Source on a different USB sensor notes similar contamination, but a different chip does not validate applying USB-specific data or timing to SPI ST411.
- The raw capture+matching path in QA57 takes ~572+15ms and the secure post-capture cleanup ~565ms, accounting for most of the 1157ms record. To beat it materially, a safe callback split would need to emit an authenticated match before protocol cleanup while completing cleanup before next device operation. libfprint supports fpi_device_verify_report followed by fpi_device_verify_complete, but fprintd behavior and strict cancellation/transport lifecycle on this target must be tested. NOT ready to deploy.

## Ponytail / Karpathy critical findings

1. MUST FIX: several QA branches change multiple timing, background, FDT and matcher features, defeating one-variable causal attribution. Stop stacking. Baseline QA57 archived verified checksum on Pegasus.
2. MUST FIX: stale background refresh in foreground worker races normal early contact. No clean reference can be derived from a contact-contaminated capture; never force-accept it. A correct solution must alter the acquisition timing or prove a safe equivalent, and must test user touching before/during/after refresh.
3. MUST FIX: QA62 shadow extra computation adds latency to the hot path but scored lower, remove from live installation (rollback QA57 completed).
4. SHOULD FIX: strict real-world promotion gates absent; a release is invalid if only a single fast sample exists or if unknown-finger, S3 or S4 lacks evidence.
5. SHOULD FIX: ACK-only FDT failures recur even with pacing changes; an additional timeout without real typed response is not a security fix.
6. SHOULD FIX: users should never need manual re-enrollment just to compensate a known no-finger ImageBase age race. Reenrollment is not causal treatment.
7. NICE TO HAVE: performance beyond the 1.157s latency floor by separating match callback and cleanup under native libfprint's state machine, contingent on independent functional proof and accurate first-press timing.

## Future controlled sequence, not a new test treadmill

- QA57 baseline active, package archived in Pacman cache (SHA256 e0d5fde84daa42a89936076050781b455e483e7425b699ed9b5c0d3270aa7a16); QA62 retained as rollback SHA750ed55a.... Do not repeat the existing passing cold-start/stock-fprintd tests without changes.
- Design a single focused failing regression test for the stale ImageBase race: clear FDT at Claim, finger arrives during background capture, background is not overwritten and existing match is not silently weakened. Prove RED against previous code before implementation. Consider proven no-contact pre-capture scheduling using native libfprint/KDE lifecycle, not timers/services; reject a proposal that cannot handle early press.
- Separately evaluate fprintd match-report-before-cleanup semantics on an isolated test driver/fake device. Do not bundle with background fix. If it proves safe, test late cancellation and wrong-finger negative, log monotonic match-report and cleanup-complete separately.
- Promote only after meaningful real LOCK after idle and immediate finger, resumed deep S3, genuine S4, cold BOOT, and wrong-finger rejection; compare against QA57 1156.903ms. Hardware evidence collection should be passive as user uses machine, not forced repeated lock cycles.
- Main/HUAWEI installers currently QA59.2; keep frozen until a confirmed winner. Do not call it PRODUCTION READY.

## External reviewed references

- ST411 exact SPI reverse engineering https://github.com/szlukabence/goodix-fingerprint-spi-linux
- USB 55a4 reference for FDT and clean background (different chip, not a drop-in fix) https://github.com/Hydrogell/goodix-27c6-55a4-fingerprint-linux
- Other 55a4 user study on print coverage and time of contact (not exact hardware) https://github.com/jith/goodix-55a4-fingerprint
- Official libfprint verify_report and verify_complete API https://fprint.freedesktop.org/libfprint-dev/libfprint-2-Internal-FpDevice.html
- Official fprintd VerifyStatus contract https://fprint.freedesktop.org/fprintd-dev/Device.html
- Own issue on S4 ImageBase and prewarm https://github.com/GodsQuantum/Huawei-Matebook-13-Linux/issues/32


## 2026-11-03 14:40 correction — rollback archive lacked runtime flags

The true QA57 archive was reinstalled on 2026-11-03 around 14:13 but the original separate /etc/systemd fprintd service drop-in was absent. Both GXFP_DECOUPLE_ACK_PACING_QA=1 and GXFP_ROTATION_RESCUE_QA=1 are opt-in in QA57 source. Thus the 14:13 to 14:38 installed package did NOT reproduce the Nov 1 Hall of Fame runtime. The original QA57 documentation explicitly confirms both were ON during the 1156.903ms result.

Restored /etc/systemd/system/fprintd.service.d/70-gxfp-qa57-pacing-decouple.conf with exactly those two Environment settings; reload/restart fprintd; verified package32/32 files, index finger retained, active env, native TLS/PMK success at 14:39:28. QA57 archive SHA e0d5fde84daa42a89936076050781b455e483e7425b699ed9b5c0d3270aa7a16.

This recovers the historical configuration; it is NOT a verified first-touch rate or faster-than-record measured lock. The stale ImageBase/early-finger race remains unresolved. No new unproven QA driver was installed. Note: newer Berkekbgz Chicago code only targets a sibling with different MCU; not safe to swap blindly.
