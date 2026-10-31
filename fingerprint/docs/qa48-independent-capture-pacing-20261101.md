# QA48 — Do not infer slow image capture from recoverable FDT retries

Status: **LAB-ONLY; default OFF**. Based on QA42 branch, not merged,
not installed on Pegasus. Keep the byte-identical QA42 + KDE47
(first-touch no-mouse hardware success) untouched.

## Actual causal evidence and prior configurations

- Pegasus after full reboot 2026-10-31: first eligible finger image
  at 21:38 had frame584ms, match65ms, cleanup570ms and one physical
  press accepted.
- While boot greeter was open and there was no finger contact, the
  native fprintd verification cycled about every 30s. The repeated,
  **recoverable** one-time FDT-manual ACK miss during background refresh
  auto-raised protocol timing 100 -> 150 -> 200 -> 250 -> 300%.
  The device still reported a checksum-valid reference and no actual
  GET_IMAGE/TLS transport loss.
- Subsequent good S3, LOCK and hidden-prompt LOCK at 23:12, 23:17,
  23:43 and 00:00 showed image frame909,902,903,921ms; their captures
  scored 7-16/7 and succeeded. Their 0x32 FDT ACK/data replies arrived
  successfully, mostly 61-62ms. A mean frame difference of ~320ms
  from first 584ms is associated with slower effective capture
  pacing; the exact scale per frame is not logged by default.
  **Correlation, not proved hardware cause**, because sleep and
  frame content can also affect capture time.
- Existing gx_cold_prepare() seeds capture gap scale from protocol
  timing at max(protocol-50, 100), so a 300% protocol timing
  translates to 250% capture gap at cold preparation, absent direct
  image retry. gx_capture_pacing_success() also separately learns
  from real GET_IMAGE/TLS retry and decays on complete clean frames.
- Independent 2020 ST411 report issue #6: rel71.30 50 sessions
  with exactly one FDT-ACK retry per clean background and **zero**
  GET_IMAGE/TLS losses; rel71.74 captures mostly ~573-601ms when no
  real desync. Do not equate ACK probe noise and capture pacing.
- Hall-of-Fame fastest valid reference LOCK = 1.181s QA42,
  capture572ms + matcher57ms + cleanup548ms. Rel71.71 had
  LOCK1.254s capture571ms but real S3 failed first pose, so earlier
  timing is not a blanket better driver. Different warm/cold cases
  are not directly comparable.

## Narrow experiment

Opt-in **GXFP_CAPTURE_GAP_FDT_INDEPENDENT_QA=1** suppresses ONLY the
automatic _cold preparation_ capture-gap seed inferred from ordinary
FDT protocol timing. It does not decrease an already learned
capture_gap_scale, and a *real* GET_IMAGE/TLS retry still calibrates
capture via gx_capture_pacing_success() and full bounded recovery.
This does NOT change FDT retries, reset, TLS-GCM, the 1000ms image ACK,
the 100ms typed 0x32 FDT proof, session safety, ImageBase 20s,
image-quality thresholds, matcher threshold7 or native KDE/PAM.
With the flag unset, exact original behavior remains.

Expected potential speed gain is from avoiding artificially
inflated 30ms gaps, NOT measured yet: no new hardware run has
occurred. Must A/B after native S3 and plain LOCK with the same
successful finger, log actual capture_gap and GET_IMAGE retries,
fail-closed recovery and first physical placement count. A software
PASS does not mean faster on the device.

## Separate security finding (do not change threshold)

Sigfrodr/libfprint-goodixtls latest matcher commit
17354ac441a2e252ea56b1ee44d4622eb086e6a7 illustrates
higher impostor scores with larger fused galleries and moves its
**different** matcher from fixed15 to size-dependent 15/18/20.
On the real GXFP51A0 SIGFM matcher in issue Sigfrodr #5,
one 2020 user measured 123 genuine, 1089 impostor comparisons,
EER ~4.7% with 20-view templates; impostor max6 vs our strict7.
This is NOT population FAR=0; don't lower thresholds and don't
copy those SIFT score values. The repository already has a local
aggregate-only real matcher eval plug-in under research/eval and
source/ABI tests. Reuse it for held-out genuine/impostor evaluation
with more subjects rather than storing/disclosing biometric payloads.
