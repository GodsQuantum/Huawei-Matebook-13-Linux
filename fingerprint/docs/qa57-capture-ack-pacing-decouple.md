# QA57 — protocol ACK / biometric capture pacing decoupling

Source baseline: QA54 on GitHub main, distinct from QA55 cancellation and QA56 200ms FDT report window. This is an opt-in experiment only.

## Hardware evidence

Pegasus 2026-11-01 QA56 runtime: control-plane misses inflated timing_scale from 100% to 300%; capture spacing floor reached 250% or 75ms. Image frame ~900–923ms versus ~573ms in the early QA56 first-touch LOCK. These results are correlated, not a controlled causal A/B comparison. Two S3 post-image cleanup 0x32 exchanges had only ACK (1/2) even with 200ms wait. Longer timeout did not help.

## Implementation

GXFP_DECOUPLE_ACK_PACING_QA=1 (default off) removes the protocol timing scale-derived minimum gap from both cold context initialization and successful image retry adaptation. Actual image transport retries, capture desynchronization, clean-streak decay and their own local capture_gap_scale remain unaltered. Control ACK timeout handling, strict FDT proof, frame acquisition, TLS, biometric threshold 7, existing rotation rescue, PAM, S3 and enrollment are unchanged. The canonical QA54 100ms FDT window is kept.

## Hardware gate

Build and local C harness and regression suite before installation. Validate each LOCK/S3 actual journal vs QA54 record 1157.858ms, QA56 1193.204ms, success rates, cleanup, wake reliability and end-to-end latency. A build pass is not equivalent to hardware validation. Preserve SHA256-verified QA54 package and enrollment; rollback immediately upon regression. Never accept FDT ACK alone or relax threshold.
