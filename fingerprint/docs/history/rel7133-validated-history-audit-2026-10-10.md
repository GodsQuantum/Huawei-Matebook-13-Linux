# GXFP51A0 release-history audit — 2026-10-10 — rel71.33 experiment

## Scope and evidence

Compared 99 versioned Markdown handoff/research documents in Pegasus canonical
project, original archived source tree and pre-sanitization full-history Git
bundle. Compared actual old Git branch sources (rel59, rel61, rel66,
rel69, rel71.20, rel71.21, rel71.22, rel71.24), current public main, and
experimental rel71.31–71.33. The archived Git bundle remains private.
This report is not a claim that all historical tests were repeatable.

## Actual successes and limits

| Release | Observed human test | Conditions and limitations |
|---|---|---|
| rel40 / 44 / 45 | Several normal cold logins PASS | WakeupMCU and clean baseline; not recurring S3 proof |
| rel48 / 49 | Cold login 23/7; deep S3 11/7 | Safe GET_IMAGE retry only on no accepted ACK; good cold baseline |
| rel56 | Clean reboot pass 7/7 | Deep S3 immediately degraded quality to <=4/7 |
| rel59 | Deep S3 accepted 8/7 after new Identify cycle | ~7.8 s READY; initial 3 physical poses scored 3–4/7; a fresh WakeupMCU preceded successful 8/7 |
| rel60 | Normal lock 9/7 | MCU rearm after failed full pose |
| rel61 | Deep S3 PASS 20/7 | Full cold reset, DriverState, firmware A8, TLS, fresh baseline, KDE PAM rearm, ~21.5 s READY |
| rel62–64 | Not stable after S3 | Over-optimized fast resume caused 3–4/7 |
| rel65 | S3 FAIL | Closed FpDevice meant driver suspend callback absent |
| rel66 | Conditional deep-S3 PASS 7/7 | Only when Windows 0x60/01 00 sleep was acknowledged before S3; ~4.434 s READY. Later runs failed |
| rel68–70 | S3/normal regressions | Sleep command on every ordinary Close, stale long-lived fprintd lifecycle or inefficient daemon hook |
| rel71.18 | Deep TLS lock recovered; 3 normal locks 8/11/12 vs 7 | One-time pre-enumeration target-only spidev unbind/rebind and GPIO reset before fprintd, warm FAST_READY ~82–83 ms; external prestart discarded in native-only architecture |
| rel71.20 / 71.22 | Proven cold-prep baseline | rel71.22 restored rel71.20 driver unchanged; package orchestration boot prewarm, now forbidden as external additional service |
| rel71.21 | Rejected | Changed every protocol-defined GPIO reset to close/reset/reopen; severe TLS digest failures; NEVER repeat |
| rel71.23 | Failed deep TLS recovery | Longer detached quiet timings were not enough |
| rel71.24 | One S3 transport success, later 3/3 normal locks | Native threshold 7, short GPIO pulse in external prestart; one later true deep S3 failed transport |
| rel71.30 | Good normal matcher scores | Later S3/hibernate TLS and image failures; not proven superior to 71.24 under matched conditions |
| rel71.31 | Native 0x60 S3 park ACK on Oct 10 03:07:41 | True deep S3 until 03:07:46; five TLS failures and >50s until warm READY; not functional unlock |
| rel71.32 | Software-only preliminary | 1-second SPI-closed quiet before and after first cold GPIO reset, compiled but not physically tested |

### Distinct kernel A/B variable

Oct 3 archive documents byte-identical rel71/rel61 libfprint:
kernel 7.2.8-2 produced 3–4/7; after return to kernel 7.2.8-1,
ordinary capture met 7/7 and S3 subsequently reached 15/7. Current Pegasus
kernel 7.2.9-1. This is **evidence of hardware/host-controller sensitivity**,
not proof that one particular Clang/SPI driver regression explains 7.2.9.
Do not silently change the kernel, mounts, controller power/control or GPU.

## Correct separation of two reset classes

A. Goodix Windows protocol resets (post-PMK staging, TLS/config flow):
host SPI continuity is **intentional** and validated by rel71.20. The
rel71.21 universal-detach experiment was rejected. Do not replace this path
with close/reset/reopen.

B. Whole-session recovery after a confirmed S3/TLS desynchronization:
the outer recovery wrapper closes SPI+IRQ, resets GPIO264 and reopens before
checking the exact firmware/A8 marker. This is a different boundary and must
preserve the validated PMK cache and original biometric threshold.

### rel71.33 narrow change to class B

On native idle system suspend, libfprint sends 0x60 sleep once (and requires
ACK). It records that the NEXT TLS handshake is post-S3. If this FIRST TLS
handshake fails, it does NOT make four more retries on the same degraded
session and does NOT declare that the cached PMK is invalid. Instead,
control returns to the existing outer recovery wrapper, which closes the
transport, waits bounded quiet time, resets GPIO264, reopens, confirms
GF_ST411SEC_APP_14115, then retries the complete cold preparation normally.
On success the S3 marker is cleared. The non-S3 TLS/PMK/Windows protocol
sequence and all matcher thresholds/capture logic remain intact.

This is a **latency/recovery experiment**, not proven hardware robustness.
Software builds and source tests do not substitute for real physical
post-S3, hibernation, cold-boot, and same-finger/wrong-finger validation.
No extra systemd unit, timer, custom sleep hook or external helper.

## Evidence sources in canonical Pegasus project

- private/session-archive/2026-10-07-pre-main-sync/fingerprint/handoff/
  HANDOFF_ACCUMULATED_BEFORE_REL7123_TRANSFER_2026-10-06.md
  (rel59 exact sequence, 3 weak poses then new Identify 8/7)
- private/session-archive/2026-10-07-pre-main-sync/fingerprint/handoff/
  HANDOFF_2026-09-30_REL66_WINDOWS_DEACTIVATE_SLEEP.md
- private/session-archive/2026-10-07-pre-main-sync/fingerprint/handoff/
  HANDOFF_2026-10-03_KERNEL_72_8_AB_RESET_BASELINE.md
- repo/huawei-matebook-13-linux/fingerprint/docs/history/candidate-71.22.md
- handoff/history/HANDOFF_REL72_FULL_2026-10-01.md
- private/git-history-pre-public-sanitize-2026-10-07/full-history.bundle
- Pegasus persistent journal: 2026-10-10 03:07:41-03:08:43 S3 failure;
  2026-10-10 11:08:26-11:08:49 cold boot CALIBRATION finger already present,
  then sensor warm READY after driver ran

## Acceptance and rollback

Before promoting any candidate: native libfprint build, full
fingerprint/research tests, Arch build/package contents audit, backup old
package, physically verify 3 normal KDE unlocks (>=7/7), cold login,
one real deep S3 after a normal success, follow-up repeat S3, hibernate,
then Bitwarden Polkit prompt (PIN preserved). Never delete templates/PMK,
lower match threshold, export biometric images or invent a success.

## Follow-up: rel71.34 experiment, October 10, 2026

- KDE PowerDevil automatically requested S3 at 11:36:48 and 11:47:27.
  First failed to freeze Bitwarden (task in uninterruptible disk I/O) then
  xHCI blocked a fallback s2idle; second entered real deep S3 and returned
  at 11:57:48.
- rel71.33 reported NATIVE_S3_PARK idle=1 ack=0 twice, but this may mean
  the sleep command was NOT attempted: it only transmitted 0x60 if the
  existing libfprint object had complete warm context. The log lacked
  an attempted field.
- After S3 rel71.33 returned to whole-context recovery on first TLS error,
  but then cleared the recovery marker after mere firmware A8 response.
  Its next TLS attempt tried five times, repeatedly failing, and Claim
  never reached READY.
- One-time DIAGNOSTIC archived recovery with stock fprintd STOPPED
  (unbind target spi-GXFP51A0:00; quiet 1 sec; GPIO264 HIGH10ms/LOW;
  quiet 2 sec; rebind target SPI child; quiet 1 sec) reported READY.
  This is not an installed helper or a service, only a one-time experiment.
  The following standard fprintd own-user DBus Claim reached normal
  production_ready=1 at 12:04:34: authentic TLS + FDT restored without
  capturing or matching any finger. The old recovery can work.
- rel71.34 changes only idle S3 sleep to ATTEMPT the raw 0x60 opcode even
  without warm host state, recording attempted/ack/warm independently.
  Post-S3 TLS-fast-recovery flag is cleared only after complete
  production-ready TLS+background+FDT, not when FW A8 first answers.
  Normal non-S3 TLS resets untouched. Full software build and all project
  tests PASS; Arch native-only package PASS, no auxiliary GXFP units.
- Installed on Pegasus ~12:10. The native own-user DBus Claim (no
  fingerprint capture or user finger) then FAILED: TLS digest failures and
  an accepted GET_IMAGE timeout. Retried exact archived target-only offline
  unbind/GPIO/rebind and restarted stock fprintd; this time a second native
  DBus Claim ~12:11 still FAILED in TLS. Thus historical recovery is not
  universally reliable, and rel71.34 is NOT a confirmed S3 fix.
- Likely remaining boundary problem: host spidev/kernel state must be
  reset at a safe native device-enumeration lifecycle point, NOT by
  unbinding under an active libfprint FpDevice (udev hot-removal risk).
  Do not repeat failed rel71.21 approach of closing SPI during every
  Goodix protocol reset. Investigate FpContext pre-enumeration or proper
  kernel PM before adding further code. No extra daemon, timer or hook.
- No changes to kernel, BIOS, GPU, mounts, PCI PM, enrollment templates,
  PMK, Bitwarden PIN or other authentication services.
