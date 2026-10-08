# Goodix GXFP51A0 / GF3658 ST411 on Linux

Native libfprint driver and zero-to-working installer for the SPI Goodix
GXFP51A0 found in the Huawei MateBook 13 2021 family.

> Français: [README.FR.md](README.FR.md) · 简体中文: [README.ZH-CN.md](README.ZH-CN.md)

## Status — 2026-10-08

### Recommended preview: rel71.30 (reference-machine field test)

rel71.30 is the default install. Native KDE enrollment followed by eight successful scored verification captures on the reference MateBook 13 at threshold 7 (7, 10, 7, 13, 9, 11, 11, 19). No post-enrollment ACK/TLS errors were logged during that series. This is not a population-level false-acceptance estimate. rel71.24 is still the last fully cold-boot/deep-S3 validated rollback (rel71.18 older immutable reference).

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

The active biometric decision is conservative: fixed threshold 7, best single enrolled view, no score addition or fusion, and no score-conditioned same-placement retry. A usable image receives one score; a score below 7 requires lift/reposition. Up to three independent physical placements are allowed.

Full rationale and package hashes: [`docs/validated-checkpoint-71.24.md`](docs/validated-checkpoint-71.24.md).

The installer is idempotent: an exact current release is a no-op for the live sensor session. A real driver change must pass a fresh semantic prewarm (`PREWARM_RESULT=READY`) before success is reported.

## Driver-only distribution (no Huawei/GPU manager)

The [driver-only installation guide](DRIVER_ONLY.md) covers the **independent source archive** and prebuilt Arch/CachyOS package attached to the rel71.30 GitHub pre-release. Both use standard libfprint → fprintd → native Linux desktop interfaces and require no HUAWEI Whiptail manager, GPU tools or project-specific enrollment UI. Only the exact GXFP51A0/ST411 hardware profile is in scope; distribution and board differences require independent confirmation.

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
6. installs one bounded boot prime and removes periodic keepalive/external sleep glue;
7. uses the desktop's native fprintd/PAM integration by default; optional legacy Plasma integrations are never applied silently;
8. preserves password authentication in parallel;
9. preserves existing fingerprint templates and the validated PMK cache;
10. instructs users to enroll with their distribution's native GUI (KDE/GNOME), without invoking CLI enrollment;
11. preserves existing finger templates;
12. finishes with `gxfp51a0-doctor`.

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

For Plasma 6.7.5, the repository carries package-managed KScreenLocker and
Plasma Login Manager integrations used by the validated reference setup.

## Production package invariants

The Arch/CachyOS production package contains the recommended rel71.30 driver and only
one GXFP-specific lifecycle helper: a **one-shot boot prime** before graphical
login. It deliberately does **not** package:

- periodic warm-keepalive timers/services;
- external fprintd suspend services;
- resume-prewarm hooks.

Normal idle readiness is retained inside the resident fprintd/libfprint process
without polling. S3 recovery is handled by the native driver lifecycle plus
KScreenLocker PAM resume handling.

### Optional privacy-preserving matcher validation

Benjamin Allègre (Sigfrodr) published `tools/eval/fp_eval.py` in
`Sigfrodr/libfprint-goodixtls` as a local-only evaluator for the Milan-SPI
family. It reports aggregate statistics while raw captures/templates remain on
the tester machine. It is not a runtime dependency, and release builds contain
no biometric capture dump writer.

## Installation by distribution

Research from rel24–rel61 is retained in [historical research](docs/history/), the [research log](docs/research-log.md) and Git history. These notes are **not current installation instructions**. Install the rel71.30 preview; keep rel71.24 for cold-boot/deep-S3-validated rollback.

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
5. installs an early cold-boot prewarm ordered before the display manager;
6. keeps deep-resume recovery inside libfprint itself, with no external
   system-sleep hook or worker;
7. deliberately installs no periodic synthetic Claim keepalive;
8. when KDE Plasma 6.7.5 is present, installs the reversible window-ready
   lock-screen integration and its pacman reapply hook;
9. when Plasma Login Manager 6.7.5 is installed, builds/installs the
   package-managed concurrent password/fingerprint compatibility package;
10. removes obsolete development timing state, reloads udev and restarts
    fprintd without touching enrollments or the PMK cache.

Non-KDE desktops are left unchanged. On KDE, one package-owned
LockScreenUi.qml file is intentionally patched by the integration helper and
restored on driver removal.

Enroll, remove and verify fingerprints through your desktop's native graphical settings (KDE Plasma **System Settings → Users**, GNOME **Settings → Users**). The installer never starts command-line biometric enrollment or verification.

The driver requests 20 enrollment presses. Move the finger slightly between
presses so the small 80×64 sensor sees different parts of the fingertip.

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
- [current handoff](HANDOFF_CURRENT.md)

The production driver subtree is `LGPL-2.1-or-later`; see per-file SPDX
notices and [provenance](PROVENANCE.md).
