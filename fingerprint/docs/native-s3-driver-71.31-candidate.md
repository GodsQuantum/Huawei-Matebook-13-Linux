# GXFP51A0 native-only S3 candidate — rel71.31 (unvalidated hardware)

**Status:** research branch `fingerprint-native-s3-driver`; software build/test passes,
installed on Pegasus on 2026-10-10 at approx. 02:40 CEST; real KDE and S3 tests still PENDING. No public stable release.

## Why the 71.24 / 71.30 baselines failed after sleep

The sensor may enter deep S3 with an idle/closed fprintd device. fprintd already
has a standard logind sleep-delay inhibitor and invokes libfprint suspend for
every reader. However libfprint v1.94.100 bypasses a driver's suspend handler
when its current action is NONE. On this ST411, the warm TLS session is then
lost during deep S3; the next Claim may fail in a long TLS/digest-error chain.

Real evidence: rel71.24 awake KDE scores 7, 11, 2, 10 vs threshold 7 (3/3
unlocks), then a 2026-10-10 01:57 deep S3 failed before a biometric image.
rel71.30 likewise achieved successful awake scans and some cold/resume scans,
but failed later deep-S3 and hibernation TLS. Do not rank the matcher based on
different hardware-lifecycle conditions.

## Native library solution

1. The libfprint `fpi_device_suspend/resume` idle-action path now dispatches
   to the existing GXFP51A0 driver callbacks; all other readers keep upstream
   semantics. No fprintd daemon patch, background service or sleep hook.
2. Only on an actual suspend, and only with idle closed SPI descriptors and an
   available warm context, Goodix opens SPI, sends the documented ST411
   Windows command `0x60 / payload 01 00`, checks ACK, and closes transport.
   Never send it on ordinary Close; rel68/69 proved that harmful.
3. On suspend/resume, host TLS/background state is invalidated. The next
   standard Claim/Open uses the existing cold preparation. Do not replay a
   GET_IMAGE with accepted ACK and lost TLS response.
4. The candidate Arch package removes standalone boot-prewarm and
   prestart-recover executables/systemd units. fprintd remains the stock
   required fingerprint daemon; it is configured resident with --no-timeout.
   The portable and Arch install scripts are correspondingly revised.

## Safety and remaining test gates

No changes to matcher threshold 7, existing enrolled templates, PMK files,
other kernel devices, GPIO112, mounts, or global system authentication.
Compilation alone cannot show that MCU SLEEP 0x60 gets an ACK on actual S3 or
that the resulting cold Claim is robust. In particular, the historical
spidev-unbind preflight sometimes recovered a persistent TLS failure; its
removal needs real cold-boot and repeated deep-S3 checks.

PASS (Cloud9 LXC700): libfprint v1.94.100 native build, source manifest,
full fingerprint/research suite, target sleep packet byte test, no release
biometric dump, staged library with no new service/helper binaries.

PASS: native Arch base-devel makepkg; package SHA256 d7919c6fe6fbef1d4bc3a1027eb83a086d4a11072ca97cd7569efe62b29e020f; installed on Pegasus, fprintd active, right-index enrollment retained, no extra GXFP service.

PENDING: real PEGASUS KDE lock/unlock
before and after deep S3 and hibernation; cold boot; wrong-finger control;
Bitwarden's actual Polkit fingerprint prompt. User, not the assistant,
triggers any actual sleep/reboot. The archived rel71.24 Arch package remains available for immediate rollback if the new candidate fails the normal or S3 gate.

## Native cold-open quiescence experiment (rel71.32, not proven)
After post-package restart at 02:48, first Claim took approx. 31 seconds and encountered repeated TLS digest failures; no later S3 was registered by logind since the package upgrade. A separate candidate inserts a bounded 1 second pre-reset and 1 second post-reset bus quiet period within the native cold-Claim path, with SPI descriptors closed, but does NOT touch sysfs binding nor reinstall external units. This addresses first-Claim quiescence, not proof of repeated S3 reliability.

## Field report, 2026-10-10 02:48 CEST

The user reported fingerprint failure after sleep; host systemd/logind journal showed no new suspend operation after rel71.31 installation at 02:39:44. The last actual suspend ended 01:57:42, before the package update. Thus native S3 callback remains untested against a new S3 event.

At 02:48 fprintd PID 142304 first-Claim native GPIO cold reset was followed by many ACK failures and TLS digest check failures at 02:48:05, 02:48:12, 02:48:19. At 02:48:23 TLS/FDT was viable but an already-pressed finger contaminated clean background acquisition. The sensor retained warm state at 02:48:32. Cold first-Claim recovery is unreliable without earlier pre-enumeration/prewarm, independent of actual new S3.

## rel71.32 code-only candidate (NOT deployed)

Adds a bounded one-second pre-reset and one-second post-reset SPI-closed quiescence within the native driver cold Claim. No extra service, hook, timer or background worker; no sysfs device unbind from a live FpDevice. Software build PASS and complete fingerprint/research test suite PASS in Cloud9 LXC700. This has not been physically validated and must not be considered a proven S3 fix. Keep rel71.31 installed until an actual post-package S3 trace can be inspected.
