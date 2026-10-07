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
| **Fingerprint** | **rel71.24 validated** | Repository-native libfprint/fprintd driver; threshold 7; deep-S3 and deep-TLS recovery validated |
| **MX250 power** | **GPU Manager v3.2** | Intel-first session; MX250 PCI-off while idle; R580 + PRIME only for managed dGPU apps |
| **Huawei hotkeys / Fn-lock / battery interfaces** | **Mainline Linux** | Use `huawei_wmi`; do not duplicate it |
| **Intel GPU / Wi-Fi / Bluetooth / camera / touch / stylus / audio** | **Native Linux** | Verify only |
| **CPU/platform power profiles** | **Distro-owned** | Keep one provider; do not stack competing managers |
| **Firmware / BIOS** | **Distro/vendor-owned** | Never flash automatically |

## Fingerprint

Current checkpoint: **rel71.24**.

- warm readiness around **82–83 ms** on the validated reference unit;
- short active-HIGH GPIO264 recovery;
- recovered a real TLS/digest lock without reboot or power-cycle;
- manual deep-S3 transport validation;
- fixed threshold **7**;
- no score fusion;
- existing template-v4 enrollments remain compatible.

Technical record: [fingerprint/docs/validated-checkpoint-71.24.md](fingerprint/docs/validated-checkpoint-71.24.md)

Standalone install:

```bash
./fingerprint/install.sh
```

Fingerprint installer coverage: **Arch/CachyOS, Debian/Ubuntu, Fedora/RHEL-family, openSUSE and Alpine**.

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

## License

GPL-2.0-only at repository root; fingerprint production-driver files retain their SPDX-declared license.
