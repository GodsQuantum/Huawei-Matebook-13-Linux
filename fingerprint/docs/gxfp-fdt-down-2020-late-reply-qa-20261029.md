# GXFP51A0 — 2020 FDT-down second-reply compatibility experiment (2026-10-29)

## Provenance and baseline
Contributor @szlukabence, [issue #6, last comment](https://github.com/GodsQuantum/Huawei-Matebook-13-Linux/issues/6#issuecomment-6501280844), reported independent 2020 WRTB-WXX9 rel71.74 testing, Omarchy 7.2.8, fprintd 1.94.5-2, Quickshell:

- First-placement lock 7/9, within two 9/9; wrong finger 0/6; regular touch-to-decision ~1.0–1.2 s.
- s2idle first-placement success; this is **not** a deep-S3 test. Native S4 first-placement success 7.3 s after wake.
- **7 of 10** accepted sessions had an incomplete post-capture FDT-down proof (1 clear reply / expected 2, ACK=1), with expensive next Claim 3.5–6.5 s versus warm FAST_READY 51–85 ms.
- The hypothesis that the second plain report is simply delayed beyond 100 ms is **UNPROVEN**. It may be absent, mislabeled or unrelated, and extending timeouts can worsen latency when there is no report.

## Candidate behavior, default OFF
The production driver remains rel71.74. Opt-in compile-time-protected bounded 2020 investigation: `GXFP_FDT_DOWN_LATE_REPLY_QA=1`, **only** in a test build, raises the silence timeout after the first 0x32 reply from 100 ms (10 x 10 ms) to **250 ms (25 x 10 ms)**. No unbounded wait, repeated 0x32 command, change to the 1000 ms GET_IMAGE ACK window, or overall biometric retry budget.

When opted in, require **both** an ACK for command 0x32 with accepted status **and** a structurally valid checksum-verified 0x36 FDT report. Ignore unrelated/duplicate plain frames as proof. Even if the legacy `GXFP_FDT_DOWN_PROOF_QA` flag is unset, opting in still mandates strict proof. Missing or malformed proof forces the existing full TLS/MCU rebuild. This is more stringent than the legacy 2-plain-reply count and does not lower matcher threshold 7, bypass any TLS-GCM capture, change enrollment, disable cleanup or alter PAM.

Sanitized log label `FDT_DOWN_LATE_REPLY_QA`: number of valid replies, ACK bit, valid-data bit, interval from first to second in ms or -1, configured 250 ms. It logs no sensor zones, pixels, fingerprints, templates, record contents or keys. The normal 100 ms configuration is identical to rel71.74.

## Executed software verification
`research/tests/test_fdt_down_late_reply_behavior.py` compiles the **actual** extracted C drain implementation with simulated IRQ events. It covers on-time ACK+FDT, delayed second reply under default vs opt-in, second reply absent, wrong/unrelated response, duplicate ACK before data, duplicate ACK only, and missing ACK; absence/malformed frames must fail closed. `make -C fingerprint verify` additionally runs existing source safety checks and native libfprint build. A successful software test proves the control flow, **not** that real 2020 hardware emits a late reply or that latency improves.

## Hardware go/no-go (must be requested from contributor)
On the 2020 model, compare default 100 ms vs opt-in 250 ms on the same set of e.g. 10 natural lock unlocks, including first-touch, bad finger and S4. Capture only `FDT_DOWN_LATE_REPLY_QA` counters/times, number of bounded MCU rebuilds, FAST_READY / Claim-to-ready, GET_IMAGE no ACK, TLS/decrypt errors, real wake mode, scores/placements. A delay histogram demonstrating validated 0x36 packets at 101–250 ms is REQUIRED to justify enabling or widening by default. If still one reply or malformed, disable the opt-in; investigate missing response source rather than increasing timeouts. Repeated genuine deep-S3 and S4 on Pegasus remain uncertified separately.

**Not hardware-certified; not installed; no merge into main or stable release.** Rollback: keep rel71.74 installed; unset QA variable to restore original path in test builds.
