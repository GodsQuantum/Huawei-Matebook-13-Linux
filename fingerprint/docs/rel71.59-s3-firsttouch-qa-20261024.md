# rel71.59 — first-touch deep-S3 QA recovery

Status: source-only QA, NOT hardware-certified, 2026-10-24.

## Evidence and defect

Pegasus 2026-10-24 20:15 genuine S3 with rel71.58: prewarm 7.49s,
first scored images 2/7, 3/7, 2/7, later successful 21/7
34.2 seconds after waking. Because background age was less than 20s,
the rel71.58 S3 stale-only rebase was bypassed: three biometric
attempts scored against a reference that looked fresh but gave
unreliable first S3 images.

Pegasus 21:32 genuine S3: first finger was quality rejected (contrast),
second contact scored 14/7. Quality failure ends that fprintd
Verify; rel71.58 did not retain a need for rebase into next Verify.

## New safe opt-in behavior

GXFP_S3_FIRSTTOUCH_QA=1 enables a one-shot guard ONLY after
native S3 resume. A first accepted positive match clears the guard.
The original first physical press, old rel71.55 hardware reset/
prewarm, and 7/7 threshold are unchanged.

If first press yields score below 7 or a no-score quality failure,
a dirty-reference suspicion remains across Verify transactions.
Before another scored press, force a truly new finger-OFF
ImageBase, even if the former reference is younger than 20 sec:
require 650ms stable FDT clear (bounded 4 sec), then native
crypto-validated background capture, with time-of-refresh
strictly greater than previous. At most two passes.
If unsuccessful, FAIL CLOSED and preserve password auth.
The native worker owns SPI and TLS; never introduce a timer,
hook or daemon. No GPIO112, PMK, firmware, kernel, PAM, GPU
or snapshot modification.

This prevents repeated scoring against a known-suspect ImageBase.
It does not prove first-touch or speed and is not a fix for early
physical finger lift; no lower threshold or best-of-N scoring.

## Research

GitHub issues #9 (Oct18 cold TLS/Claim timeout on 2020 unit) and
#10 (Oct20 active-Verify suspend leak) reviewed. Independent
Goodix Milan-SPI references from szlukabence and Sigfrodr
are compatible at the protocol level, not proof of identical MCU.
The active-Verify suspend and cold boot scenarios remain release gates.

Next: native build, regression tests, distro CI, package integrity
and supervised hardware LOCK / deep S3 / cold boot tests.

## 2026-10-24 Arch build and attempted Pegasus QA upgrade

- Native full regression and libfprint compilation: PASS.
- GitHub PR #21 (draft) all 12 requested CI checks PASSED at commit
  d4f8713 (only a public handoff path was redacted after ffedbd9);
  none of these software tests is a hardware first-touch certification.
- Isolated Arch base-devel package:
  libfprint-goodix51a0-1.94.100.goodix51a0-71.59-x86_64.pkg.tar.zst,
  SHA-256 d3ab9879c3b3203394ff9ff296092e926524346824e5f84c1af1d42a10f78397.
  The exact package hash was independently checked after staging to
  Pegasus /tmp/gxfp51a0-rel71.59-qa.pkg.tar.zst. Build/source remain in
  the CT700 developer workspace.
- Upgrade attempt at 22:47 CEST was NOT APPLIED. Native Pacman refused
  the transaction because a separate foreground arch-update process
  was running pacman -Syu on Pegasus with the database locked since
  22:22. DO NOT delete db.lck and DO NOT terminate or interfere with
  the ongoing user-managed system update.
- Reverted the brief systemd drop-in staging to the previous
  GXFP_S3_STALE_REBASE_QA=1 for rel71.58. Verified live fprintd PID
  environment, configured unit environment, active service and installed
  rel71.58 package all agree. No fprintd restart, KDE/PLM change,
  reboot, suspend or other hardware operation occurred.
- rel71.59 package remains STAGED but NOT INSTALLED. No hardware gate
  for rel71.59 can be counted yet; no Hall-of-Fame rel71.59 entry.
- Safe next step AFTER the existing user Pacman transaction completes:
  verify source and package checksum, package manager unlocked, no
  active kscreenlocker greeter, preserved rel71.55 rollback checksum,
  replace only the QA firsttouch override, install with native pacman,
  check package integrity, fprintd readiness and passive fingerprint
  discovery. If startup readiness fails, revert driver and override
  without reboot. Then request a single human physical LOCK test,
  followed by real S3 tests on this installed version only.
