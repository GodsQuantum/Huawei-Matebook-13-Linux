# One-command MateBook 13 Linux onboarding

## Goal

A Huawei MateBook 13 matching the validated reference profile should be able to move to a fresh Linux installation or another supported distribution and converge to the known-good hardware setup with:

```bash
git clone https://github.com/GodsQuantum/huawei-matebook-13-linux.git
cd huawei-matebook-13-linux
./install.sh
```

The repository closes **hardware-specific Linux gaps** and then gets out of the way. Normal desktop settings, package updates, audio, networking, input, power profiles and firmware tools remain distro-owned.

## Reference profile

Fully validated reference:

- Huawei MateBook 13 `WRTB-WXX9`
- Intel Core i7-10510U / Comet Lake-U
- Intel UHD `8086:9b41`
- NVIDIA GeForce MX250 / GP108M Pascal `10de:1d13`
- Intel CNVi Wi-Fi `8086:02f0`
- Intel Bluetooth `8087:0aaa`
- Intel HDA `8086:02c8`
- IMC Networks UVC camera `13d3:56c6`
- ELAN touchpad / touch / stylus
- mainline `huawei_wmi`
- Goodix `GXFP51A0` / GF3658 ST411 fingerprint reader

Other Huawei models are never assumed identical. Each hardware-specific module still validates its own IDs.

## Native-first rule

Every feature belongs to one of four classes:

1. **Native and correct** — verify it; do not replace it.
2. **Native Huawei interface** — load/use the upstream kernel interface; do not ship a duplicate driver.
3. **Real upstream gap** — install the smallest reviewed repository component.
4. **User policy** — expose/verify it, but do not silently choose it.

## Component policy

| Component | Reference-machine status | Repository action |
| --- | --- | --- |
| Goodix GXFP51A0 fingerprint | Missing from upstream libfprint | Install the repository-native libfprint/fprintd driver, current checkpoint **rel71.24** |
| MX250 / GP108M Pascal | PRIME works, but NVIDIA PCIe RTD3 requires Turing+ | Install the reviewed on-demand PCI power-gating manager v3.2 |
| Intel UHD | Mainline i915 | Verify only |
| Huawei hotkeys / Fn-lock / mic-mute LED | Mainline `huawei_wmi` | Load/verify only |
| Battery charge thresholds | Mainline `huawei_wmi` | Verify interfaces only; never choose percentages silently |
| CPU/platform power policy | Distro-native | Keep one provider; do not stack PPD/tuned/TLP/auto-cpufreq |
| Camera | UVC native | Verify only |
| Wi-Fi / Bluetooth | Intel native | Verify only |
| Touchpad / touch / stylus | HID/I2C native | Verify only |
| Audio | Intel HDA + normal PipeWire/ALSA stack | Verify only |
| Suspend / resume | Kernel/systemd/desktop native | Verify deep-sleep availability; fingerprint handles only its device lifecycle |
| BIOS / firmware | fwupd/vendor/distro | Never flash automatically |

## Fingerprint

Current public checkpoint: **rel71.24**.

Key properties:

- libfprint base `v1.94.100`
- fixed matcher threshold **7**
- template v4 / 20 enrollment views
- one usable physical placement = one biometric score
- no score fusion
- short active-HIGH GPIO264 prestart recovery
- real TLS deep-lock recovery validated without reboot/power-cycle
- manual deep-S3 transport validation
- warm readiness around 82–83 ms on the reference unit
- no firmware flashing and no kernel pinning

See:

- [validated checkpoint 71.24](../fingerprint/docs/validated-checkpoint-71.24.md)
- [recovery architecture](../fingerprint/docs/recovery-architecture-2026-10-06.md)

### Fingerprint distro coverage

The portable installer supports:

- Arch / CachyOS — native pacman package
- Debian / Ubuntu
- Fedora / RHEL-family
- openSUSE
- Alpine

Non-Arch systems stage the pinned libfprint build, validate the distro fprintd ABI before installation, and isolate the local libfprint to fprintd rather than globally replacing the distro library.

## MX250 power management: why the repository manages it

The MX250 is **Pascal**. NVIDIA's documented PCIe Runtime D3 mechanism requires **Turing or newer**, so ordinary PRIME on this GPU does not guarantee that the discrete GPU reaches a true off state.

The reference machine confirms this: with the MX250 merely present under R580, the NVIDIA power interface reports RTD3 disabled and the PCI device can remain in D0 even with `power/control=auto`.

The repository therefore uses the existing reviewed kernel mechanisms rather than bbswitch/acpi_call:

```text
normal desktop/app
→ Intel is the default DRI/Vulkan device
→ MX250 stays PCI-absent while idle

managed dGPU application
→ PCI rescan
→ load NVIDIA R580
→ PRIME Render Offload
→ run application
→ wait until no dGPU users remain
→ unload NVIDIA
→ remove only the MX250 PCI function
→ full Integrated idle
```

### Preventing accidental GPU wakeups

Some ordinary Vulkan applications enumerate every installed ICD and can open `/dev/nvidia*` even when they do not need the dGPU.

GPU Manager v3.2 writes a user-session Intel/Mesa default:

```text
DRI_PRIME=pci-<Intel BDF>
VK_LOADER_DRIVERS_SELECT=*intel*
__GLX_VENDOR_LIBRARY_NAME=mesa
__EGL_VENDOR_LIBRARY_FILENAMES=<detected Mesa GLVND JSON>
```

The managed dGPU runner explicitly removes the Intel DRI/Vulkan/EGL filters, selects NVIDIA through the standard PRIME/GLVND variables, and then starts the selected application.

These loader defaults reduce accidental GPU grabs but are not treated as a security boundary. The helper always checks actual NVIDIA device-node ownership. If an unrelated application still opens the dGPU (for example through a direct DRM/DMABUF path), shutdown **fails safe**: the manager does not kill the process and does not force-unload NVIDIA; the cleanup timer retries later.

Applications already declaring the standard desktop metadata:

- `PrefersNonDefaultGPU=true`
- `X-KDE-RunOnDiscreteGpu=true`

are automatically imported into the managed set, except the Steam client. Steam itself remains on Intel; individual games can be managed through preserved Launch Options.

The manager is transactional, migration-aware and reversible. It refuses to unload/remove a GPU while a process still owns NVIDIA device nodes.

## NVIDIA driver branch

For Maxwell/Pascal/Volta, NVIDIA **R580** is the final supported driver branch. The manager refuses a newer incompatible branch.

Packaging varies by distribution:

- Arch/CachyOS: `nvidia-580xx-dkms` / matching userspace where available, with repository/AUR helper support.
- Fedora: RPM Fusion's 580xx packages when that repository is already enabled.
- Ubuntu: `nvidia-driver-580` where the distro exposes it.
- Debian/openSUSE/other systemd distributions: use the distro's supported R580 packaging first when automatic package discovery cannot safely provide it.

The project deliberately prefers distribution-packaged NVIDIA drivers over NVIDIA's generic runfile.

## Power policy

The repository does **not** choose battery thresholds or a permanent performance profile.

It verifies that Huawei's kernel interfaces exist and checks for conflicting power-policy daemons. A normal installation should have at most one high-level provider such as `power-profiles-daemon`.

The current profile remains a normal desktop choice.

## Firmware

The repository never flashes BIOS, EC, fingerprint firmware, GPU firmware or other device firmware automatically.

If `fwupd` exposes an update, the user remains responsible for reviewing and applying it through the normal distro workflow.

## Root installer behavior

`./install.sh`:

1. verifies Huawei DMI identity;
2. loads mainline `huawei_wmi` when available;
3. installs/updates GXFP51A0 support only when that SPI device exists;
4. installs/repairs the MX250 manager for the reference Pascal GPU;
5. leaves native camera/network/input/audio drivers untouched;
6. does not alter battery thresholds;
7. does not install competing CPU power managers;
8. finishes with the read-only root doctor.

Useful modes:

```bash
./install.sh --doctor-only
./install.sh --no-fingerprint
./install.sh --no-gpu
./install.sh --gpu-driver-preinstalled
```

Run the installer as the normal desktop user, not with `sudo`.

## Fresh-install contract

A clean reinstall should require no historical handoff or private machine knowledge. Public main contains:

- complete fingerprint source and installers;
- current known-good fingerprint checkpoint;
- the MX250 power manager;
- multi-distro adapters;
- deterministic tests;
- one-command root onboarding;
- read-only doctor;
- rollback/uninstall paths;
- public-safe technical rationale.

Private hostnames, usernames, home paths, serials, fingerprint captures/templates and PMK/PSK material are excluded.

## External technical basis

Current upstream sources used by the project include:

- Linux mainline `HUAWEI_WMI` Kconfig: Huawei hotkeys, battery charge control, Fn-lock and mic-mute LED are kernel-supported.
- Linux power-supply ABI: standard charge-control start/end threshold interfaces.
- NVIDIA Linux R580 documentation: PCIe RTD3 requires Turing or newer.
- NVIDIA Unix legacy support policy: 580.* is the final Linux driver series for Maxwell, Pascal and Volta.
- NVIDIA PRIME Render Offload documentation.
- Linux VGA Switcheroo / PRIME documentation.

See [HARDWARE_SUPPORT_MATRIX.md](HARDWARE_SUPPORT_MATRIX.md) for the audited support matrix and upstream references.
