# QA43 — bounded fast first capture on warm FDT liveness (2026-10-30)

Draft, disabled by default. Fork of PR42 commit 3c92fb0 including FDT typed32 and PR41 pacing fixes.

## Evidence
Pegasus first-touch LOCK 01:44:27-29 CET: FAST_READY 52ms; stale ImageBase age249214ms; FAST_READY to VERIFY_READY 835ms. FDT ACK retry and warm background refresh was discarded when finger landed. First original valid encrypted image still matched13/7 and positive returned1181ms after DETECTED_HOLD.

## Opt-in
GXFP_FIRST_CAPTURE_FAST_QA=1 defers the stale ImageBase refresh only if TLS/production warm state has already been validated, valid clean bg+FDT, age between 20 and 300 seconds, and recent successful FDT open proof less than2 seconds; rejects dirty background, sleep/cold boundary, stale TLS, pending transport recovery, invalid pointers and >300s reference. Default 20s freshness policy is unchanged without flag.

The first capture still passes through TLS-GCM, full contrast/image/quality and matcher score threshold7. Mandatory cleanup remains. If the first pose fails, normal rebase_before_retry invokes gx_warm_validate before the next pose (or defers on physical contact), preserving the original fail-closed and recovery behavior. A score below threshold remains a rejection, never an acceptance.

This is a latency-vs-stale-reference tradeoff. One success at 249seconds on Pegasus does not prove that all old backgrounds will match. Requires at least5 human first-touch LOCK trials, scores, retry rate and full UX timing before promotion. Do not merge until then. Preserve qa42 and rel71.74 rollback packages; do not force sleep or reboot.
