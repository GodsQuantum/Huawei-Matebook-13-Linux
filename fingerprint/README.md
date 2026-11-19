**Safety update — 19 November 2026.** After a deep-S3 lockout affecting both fingerprint and password, the optional KDE QML v12 is quarantined and Pegasus runs stock Plasma 6.8.3 QML. The greeter remembers `Regular` (password) to avoid KDE crash #528285. `Fingerprint=false` hides the selectable fingerprint method while the parallel fingerprint worker can still run. The operator confirmed one password unlock after a kernel-confirmed deep-S3 resume. Repeatability remains unverified. [Incident report](docs/real-hardware-certification.md).

**Reliability changes — 19 November 2026.** QA63.2 protects calibration when the sensor is partly touched and waits for a confirmed lift before continuing a scored attempt. A separate [native KDE 6.8.3 candidate](integration/kscreenlocker-upstream-s3/README.md#plasma-683-native-readiness-candidate) retains early authentication requests and can restart the implicit fingerprint conversation after a timeout. The driver installer does not replace KDE. Fresh physical LOCK/S3 checks remain required; historical QA63 results do not certify this revision.

**Current source installer: QA63.2 (19 November 2026), Arch package QA63-3.**
**Historical v12 trials (18 November).** The then-active opt-in workaround
started fingerprint authentication without pointer movement and hid the
prompt. Several unlocks passed after right-index re-enrollment. One placement
scored below 7. Two kernel-confirmed deep-S3 cycles recovered fingerprint
unlock after repositioning. The three-cycle gate, first-touch S3 reliability,
suspend power use, cold boot and S4 remain unverified.

Earlier QA63 trials recorded three LOCK and two genuine deep-S3 first-placement successes. The ±18° unknown-finger, repeated S4, BOOT and REBOOT gates remain unverified. Keep the password fallback. The rel71.74 tag below remains the previous public release, not the current mainline installer.

**Historical tagged reference release: rel71.74 (28 October 2026).** The driver retains a proven clean ImageBase for an already-held finger after sleep, and preserves a TLS-GCM-authenticated positive score if later sensor cleanup fails while invalidating that transport for next use. LOCK succeeded at first physical touch; the operator reported S3 first-touch success. A more recent S3/S4 test was aborted *before suspend* because KDE and the Cloud9 NFS automount blocked the freezer. **Repeatable S4 and cross-device reliability are not established.** Keep password fallback. See [release evidence](docs/rel71.74-production-20261028.md).

**Historical rel71.69 fingerprint reliability warning (2026-10-27):** rel71.69 is now an **experimental prerelease**. An S4 hibernation resume on the reference machine required 38.69 s to prepare the sensor, and biometric login was unavailable. Previous isolated successful LOCK/S3/S4 trials do not certify reliable sleep, boot or first-touch login. See [confirmed issue #32](https://github.com/GodsQuantum/Huawei-Matebook-13-Linux/issues/32). Preserve password fallback; avoid depending on fingerprint alone.

# Goodix GXFP51A0 / GF3658 ST411 on Linux

> **Historical QA note (rel71.55, 23 October 2026; NOT current Pegasus state):** Native S3 cancellation fixes and KDE Plasma Login Manager 6.8.1 dual-PAM code have passed software builds, but **S3, reboot, and cold BOOT are NOT physically certified**. At that historical point Pegasus ran rel71.55 QA; Pegasus now uses QA63. The known-good rel71.49 driver and stock KDE 6.8.1 rollback are preserved. [Hardware certification gates](docs/real-hardware-certification.md).


Native libfprint driver and zero-to-working installer for the SPI Goodix
GXFP51A0 found in the Huawei MateBook 13 2021 family.

> Français: [README.FR.md](README.FR.md) · 简体中文: [README.ZH-CN.md](README.ZH-CN.md)

## Experimental reference — S4 reliability issue (historical) — rel71.69 (27 October 2026)

Native Linux/libfprint GXFP51A0 with verified 50 µs Chip Select setup. Three user-attested first-touch authentications on the reference MateBook 13 (LOCK, hibernation S4 resume and genuine deep-S3 resume); S3 wake-to-positive 8.218 s, finger-to-positive 2.054 s, score 15/7. Native fprintd and KDE/Polkit integration, password fallback, TLS-GCM, fixed matcher threshold7. Hardware evidence: [rel71.69](docs/rel71.69-hardware-final-20261027.md), [Hall of Fame](docs/HALL_OF_FAME.md). Not independently certified across machines, cold boots or repeated S3 cycles. Install from this repository's [driver-only installer](install.sh) or the standard Arch package.

Earlier rel71.30 and rel71.24 reference notes below are historical, not the current package release.

## Historical status — 2026-10-08

### Recommended preview: rel71.30 (reference-machine field test)

At that historical time rel71.30 was the default (superseded by QA63.2). Native KDE enrollment followed by eight successful scored verification captures on the reference MateBook 13 at threshold 7 (7, 10, 7, 13, 9, 11, 11, 19). No post-enrollment ACK/TLS errors were logged during that series. This is not a population-level false-acceptance estimate. rel71.24 is still the last fully cold-boot/deep-S3 validated rollback (rel71.18 older immutable reference).

### Changes in rel71.30

rel71.30 keeps the fixed authentication threshold at 7 and adds two narrowly scoped changes: a conservative per-view photometric rescue for genuine near misses, plus connected enrollment that builds a redundant five-view anchor before controlled coverage expansion. It deliberately does **not** restore the rejected multi-view score fusion.

GUI re-enrollment and routine KDE lock/unlock have been successful on the reference machine. A user-reported **unenrolled right-middle rejection followed by immediate right-index acceptance** was also observed as scores 2/3/4 rejected then 16 accepted (threshold 7) in the reference machine's fprintd logs. Broader wrong-finger testing, independent hardware, cold boot and deep-S3 remain necessary before full validation. See [`docs/candidate-71.30.md`](docs/candidate-71.30.md).

Hardware-validated target:

- ACPI HID: `GXFP51A0`
- Goodix GF3658 / ST411, chip ID `0x2504`
- validated firmware: `GF_ST411SEC_APP_14115`
- SPI mode 0 + `SPI_CS_HIGH`, 1 MHz
- GPIO48 readiness/IRQ and GPIO264 active-HIGH MCU reset
- TLS 1.2 `PSK-AES128-GCM-SHA256`
- 80×64 active fingerprint image
- libfprint base: pinned `v1.94.100`
- fixed SIGFM acceptance threshold: **7**
- template version 4; existing 20-view enrollments remain compatible
- no firmware flashing, periodic keepalive, persistent timing file or external suspend hook

Runtime path:

```text
GXFP51A0 → spidev → libfprint → fprintd → desktop fingerprint PAM / CLI
```

The rel71.24 recovery boundary is performed before libfprint enumerates the reader:

```text
fprintd start/restart
→ unbind only spi-GXFP51A0:00 from spidev
→ GPIO264 active-HIGH short pulse (~10 ms)
→ LOW + complete GPIO request release
→ detached settle
→ rebind only spi-GXFP51A0:00
→ udev settle
→ resident fprintd --no-timeout
```

A reader already stuck in repeated TLS digest failures recovered without reboot or power-cycle. Warm Claims remain around **82–83 ms FAST_READY**. Normal KDE locking and a manual deep-S3 suspend/resume were validated with existing PMK, firmware and template-v4 enrollments.

ImageBase freshness is separate from the long-lived TLS context: stale background state is refreshed before biometric READY while healthy warm Claims keep the fast FDT-only path.

The active biometric decision is conservative: fixed threshold 7, best single enrolled view, no score addition or fusion, and no score-conditioned same-placement retry. A usable image receives one score; a score below 7 requires lift/reposition. Up to three independent physical placements are allowed per driver action.

Full rationale and package hashes: [`docs/validated-checkpoint-71.24.md`](docs/validated-checkpoint-71.24.md).

The installer is idempotent: an exact current release is a no-op for the live sensor session. A real driver change must pass a fresh semantic prewarm (`PREWARM_RESULT=READY`) before success is reported.

## Driver-only distribution (no Huawei/GPU manager)

The [driver-only installation guide](DRIVER_ONLY.md) covers current source and the separately published rel71.74 assets. Older rel71.74 archives predate the optional guided wizard. Both use standard libfprint → fprintd → native Linux desktop interfaces and require no HUAWEI Whiptail manager, GPU tools or project-specific enrollment UI. Only the exact GXFP51A0/ST411 hardware profile is in scope; distribution and board differences require independent confirmation.

## Quick start: zero fingerprint support → working stack

Clone the repository and run one command as your normal user:

```bash
git clone https://github.com/GodsQuantum/huawei-matebook-13-linux.git
cd huawei-matebook-13-linux
./fingerprint/install.sh
```

The installer:

1. refuses unsupported hardware unless `GXFP51A0` is actually present;
2. installs the distro build/runtime dependencies;
3. builds the reviewed driver reproducibly;
4. validates the distribution `fprintd` ABI **before** system installation;
5. installs native udev/spidev integration and the fprintd runtime path;
6. uses the existing resident fprintd service and removes obsolete keepalive/external sleep glue;
7. uses the desktop's native fprintd/PAM integration by default; optional legacy Plasma integrations are never applied silently;
8. preserves password authentication in parallel;
9. preserves existing fingerprint templates and the validated PMK cache;
10. supports KDE/GNOME native settings and a separate optional guided UI, without automatically enrolling fingers;
11. finishes with `gxfp51a0-doctor`.

The installer deliberately refuses to enable `pam_fprintd` globally when doing
so would serialize password authentication behind a fingerprint timeout.
Desktop integration requires a separate fingerprint PAM path such as
`kde-fingerprint`, `plasmalogin-fingerprint` or `gdm-fingerprint`.

Useful modes:

```bash
./fingerprint/install.sh --build-only
./fingerprint/install.sh --no-install-deps
./fingerprint/install.sh --no-desktop-integration
./fingerprint/install.sh --no-enroll
./fingerprint/install.sh --no-verify
./fingerprint/install.sh --no-enroll
```

Arch/CachyOS can use the native package path directly:

```bash
./fingerprint/install-arch.sh
```

Diagnostics:

```bash
./fingerprint/gxfp51a0-doctor.sh
# portable installs also provide:
/usr/local/bin/gxfp51a0-doctor
```

Rollback for the portable source install:

```bash
sudo /var/lib/gxfp51a0-local-install/uninstall.sh
```

## Portability

The portable source/build/ABI path has passed in:

- Arch/CachyOS
- Debian stable
- Fedora 44
- openSUSE Tumbleweed
- Alpine edge / musl

The source is therefore distribution-portable across the tested package/libc
families. This does **not** mean every Goodix laptop is supported: the production
hardware profile is specifically the MateBook target described above.

## Desktop and PAM integration

`pam_fprintd` is serialized inside a single PAM stack. Password + fingerprint
can be truly concurrent only when the desktop launches separate authentication
processes/stacks. The production installer therefore:

- uses the validated separate KDE/Plasma fingerprint stack when available;
- accepts a separate GDM fingerprint PAM service when provided by the distro;
- never silently inserts `pam_fprintd` into a global password stack;
- always leaves password authentication available.

Historical Plasma 6.7.5 compatibility packages require the explicit
`--legacy-plasma-patches` option. The separate native KDE 6.8.3 candidate
is documented in [the native integration guide](integration/kscreenlocker-upstream-s3/README.md#plasma-683-native-readiness-candidate).
Neither path silently replaces the current desktop packages.

## Current package invariants

The Arch/CachyOS package contains the QA63.2 driver and a drop-in for the
existing native fprintd service. It does **not** package:

- a separate boot-prime helper or service;
- periodic warm-keepalive timers/services;
- external fprintd suspend services;
- resume-prewarm hooks.

Normal idle readiness is retained inside the resident fprintd/libfprint process
without an external polling service. S3 recovery stays inside the native
libfprint/fprintd lifecycle; desktop authentication is a separate boundary.

### Optional privacy-preserving matcher validation

Benjamin Allègre (Sigfrodr) published `tools/eval/fp_eval.py` in
`Sigfrodr/libfprint-goodixtls` as a local-only evaluator for the Milan-SPI
family. It reports aggregate statistics while raw captures/templates remain on
the tester machine. It is not a runtime dependency, and release builds contain
no biometric capture dump writer.

## Installation by distribution

Research from rel24–rel61 is retained in [historical research](docs/history/), the [research log](docs/research-log.md) and Git history. These notes are **not current installation instructions**. The current source installer selects QA63.2; rel71.30 and rel71.24 are historical preview and rollback references.

### Arch / CachyOS

From the repository root:

```bash
./fingerprint/install-arch.sh
```

The installer:

1. refuses to run if the `GXFP51A0` SPI/ACPI device is absent;
2. builds the reviewed libfprint patch locally;
3. installs `libfprint-goodix51a0` and `fprintd`;
4. grants fprintd only the additional gpiochip device access needed by this
   driver;
5. starts the existing native fprintd service through graphical.target;
6. keeps deep-resume recovery inside libfprint itself, with no external
   system-sleep hook or worker;
7. deliberately installs no periodic synthetic Claim keepalive;
8. leaves the current KDE QML and desktop packages unchanged by default;
9. exposes historical Plasma 6.7.5 packages only through the explicit
   `--legacy-plasma-patches` option;
10. removes obsolete development timing state, reloads udev and restarts
    fprintd without touching enrollments or the PMK cache.

Native desktop integration is preserved. The historical QML helper's ordinary
`--apply` is quarantined after the sleep lockout; it does not patch
`LockScreenUi.qml`. Its explicit experimental path is not a default install.

By default, enroll, remove and verify fingerprints through your desktop's native graphical settings (KDE Plasma **System Settings → Users**, GNOME **Settings → Users**). The installer never starts command-line biometric enrollment or verification.

The driver requests 20 enrollment presses. Move the finger slightly between
presses so the small 80×64 sensor sees different parts of the fingertip.

### Optional guided enrollment on any Linux desktop

The driver package includes the optional GTK3/PyGObject graphical application
`gxfp51a0-enroll` (menu entry **GXFP51A0 Guided Fingerprint Enrollment** or launch
`gxfp51a0-enroll`). It uses only native fprintd D-Bus. **KDE System Settings →
Users → Fingerprints still opens KDE's own GUI, never this separate wizard.** It suggests 20 gently varied positions (center, upper/lower,
left/right, small rotations, normal unlocking), advancing solely after accepted
driver stages. Suggested poses are not verified measurements of finger coverage.

Existing fingerprints are never removed or replaced by this application.
Only unenrolled fingers are selectable. To redo an existing finger, first
confirm that password login works, then delete only that finger with the
desktop's native fingerprint settings. The installer never launches enrollment.
Runtime UI packages: Python3, PyGObject, GTK3 (Arch: python, python-gobject,
gtk3; Debian/Ubuntu: python3, python3-gi, gir1.2-gtk-3.0).
No custom daemon, sleep hook, sensor image dump, or biometric threshold change.
Successful completion is not a hardware/security certification.

**Multiple fingers:** fprintd Identify compares a scan against **all enrolled
prints**; the driver scores every candidate and selects the best one above the
fixed threshold of 7, not merely the first enrolled finger. After adding a
finger, verify it with several independent lock/unlock attempts.

### Existing development templates

The current template format is driver v4 / SIGFM v3. If an older experimental enrollment cannot be recognized, remove **only that specific finger** through the native desktop **Users → Fingerprints** settings and enroll it again there. Do **not** bulk-delete existing fingerprints or use CLI enrollment/deletion. Fresh installs do not require this step.

### Debian / Ubuntu / Fedora / openSUSE / Alpine / other Linux

The portable source installer rebuilds the exact pinned libfprint candidate and
keeps the replacement isolated under `/usr/local`:

```bash
./fingerprint/install.sh
```

It installs build dependencies on Arch/CachyOS, Debian/Ubuntu, Fedora,
openSUSE and Alpine. Arch/CachyOS delegates to the native pacman package.
Elsewhere it stages the candidate, validates the distro fprintd ABI, then
isolates the local libfprint to fprintd via a systemd drop-in or D-Bus
activation wrapper. A rollback manifest is recorded.

Rollback after a source installation:

```bash
sudo /var/lib/gxfp51a0-local-install/uninstall.sh
```

Use `./fingerprint/install.sh --build-only` to validate compilation
without installing anything.

## Matcher

The production matcher uses:

- adaptive background subtraction for the exact target sensor;
- percentile normalization + unsharp enhancement;
- two-level multi-scale FAST-9 keypoints;
- unsteered BRIEF-256 descriptors;
- mutual-best cross-check + Lowe ratio filtering;
- 200-iteration rigid RANSAC with 2 px inlier tolerance;
- least-squares rigid refinement;
- best score across 20 enrolled views.

The driver does **not** lower the threshold after a failed attempt, sum weak
scores across attempts, or learn from failed/low-confidence verification.

A pixel-overlap/ZNCC scorer remains available only behind the explicit
`GXFP_MATCH_DIAGNOSTICS` research environment flag. It is not part of the
authentication decision.

## Contributor validation

```bash
make -C fingerprint verify
```

This runs the deterministic research/safety suite, validates the source
manifest, fetches exact libfprint `v1.94.100`, builds the candidate and checks
the resulting artifacts.

Release gates include:

```text
SOURCE_MANIFEST=PASS
LIBFPRINT_BUILD=PASS
GOODIX51A0_OBJECT_COMPILED=YES
GOODIX51A0_FASTBRIEF_RANSAC_IN_LIBRARY=YES
GOODIX51A0_IDENTIFY_PATH_IN_LIBRARY=YES
RELEASE_BIOMETRIC_DUMP_HOOK=ABSENT
SOFTWARE_BUILD_READY=YES
ACTIVE_SENSOR_IO=NONE
GPIO_WRITES=NONE
MMIO_WRITES=NONE
FIRMWARE_ACTIONS=NONE
```

The validation target performs no active sensor transfer, GPIO/MMIO write or
firmware action.

Optional local diagnostics for maintainers are under `fingerprint/tools/`.
Normal users do not need them.

## Safety and privacy

Never commit or publish:

- fingerprint captures or enrolled templates;
- PMK/PSK/key material or per-unit fixtures;
- proprietary Goodix/Huawei binaries or firmware;
- serial numbers or private machine identifiers.

The PMK cache is private runtime state under `/var/lib/fprint/` and is never
shipped in the package or repository. Production timing adaptation is
session-local in RAM; rel49 does not persist learned timing files.

The v4 local fprintd template is biometric data. It includes normalized
per-view information used by the matcher/research diagnostics and should be
protected like any other fingerprint template.

The driver does not flash sensor firmware.

## Support scope

The proven target is the exact GXFP51A0 / GF3658 / ST411 combination above.
Another machine with the same ACPI HID may still have different GPIO wiring,
firmware or board integration. The installer therefore detects the HID, while
the runtime also validates the expected target behavior.

This remains reverse-engineered, experimental biometric software. Validation so
far is strongest on the reference unit and same-user cross-finger negative
controls; it is not a substitute for a large cross-person biometric
certification corpus. Do not treat fingerprint alone as a high-assurance
security factor.

See:

- [native desktop integration](docs/native-desktop-integration.md)
- [provenance](PROVENANCE.md)
- [driver source](driver/goodix51a0/)
- [research log](docs/research-log.md)
- [current handoff](docs/rel71.74-production-20261028.md)

The production driver subtree is `LGPL-2.1-or-later`; see per-file SPDX
notices and [provenance](PROVENANCE.md).
