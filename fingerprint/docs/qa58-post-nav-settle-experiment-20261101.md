# QA58 — conservative post-NAV settling before final FDT_DOWN (opt-in)

## Trigger: factual evidence, not proven cause
The 2020 external issue #6 reports final FDT 0x32 report missing in 2/24 and 7/20 cleanups, and occasionally on the next pre-capture command. On reference 2021 Pegasus QA57 2026-11-01 at 22:28: one contrast-rejected first capture, three under-threshold 4/7 captures, two post-capture FDT 0x32 ACK1/report0 failures; first-to-terminal 16.356s across five detected contacts. Deep S3 TLS+prewarm were fully successful; no fprintd stuck device. Earlier QA56 tried 200ms (versus 100ms) *after ACK*: NO later FDT reports were observed and no benefit demonstrated.

## Hypothesis and minimal implementation
The final post-capture FDT_DOWN follows a NAV (0x50) with a 30ms recipe gap. A lost report might be related to sensor readiness after NAV and image-record processing, rather than merely the response window after the last command. QA58 adds a bounded **50ms extra gap before sending that last FDT_DOWN**. Exact helper takes previous/current command codes and returns 50ms only for 0x50→0x32 when GXFP_POST_NAV_SETTLE_QA=1; default remains byte-for-byte timing of QA57. This is a *hardware experiment*, not evidence of a fix.

The capture recipe, FDT_POST_DOWN payload, required successful 0xB0 command ACK and distinct checksum-valid data tag 0x32, timeout100ms, TLS authenticated intermediate cleanup image, matcher threshold7, negative/unknown finger and 2020 support constraints, security fail-closed on missing data, S3/S4 native lifecycle, password fallback, and no-replay-after-ACK rules remain unchanged. No daemon patch or extra sleep hooks. Setting is NOT installed as a default in generic package builds.

Measured tradeoff if opt-in: +50ms per completed cleanup, paid even on successes; pays off only if report loss and cold rebuilds fall enough to compensate. Do not promote absent repeated human physical first-placement successes. Pre-capture FDT is unaffected and existing image frame timing should remain ~570ms; verify in logs. If reported FDT missing, fail closed and rollback QA58 without diluting evidence.

## Gates
- C compiled helper default off; exact command-boundary 0x50→0x32 +50000us; unrelated pairs zero.
- Driver source/QA57 regression tests, build 1.94.100, no hardware IO during CT700 suite.
- Packaged Arch ABI stable, new qa58 package checksum, QA57 verified rollback SHA e0d5fde84daa42a89936076050781b455e483e7425b699ed9b5c0d3270aa7a16 (subject to live verification).
- Real user test LOCK, natural genuine deep S3 one physical finger press (first-touch and fprintd logs), then negative/unenrolled finger and S4 before production.
- Keep branch PR draft until certified. Note fprintd MR259 is a separate dormant PR and **not deployed** (stock CachyOS 1.94.5-2.1).
