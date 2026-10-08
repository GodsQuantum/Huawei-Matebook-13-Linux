# Huawei MateBook 13 on Linux

> One-command, native-first Linux onboarding for the Huawei MateBook 13 reference platform.
>
> **Français: [README.FR.md](README.FR.md)** · **简体中文: [README.ZH-CN.md](README.ZH-CN.md)**

## One command

On a fresh install or after changing distribution:

```bash
git clone https://github.com/GodsQuantum/huawei-matebook-13-linux.git
cd huawei-matebook-13-linux
./install.sh
```

Run it as your normal desktop user, not with `sudo`.

With no arguments in an interactive terminal, `./install.sh` first performs a read-only hardware audit and opens a **Whiptail** menu. The UI auto-sizes to the current terminal; `Esc`/Cancel exits without changes. **RECOMMENDED** checks the native platform baseline and applies only missing/outdated repository components, while **FINGERPRINT** and **GPU** are strictly component-only actions.

After a successful install/update, the checkout installs a user launcher:

```bash
HUAWEI
```

`HUAWEI` opens the same audited menu. Explicit flags such as `HUAWEI --doctor-only` remain available for scripting.

Re-running the exact installed fingerprint release is safe: the installer preserves the live fprintd/TLS session instead of rebuilding or restarting the sensor. A real driver upgrade must complete a new semantic `PREWARM_RESULT=READY` gate before the installer reports success.

The project fixes only the hardware gaps that Linux does not already handle correctly. Native kernel/distro support remains native.

## Validated reference hardware

Huawei MateBook 13 `WRTB-WXX9`:

- Intel Comet Lake-U UHD
- NVIDIA GeForce MX250 / GP108M `10de:1d13`
- Intel CNVi Wi-Fi + Bluetooth
- Intel HDA audio
- IMC Networks UVC camera `13d3:56c6`
- ELAN touchpad / touch / stylus
- mainline `huawei_wmi`
- Goodix `GXFP51A0` / GF3658 ST411 fingerprint reader

Other Huawei revisions are not assumed compatible; hardware-specific modules validate their IDs.

## What the repository manages

| Area | Status | Policy |
| --- | --- | --- |
| **Fingerprint** | **rel71.30 recommended preview · rel71.24 S3-validated rollback** | Repository-native libfprint/fprintd; threshold 7; rel71.30 adds connected enrollment + conservative per-view near-miss rescue |
| **MX250 power** | **GPU Manager v3.2** | Intel-first session; MX250 PCI-off while idle; R580 + PRIME only for managed dGPU apps |
| **Huawei hotkeys / Fn-lock / battery interfaces** | **Mainline Linux** | Use `huawei_wmi`; do not duplicate it |
| **Intel GPU / Wi-Fi / Bluetooth / camera / touch / stylus / audio** | **Native Linux** | Verify only |
| **CPU/platform power profiles** | **Distro-owned** | Keep one provider; do not stack competing managers |
| **Firmware / BIOS** | **Distro/vendor-owned** | Never flash automatically |

## Fingerprint

**Default fingerprint install: rel71.30.** After KDE-native re-enrollment, eight post-enrollment verification captures passed on the Huawei MateBook 13 reference machine at threshold 7 (scores 7, 10, 7, 13, 9, 11, 11, 19). rel71.24 remains the last fully cold-boot/deep-S3 validated rollback. An initial wrong-finger check on the reference unit also rejected three unenrolled-finger placements (scores 2, 3, 4) and then accepted the enrolled finger at score 16. Broader cross-finger and other-machine evidence is still required.

- warm readiness around **82–83 ms** on the validated reference unit;
- short active-HIGH GPIO264 recovery;
- recovered a real TLS/digest lock without reboot or power-cycle;
- manual deep-S3 transport validation;
- fixed threshold **7**;
- no score fusion;
- existing template-v4 enrollments remain compatible.

Technical records: [validated rel71.24](fingerprint/docs/validated-checkpoint-71.24.md) · [candidate rel71.30](fingerprint/docs/candidate-71.30.md)

Driver-only install (no Huawei/GPU Manager). See [standalone guide](fingerprint/DRIVER_ONLY.md):

```bash
./fingerprint/install.sh
```

Fingerprint source installers support **Arch/CachyOS, Debian/Ubuntu, Fedora/RHEL-family, openSUSE and Alpine**. **Ubuntu compatibility is a build/installation design target, not yet a runtime validation.** On a similar Ubuntu MateBook, the **driver-only** installer is `./fingerprint/install.sh`; the **full HUAWEI + GPU** installer is `./install.sh` after its read-only `--doctor-only` audit. The GPU path additionally requires the exact MX250 `10de:1d13` and compatible NVIDIA R580 (available in Ubuntu 24.04/26.04 repositories); Secure Boot/DKMS, fingerprint D-Bus ABI and native desktop PAM must be verified locally.

## Why the MX250 needs repository power management

The MX250 is **Pascal**. NVIDIA's documented PCIe Runtime D3 power management requires **Turing or newer**, so standard PRIME alone cannot guarantee a true off state on this GPU.

GPU Manager v3.2 therefore:

1. keeps normal DRI/Vulkan/GLX/EGL applications on Intel/Mesa;
2. leaves the MX250 removed from PCI while idle;
3. rescans and loads NVIDIA R580 only for a managed dGPU application;
4. uses standard PRIME Render Offload for that application;
5. refuses to power down while anything still uses NVIDIA;
6. unloads NVIDIA and removes only the MX250 again.

If any unrelated process still owns an NVIDIA device node after a managed workload exits, the helper **fails safe**: it leaves the GPU online rather than killing the process or force-unloading an in-use driver, and the cleanup timer retries later.

Applications that already declare `PrefersNonDefaultGPU=true` or `X-KDE-RunOnDiscreteGpu=true` are imported automatically. Steam itself stays on Intel; individual games can be managed separately.

Manual management remains available when an application's desktop metadata is wrong or missing:

```bash
GPU-control add
GPU-control list
GPU-control doctor
```

## Native Huawei features

Current Linux `huawei_wmi` already provides the platform features this model needs, including Huawei hotkeys, Fn-lock, battery charge control and mic-mute LED support.

The repo therefore **does not** ship a duplicate Huawei platform driver and **does not** silently choose battery thresholds.

## Hardware doctor

Read-only:

```bash
./matebook13-doctor.sh
# or
./install.sh --doctor-only
```

It checks the reference model, Huawei WMI, R580/MX250 policy, conflicting power daemons, deep sleep, fingerprint, camera, input, Wi-Fi/Bluetooth, audio and firmware-management visibility.

## Installer options

```bash
./install.sh --no-fingerprint
./install.sh --no-gpu
./install.sh --gpu-driver-preinstalled
```

The proprietary NVIDIA package name differs by distribution. The manager uses distro packages where it can do so safely and refuses an incompatible post-R580 branch on Pascal.

## Design and audit

- [One-command architecture](docs/ONE_COMMAND_ONBOARDING.md)
- [Audited hardware support matrix](docs/HARDWARE_SUPPORT_MATRIX.md)
- [Fingerprint README](fingerprint/README.md)
- [GPU/power README](gpu-power/README.md)

## Safety / privacy

The default onboarding never flashes firmware, never silently changes charge thresholds, never pins/downgrades the kernel for fingerprint support, and never installs multiple competing CPU power managers.

Do not publish usernames, home paths, hostnames, serial numbers, UUIDs, credentials, fingerprint captures/templates, PMK/PSK material, proprietary firmware or Windows binaries.

## Command shortcuts

The same controls are also available inside the `HUAWEI` Whiptail menu under **SHORTCUTS**.

| Command | Purpose |
|---|---|
| `HUAWEI` | Open the audit-first MateBook control center. |
| `HUAWEI --doctor-only` | Run a read-only hardware/software audit. |
| `HUAWEI --menu` | Force the interactive Whiptail menu. |
| `HUAWEI --no-fingerprint` | Run onboarding while skipping fingerprint changes. |
| `HUAWEI --no-gpu` | Run onboarding while skipping GPU Manager changes. |
| `HUAWEI --gpu-driver-preinstalled` | Require a distro-provided NVIDIA R580 driver; never install one. |
| `GPU-control` | Show the GPU Manager overview. |
| `GPU-control menu` | Open GPU Manager's own interactive menu. |
| `GPU-control overview` | Explicit overview/dashboard alias. |
| `GPU-control status` | Show detailed MX250/NVIDIA/PCI/lease state. |
| `GPU-control list` | List applications currently managed for NVIDIA. |
| `GPU-control run -- COMMAND` | Use the MX250 for one launch only, then release it. Example: `GPU-control run -- handy`. |
| `GPU-control add App.desktop` | Make one desktop application use the MX250 whenever launched normally. Example: `GPU-control add Handy.desktop`. |
| `GPU-control remove App.desktop` | Remove that permanent NVIDIA wrapper and restore default/Intel launch behavior. |
| `GPU-control add` | Interactively choose a desktop application to manage. |
| `GPU-control steam-add APPID` | Enable on-demand NVIDIA for one Steam game. |
| `GPU-control steam-remove APPID` | Remove on-demand NVIDIA for one Steam game. |
| `GPU-control steam-all-on` | Enable on-demand NVIDIA for all detected Steam games. |
| `GPU-control steam-all-off` | Disable the all-games Steam policy. |
| `GPU-control apply` | Reapply all saved desktop/Steam routing rules. |
| `GPU-control install / repair / upgrade` | Install or transactionally repair/upgrade GPU Manager. |
| `GPU-control doctor` | Read-only GPU Manager integrity audit. |
| `GPU-control test` | Reversible wake/load/test/release MX250 smoke test. |
| `GPU-control uninstall` | Remove GPU Manager integration and restore saved desktop state. |
| `GPU-control --lang en|fr|zh COMMAND` | Select the GPU Manager CLI/menu language. |

GPU Manager deliberately does **not** guess which applications are “GPU-heavy”. During install/repair it can import desktop files that explicitly request the discrete GPU through `PrefersNonDefaultGPU=true` or `X-KDE-RunOnDiscreteGpu=true`. Other applications stay on Intel by default. Use `GPU-control run -- app` for an occasional NVIDIA launch, or `GPU-control add App.desktop` to make that application use NVIDIA whenever launched normally.

For Handy specifically, the recommended setup is to keep its autostart on Intel and use `GPU-control run -- handy` only when the MX250 is explicitly desired.

## License

GPL-2.0-only at repository root; fingerprint production-driver files retain their SPDX-declared license.
