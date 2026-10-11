# rel71.38 — native pre-greeter readiness / full recovery, 2026-10-11

**EXPERIMENTAL field candidate.** Verified Pegasus kernel 7.2.9-2 and
native fprintd startup/warm first-Claim tests. Cold physical boot login,
human lock and post-S3 on this revision are PENDING. Do not claim stable.

## Why 83 ms had been lost

- Original rel23 tag fingerprint-gxfp51a0-rel23 populated a TLS/background/FDT
  warm context in libfprint gx_dev_probe during enumeration, before first PAM
  authentication. Subsequent Claim reuse was around 100-120 ms, not raw
  physical cold bootstrap time.
- The old rel24/25 probe path was rejected because in-protocol reset while
  SPI was open and abbreviated TLS/GET_IMAGE failure handling poisoned
  early sessions. Later rel71.36 made probe passive, moved whole
  preparation to first user Claim and kept warm FAST_READY in 82-83ms.
  Pegasus needed 7-16s (and occasionally longer) BEFORE first Claim could
  authenticate at a visible login screen.
- Source-verified fprintd 1.94.5 creates FprintManager and synchronously
  enumerates libfprint devices BEFORE owning its system D-Bus name.
  Pegasus already has native fprintd Before=plasmalogin.service dependency.
  Therefore sensor preparation during probe can finish BEFORE PLM appears
  and fprintd DBus clients see prewarmed TLS/background/FDT.

## Rejected rel71.37 (important)

First attempted a shortened probe with 2 TLS retries per prewarm and two
separate attempts. On Pegasus 2026-10-11 13:26 both probes failed GET_IMAGE
TLS-image timeouts; global fprintd TimeoutStartSec=15s killed the daemon,
leaving no usable fingerprint. Immediate full rollback to proven rel71.36
was executed; fprintd restored and biometric enrollment unchanged.
Reject that candidate permanently. Source diff + logs are retained in
the private history archive, not in stable GitHub main.

## Final rel71.38 implementation

- In existing native libfprint GXFP51A0 gx_dev_probe, do ONE bounded, FULL
  gx_cold_prepare on an opened SPI descriptor, preceded by
  SPI/IRQ close -> one-second quiet -> GPIO264 reviewed cold reset ->
  one-second quiet -> open. The FULL rel71.36 routine handles actual
  TLS/PMK staging and full detached GET_IMAGE recovery. No simplified
  TLS retry sequence, no loss of validated root-only PMK cache.
- On a successfully prepared probe, retain TLS, ImageBase/background and
  FDT data in same FpDevice across transport close. Existing
  gx_dev_open -> gx_warm_fast_ready returns in ~82-83ms.
- If prewarm fails after its bounded full recovery, keep FpDevice
  discoverable and permit the existing Claim/open cold fallback. No
  biometric threshold change or false success.
- Existing shipped fprintd.service drop-in now explicitly sets
  TimeoutStartSec=90s ONLY for this daemon, because this Pegasus machine
  otherwise imposes a global 15-second timeout shorter than a normal
  whole-session Goodix recovery. No new unit, daemon, timer, helper,
  sleep hook, sysfs unbind or kernel/PCI PM change. Cross-distro
  install-linux.sh uses the same limit.
- The existing PLM 6.7.5-3.10 renders passive "Initialisation..." until
  pam_fprintd says "Placez votre index droit". Password login remains
  independent. No QML changes in rel71.38.
- Existing rel71.36 native deep-S3 0x60 park, detached GPIO short reset,
  historical rel61 TLS retry phase and fresh-background guards are
  unchanged. Matcher requires score >=7, enrolled right-index unchanged.

## Real tests 2026-10-11, running Pegasus kernel 7.2.9-2

- Native pinned libfprint v1.94.100 source build PASS and full
  fingerprint/research test suite PASS.
- Isolated ArchLinux makepkg PASS and actual package inspection PASS.
  Native libfprint + original fprintd unit/drop-in only; no extra GXFP
  custom unit, timer, helper or hook.
- Installed rel71.38 on Pegasus; Snapper root pre/post 1565/1566.
  Existing enrollment retained. fprintd active; existing PLM active.
- Three controlled fprintd service restarts followed IMMEDIATELY by
  own-user D-Bus Claim (no VerifyStart / no fingerprint image):

  | Native startup barrier | First D-Bus Claim | Result |
  |---:|---:|---|
  | 7,435 ms | 106 ms | PASS |
  | 14,411 ms | 107 ms | PASS |
  | 15,553 ms | 107 ms | PASS |

- Driver FAST_READY recorded 82-83ms, live warm session retained.
  These are the original-fast-path-class results but are not cold boot
  fingerprint-login proof. At boot, PLM must be blocked behind the
  successful readiness barrier, not user password access in an already
  shown greeter. Boot physical gate still outstanding.

## Future acceptance and safe fallback

1. User-initiated full reboot into the same 7.2.9-2 kernel. Observe PLM
   appearance, actual immediate right-index fingerprint acceptance on
   FIRST placement, no false "ready" prompt. Password remains usable.
2. Standard KDE Super+L, right index first-pose accepted score >=7.
3. Manual real ACPI deep S3, wake, right-index physical score >=7.
   Do not mistake a PAM cue for hardware readiness if it is still
   calibrating; check journald TLS/FDT/MATCH and unlock elapsed seconds.
4. If any gate fails, preserve rel71.36 exact package for rollback,
   diagnose logs first and do not merge or tag stable main.
5. Never automatically reboot, log out or suspend user's machine, erase
   enrolled templates/cache/PIN or change established mounts.

## Historical preservation

Canonically indexed 226 original Markdown/text/log records plus the
original 126-reference private Git bundle under the project's private
research/history-2026-10-11 workspace. File SHA256 index, headings index,
version evidence matrix, original bundle ref inventory and a private tar.gz
lossless text-history snapshot are all retained. These PRIVATE files
must never be pushed to public GitHub. Stable main is untouched until
physical end-to-end verification.

## Additional same-boot field evidence (2026-10-11 14:57 CEST)

- Three native service starts + first own-user D-Bus Claims:
  7435ms + 106ms, 14411ms + 107ms, 15553ms + 107ms;
  native FAST_READY 82-83ms.
- Existing fprintd Before=plasmalogin ordering and PLM After=fprintd
  verified. Per systemd Type=dbus semantics, unit readiness means D-Bus
  BusName acquired; native fprintd/libfprint enumeration finishes first.
  References: https://fprint.freedesktop.org/libfprint-dev/FpContext.html
  and https://www.freedesktop.org/software/systemd/man/latest/systemd.service.html
- Pacman integrity on installed libfprint, PLM and fprintd:
  32/210/270 files respectively, zero modified files.
  Right-index enrollment retained. No new systemd fingerprint units.
- Another host power transition at 14:06-14:28 was specifically ACPI
  **S4 hibernation**, NOT deep S3 (kernel journal):
  S4 resumed 14:28:49, native short GPIO reset 14:28:51,
  post-resume VERIFY_READY 14:28:56. NO accepted biometric match was
  recorded then, so do not mislabel it as a successful S3 unlock.
  The kernel S4 state is described at
  https://cdn.kernel.org/doc/html/latest/admin-guide/pm/sleep-states.html
- The canonical PRIVATE project archive contains whole-project snapshot
  plus SHA256, 226 indexed records and original 126-ref Git history
  bundle, an isolated experimental source clone, and protected original
  main edits. No private files uploaded to GitHub.
- Real rel71.38 COLD BOOT fingerprint LOGIN, ordinary lock and genuine
  S3 wake with >=7/7 matcher score remain physically UNTESTED.
  Do not promote or tag public stable main until all are proven.
