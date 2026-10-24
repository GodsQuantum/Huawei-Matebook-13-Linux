# rel71.58 S3 stale ImageBase rebase (experimental, opt-in only)

QA `GXFP_S3_STALE_REBASE_QA=1` requires verified clean finger-OFF
ImageBase before rearming a new scored verification after a low-score
press with a stale (>20s) background. Prevents the 2026-10-24
19:06:12–29 repeat 2/7,2/7,2/7 after early finger on S3 wake.

Kept completely separate from rejected rel71.56 S3 fast preflight.
No change to matcher threshold=7, security, password fallback or the
default rel71.55 retry. New code uses existing cancellation-aware
FDT clear-stable checks, 650ms dwell, bounded 4000ms wait per pass,
and at most two full clean-background captures; no GPIO resets.
Any contamination requires a clean retry; no fresh reference => no
another scored Verify using stale background. A transport error
retains the established native session-recovery pathway. New runtime
behavior MUST be certified on Pegasus hardware with safe rel71.55
rollback available. This is not production-ready.
