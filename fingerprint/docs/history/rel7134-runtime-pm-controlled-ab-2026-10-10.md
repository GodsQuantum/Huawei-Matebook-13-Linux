# rel71.34 runtime-PM and native readiness investigation — 2026-10-10

**Status: physically tested normal initialization, NOT verified fingerprint match or post-S3.**
No biometric images, templates, PMKs or private system identifiers here.

## Current hardware problem
Deep S3 has produced repeated TLS digest failures on the Goodix GXFP51A0
ST411. One additional system suspend aborted due to a userspace
uninterruptible I/O task and USB xHCI error -16; separate from biometric
matching failure.

## Controlled reversible PCI runtime PM diagnostic
Previously: PCI LPSS SPI controller power/control=auto,
runtime_status=suspended. On rel71.34 an earlier own-user standard fprintd
D-Bus Claim FAILED to reach capture readiness. Exact one-shot historical
SPI child unbind/short GPIO264/rebind was tried with stock fprintd stopped;
it reported successful bind, but the subsequent native Claim also FAILED.
Thus this archived sequence is intermittent, NOT a guaranteed fix.

12:38 local:
- Temporarily set SPI PCI power/control=on, observed runtime_status=active.
- Standard own-user fprintd D-Bus Claim ONLY (no VerifyStart/no finger)
  returned success after one TLS image timeout and outer recovery;
  libfprint logged production_ready=1.
- PCI power/control restored immediately to its original auto setting
  using a shell exit trap.

12:39 local, fresh fprintd process, PCI still auto:
- Cold native own-user D-Bus Claim SUCCEEDED (after two transient TLS
  digest errors), approximately 13 seconds until return.

12:40 local, PCI still auto:
- Another Claim SUCCEEDED in 136ms wall-clock; libfprint reported
  FAST_READY in 82ms, FDT touch=0x00 mean=353 floor=329.
- The warm TLS/background state was preserved with stock fprintd active.

CONCLUSION: always-on PCI power is NOT necessary for successful cold
initialization. The one successful trial with power=on cannot prove an
improvement. Do not permanently force it on (battery cost).

## User physical gate
(1) With fprintd currently warm and untouched, user locks KDE session with
Super+L and unlocks once with enrolled right index when prompted; report
success and physical finger placement count.
(2) If and ONLY if normal fingerprint unlock succeeded, user initiates
KDE Mettre en veille, wakes normally, and tries right index again.
(3) After user report, read native logind S3, native S3 park attempt/ACK,
TLS, and real biometric match score. No automated suspend or reboot.

## Architecture constraint
Live FpContext owns enumerated SPI device objects. Unbinding spidev under an
active FpDevice risks udev hot-remove, invalidating libfprint object state.
Coordinate any future rebind before enumeration or with kernel PM, not during
a biometric capture. The rejected historical rel71.21 blanket protocol
reset close/reopen must remain rejected. No new helper/service/timer/hook,
no matcher relaxation, no PMK/template mutation.
