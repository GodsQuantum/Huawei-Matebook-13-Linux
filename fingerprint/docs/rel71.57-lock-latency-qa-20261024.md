# rel71.57 — read-only cleanup timing research; no speed claim

This branch starts from the known-good rel71.55 certification branch.
It does **NOT** import the rejected rel71.56 S3 fast-path. The real
rel71.55 remains installed on Pegasus.

## LOCK post-rollback evidence, Oct 24, 2026

- LOCK #1: 15:21:10.733 finger detected; 15:21:11.633 score15/7;
  15:21:12.443 successful terminal; user one pose.
- LOCK #2: KDE PAM worker 15:24:59.950; FAST_READY 15:25:01.270
  (72ms sensor readiness alone); stale ImageBase 566709 ms;
  warm rebase started then discarded after the user touched at
  15:25:02.359, ~1.06s spent on a discarded operation.
  Finger detected 15:25:02.751; scored 7/7 at 15:25:03.680;
  cleanup completed at 15:25:04.383; successful terminal
  15:25:04.505. Owner confirmed ONE pose but perceived the
  greeter as slow/waiting for finger lift. It actually completes
  without physical release; cleanup and signal timing explain the UI.
- Under the exact firmware capture recipe, cleanup is
  FDT_UP -> GET_IMAGE (AES-GCM record authenticated) -> NAV ->
  FDT_POST_DOWN. Cannot skip this mandatory chain or output an
  authentication success while SPI/TLS is still being mutated.

## Instrumentation

When the optional fprintd env GXFP_TIMING_DIAGNOSTICS=1 is present,
emit per-cleanup-step command byte, transport/TLS time, command-to-command
configured gap time, and total cleanup time. No image, packet payload,
PMK or other biometric data is logged. If unset, protocol ordering,
gap length, number of commands and security decisions are unchanged.
No timers, polling, daemons or external helpers.

Do not deploy as an assumed speedup. Source and build verification
first; real hardware latency tests required before future tuning.
