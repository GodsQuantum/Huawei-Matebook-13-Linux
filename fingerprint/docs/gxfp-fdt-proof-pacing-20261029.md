# GXFP51A0 rel71.74 — FDT proof failure and capture pacing (QA, 29 October 2026)

## Evidence and scope

- Independent MateBook 13 2020 ST411 / rel71.74 report (issue #6, 2026-10-29): 7/10 incomplete two-response FDT-down proofs (ACK=1 and replies=1), repeated session reconstruction costing 3.5–6.5s on next claim. This does not establish whether the second packet is late, missing, or misclassified.
- Pegasus MateBook 13 2021, PR #40 opt-in 250-ms / extra 0x36 validation: six consecutive physical press detections, each with `FDT_DOWN_PROOF incomplete ... replies=1 expected=2 ack=1`, **zero image captures**, capture pacing 100% -> 300%. QA40 must not be promoted to default for Pegasus. The exact packet root cause remains unknown.
- Reference Pegasus rel71.74 restored and independently verified; physical post-rollback LOCK still requires operator observation.

## Minimal source change and constraints

Fork **directly from rel71.74 main `bacbcc3`**, not the experimental PR #39/#40. For the existing strict FDT proof failure path only, run the existing bounded `gx_capture_transport_desync()` with temporary `capture_pacing_suppressed=TRUE` and restore the previous flag immediately. This preserves all session recovery semantics while preventing a control-plane response mismatch from being counted as a too-short image-capture SPI gap.

No change to 100-ms FDT budget; no acceptance of ACK-only as capture proof, no 0x36 special case, no TLS-GCM, threshold7, PAM, enrollment, GET_IMAGE replay, firmware, S3/S4 or image-processing changes. Generic image transport desync still raises capture pacing 50 percentage points; a previously suppressed lifecycle call remains suppressed. Test compiles **the actual modified driver C branches** and checks repeated FDT failures, unrelated image transport error, inherited suppression, and max-scale behavior.

## Validation gate

- Local test: `python3 fingerprint/research/tests/test_fdt_proof_pacing_behavior.py`.
- Full native/regression: `make -C fingerprint verify` and source manifest + GitHub CI.
- **Do not deploy/promote** without a real operator LOCK trial of rel71.74 recovery, and then a separate experimental A/B if needed. This patch avoids pointless slowing but does not make a missing FDT response appear; a repeated `images=0` still fails safely.
- Existing rel71.74 rollback package on Pegasus is `/var/lib/gxfp51a0-qa-rollback/rel71.74-held-finger-QA.pkg.tar.zst` with SHA256 `20c9ce0ee5548c033cba7619076a0ad16b8bf77e635b41a4fb1881407a27ab09`.
