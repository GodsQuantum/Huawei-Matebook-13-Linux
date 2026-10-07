# Huawei MateBook 13 on Linux

> One-command, native-first Linux onboarding for the Huawei MateBook 13 family.
>
> **Français: [README.FR.md](README.FR.md)** · **简体中文: [README.ZH-CN.md](README.ZH-CN.md)**

## Goal

A supported MateBook 13 should be able to move to a fresh Linux install or a different distribution and reach the validated hardware state with one command:

```bash
git clone https://github.com/GodsQuantum/huawei-matebook-13-linux.git
cd huawei-matebook-13-linux
./install.sh
```

The project follows one rule: **native first**. If Linux already supports a component correctly, the repository verifies it and leaves it under the distro/kernel/desktop. Repository code is installed only for real hardware-specific gaps.

See [`docs/ONE_COMMAND_ONBOARDING.md`](docs/ONE_COMMAND_ONBOARDING.md) for the architecture and fresh-install contract.

## Current reference hardware

The validated reference profile is Huawei `WRTB-WXX9` / MateBook 13 with:

- Intel Comet Lake-U UHD graphics;
- NVIDIA GeForce MX250 / GP108M (`10de:1d13`);
- Intel CNVi Wi-Fi + Bluetooth;
- Intel HDA audio;
- IMC Networks UVC HD camera (`13d3:56c6`);
- ELAN touchpad / touch / stylus;
- mainline `huawei_wmi`;
- Goodix `GXFP51A0` / GF3658 ST411 fingerprint reader.

Other MateBook 13 revisions are detected by hardware IDs rather than assumed compatible.

## Status

| Area | Status | Project policy |
| --- | --- | --- |
| **Fingerprint — GXFP51A0 / GF3658** | **Validated rel71.18 checkpoint** | Repository-managed native libfprint/fprintd driver; fixed threshold 7; target-only pre-enumeration spidev recovery; 3 consecutive first-pose KDE lock passes |
| **GPU — NVIDIA MX250** | **Working with standard PRIME** | Distro NVIDIA R580 legacy branch + PRIME Render Offload; do not install the old PCI-remove manager by default |
| **Huawei hotkeys / Fn lock / battery thresholds** | **Mainline kernel** | Use `huawei_wmi`; validate, do not duplicate the driver |
| **Camera / Wi-Fi / Bluetooth / touchpad / stylus / audio** | **Native on the reference unit** | Validate only; no hardware-specific replacement unless a model proves it needs one |
| **Power profiles / suspend** | **Distro-owned** | Use the normal distro stack; do not stack competing power managers |

## Fingerprint checkpoint 71.18

71.18 is the frozen known-good checkpoint before further optimization.

The key recovery boundary is:

```text
fprintd start/restart
→ unbind only spi-GXFP51A0:00 from spidev
→ GPIO264 active-HIGH reset
→ settle
→ rebind only spi-GXFP51A0:00
→ udev settle
→ libfprint/fprintd
```

A reader deliberately left in a TLS-degraded state recovered without reboot or power-cycle. Warm Claims returned to about **82–83 ms FAST_READY**, then three consecutive ordinary KDE locks succeeded on the first physical placement with baseline scores **8/7, 11/7 and 12/7**.

Technical record: [`fingerprint/docs/validated-checkpoint-71.18.md`](fingerprint/docs/validated-checkpoint-71.18.md).

Standalone fingerprint install:

```bash
./fingerprint/install.sh
```

## GPU policy

The reference MX250 uses the standard distribution PRIME path:

```text
desktop → Intel UHD
GPU workload → prime-run → NVIDIA MX250
idle → standard NVIDIA/PCIe runtime power management
```

The historical [`gpu-power/`](gpu-power/) PCI-remove manager is retained for research/history only. It is not part of the default onboarding path.

## Hardware doctor

Read-only validation:

```bash
./matebook13-doctor.sh
# or
./install.sh --doctor-only
```

The doctor checks Huawei WMI, PRIME/NVIDIA presence, GXFP51A0 integration, camera, hotkeys, touchpad, Wi-Fi, Bluetooth, video and audio nodes without changing user policy.

## Safety and privacy

The default onboarding never flashes firmware, never changes battery charge thresholds silently, never pins/downgrades the kernel for fingerprint support, and never installs multiple competing power managers.

Do not publish usernames, home paths, hostnames, serial numbers, UUIDs, credentials, fingerprint captures/templates, PMK/PSK material, proprietary firmware or Windows binaries.

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) and [SECURITY.md](SECURITY.md). Reports should distinguish **CONFIRMED**, **INFERRED** and **HYPOTHESIS** claims and use generic hardware identifiers.

## License

GPL-2.0-only for the repository root; fingerprint production-driver files retain their SPDX-declared license.
