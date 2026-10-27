<div align="center">

# Huawei MateBook 13 · Linux

**Bring missing Linux hardware support to your MateBook 13.**

Goodix **GXFP51A0 / GF3658 ST411** fingerprint · NVIDIA **MX250** on-demand power · native-first setup for MateBook 13 **2020 / 2021** (`WRTB-WXX9`)

[![Quality](https://github.com/GodsQuantum/Huawei-Matebook-13-Linux/actions/workflows/quality.yml/badge.svg)](https://github.com/GodsQuantum/Huawei-Matebook-13-Linux/actions/workflows/quality.yml)
[![Linux distro builds](https://github.com/GodsQuantum/Huawei-Matebook-13-Linux/actions/workflows/fingerprint-portability.yml/badge.svg)](https://github.com/GodsQuantum/Huawei-Matebook-13-Linux/actions/workflows/fingerprint-portability.yml)
[![License](https://img.shields.io/badge/license-GPL--2.0%20%2B%20LGPL--2.1-3dd7cf)](LICENSES.md)
[![Linux](https://img.shields.io/badge/platform-Linux-0B1622)](docs/HARDWARE_SUPPORT_MATRIX.md)
[![Release](https://img.shields.io/badge/fingerprint-rel71.69%20reference-2D8542)](https://github.com/GodsQuantum/Huawei-Matebook-13-Linux/releases/tag/matebook13-rel71.69)

[**English**](README.md) · [**Français**](README.FR.md) · [**简体中文**](README.ZH-CN.md)

</div>

**Maintainers and AI agents:** [project map](docs/PROJECT_MAP.md) · [repository rules](AGENTS.md) · [fingerprint current state](fingerprint/HANDOFF_CURRENT.md). rel71.69 is the reference; older benchmarks are historical.

## Choose your install

**Fingerprint driver only:** `./fingerprint/install.sh` ([standalone archive](fingerprint/DRIVER_ONLY.md)). **Full MateBook 13 setup (fingerprint + MX250):** `./install.sh`. **Read-only compatibility check:** `./install.sh --doctor-only`.

Run everything as your regular desktop user, **not root**. Enroll or remove fingerprints using **KDE/GNOME's native graphical settings**, never a custom enrollment tool.

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
| **Fingerprint** | **rel71.69 recommended reference · one verified S3 + S4 on reference unit** | Repository-native libfprint/fprintd; threshold 7; rel71.30 adds connected enrollment + conservative per-view near-miss rescue |
| **MX250 power** | **GPU Manager v3.2** | Intel-first session; MX250 PCI-off while idle; R580 + PRIME only for managed dGPU apps |
| **Huawei hotkeys / Fn-lock / battery interfaces** | **Mainline Linux** | Use `huawei_wmi`; do not duplicate it |
| **Intel GPU / Wi-Fi / Bluetooth / camera / touch / stylus / audio** | **Native Linux** | Verify only |
| **CPU/platform power profiles** | **Distro-owned** | Keep one provider; do not stack competing managers |
| **Firmware / BIOS** | **Distro/vendor-owned** | Never flash automatically |

## Fingerprint

**Default fingerprint install: rel71.69.** Actual reference MateBook 13 with GXFP51A0 passed three operator-attested first-touch authentications (LOCK, S4 hibernation return, genuine deep S3 on 27 October 2026). Native S3 became ready after 5.153 s, positive after 8.218 s from wake; measured finger-to-positive 2.054 s and score 15/7. rel71.69 adds native 50 µs SPI CS-to-clock setup validated using kernel SPI tracepoints, plus proven clean S3 reference and strict FDT acknowledgement gates. **More S3 cycles, cold boots, reboots and wrong-finger testing are still required for broader certification.** Prior rel71.30 and rel71.24 data are preserved as historical references. See [rel71.69 measured hardware evidence](fingerprint/docs/rel71.69-hardware-final-20261027.md) and the [Hall of Fame](fingerprint/docs/HALL_OF_FAME.md).

- warm readiness around **82–83 ms** on the validated reference unit;
- short active-HIGH GPIO264 recovery;
- recovered a real TLS/digest lock without reboot or power-cycle;
- manual deep-S3 transport validation;
- fixed threshold **7**;
- no score fusion;
- existing template-v4 enrollments remain compatible.

Technical records: [rel71.69 reference](fingerprint/docs/rel71.69-hardware-final-20261027.md) · [historical rel71.30](fingerprint/docs/candidate-71.30.md)

Driver-only install (no Huawei/GPU Manager). See [standalone guide](fingerprint/DRIVER_ONLY.md):

```bash
./fingerprint/install.sh
```

Fingerprint source installers support **Arch/CachyOS, Debian/Ubuntu, Fedora/RHEL-family, openSUSE and Alpine**. **Ubuntu 24.04 LTS and 26.04 LTS both PASS the complete source-build-only GitHub CI**, alongside Debian, Fedora, openSUSE, Arch and Alpine (7/7). Ubuntu hardware enrollment, unlock and MX250 runtime remain **untested**, so compilation success is not a functional guarantee. On a similar Ubuntu MateBook, the **driver-only** installer is `./fingerprint/install.sh`; the **full HUAWEI + GPU** installer is `./install.sh` after its read-only `--doctor-only` audit. The GPU path additionally requires the exact MX250 `10de:1d13` and compatible NVIDIA R580 (available in Ubuntu 24.04/26.04 repositories); Secure Boot/DKMS, fingerprint D-Bus ABI and native desktop PAM must be verified locally.

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
HUAWEI --gpu add
HUAWEI --gpu list
HUAWEI --gpu doctor
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
| `HUAWEI --gpu` | Show the GPU Manager overview. |
| `HUAWEI --gpu menu` | Open GPU Manager's own interactive menu. |
| `HUAWEI --gpu overview` | Explicit overview/dashboard alias. |
| `HUAWEI --gpu status` | Show detailed MX250/NVIDIA/PCI/lease state. |
| `HUAWEI --gpu list` | List applications currently managed for NVIDIA. |
| `HUAWEI --gpu run -- COMMAND` | Use the MX250 for one launch only, then release it. Example: `HUAWEI --gpu run -- handy`. |
| `HUAWEI --gpu add App.desktop` | Make one desktop application use the MX250 whenever launched normally. Example: `HUAWEI --gpu add Handy.desktop`. |
| `HUAWEI --gpu remove App.desktop` | Remove that permanent NVIDIA wrapper and restore default/Intel launch behavior. |
| `HUAWEI --gpu add` | Interactively choose a desktop application to manage. |
| `HUAWEI --gpu steam-add APPID` | Enable on-demand NVIDIA for one Steam game. |
| `HUAWEI --gpu steam-remove APPID` | Remove on-demand NVIDIA for one Steam game. |
| `HUAWEI --gpu steam-all-on` | Enable on-demand NVIDIA for all detected Steam games. |
| `HUAWEI --gpu steam-all-off` | Disable the all-games Steam policy. |
| `HUAWEI --gpu apply` | Reapply all saved desktop/Steam routing rules. |
| `HUAWEI --gpu install / repair / upgrade` | Install or transactionally repair/upgrade GPU Manager. |
| `HUAWEI --gpu doctor` | Read-only GPU Manager integrity audit. |
| `HUAWEI --gpu test` | Reversible wake/load/test/release MX250 smoke test. |
| `HUAWEI --gpu uninstall` | Remove GPU Manager integration and restore saved desktop state. |
| `HUAWEI --gpu --lang en|fr|zh COMMAND` | Select the GPU Manager CLI/menu language. |

GPU Manager deliberately does **not** guess which applications are “GPU-heavy”. During install/repair it can import desktop files that explicitly request the discrete GPU through `PrefersNonDefaultGPU=true` or `X-KDE-RunOnDiscreteGpu=true`. Other applications stay on Intel by default. Use `HUAWEI --gpu run -- app` for an occasional NVIDIA launch, or `HUAWEI --gpu add App.desktop` to make that application use NVIDIA whenever launched normally.

For Handy specifically, the recommended setup is to keep its autostart on Intel and use `HUAWEI --gpu run -- handy` only when the MX250 is explicitly desired.

## License

GPL-2.0-only at repository root; fingerprint production-driver files retain their SPDX-declared license.
