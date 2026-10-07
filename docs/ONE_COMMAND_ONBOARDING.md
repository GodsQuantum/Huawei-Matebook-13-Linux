# One-command MateBook 13 Linux onboarding

## Goal

A supported Huawei MateBook 13 should be able to move to a fresh Linux install or a different distribution and reach the validated hardware configuration with one command:

```bash
./install.sh
```

The repository is not intended to replace the distribution. Its job is to close hardware-specific gaps and then get out of the way so normal system settings, updates, desktop controls and package management remain authoritative.

## Design rule: native first

Every component is classified before the repository touches it:

1. **Native and working** — verify it and leave it alone.
2. **Native but needs a Huawei-specific setting** — use the mainline kernel/sysfs interface, not an out-of-tree duplicate.
3. **Not supported natively** — install the smallest reviewed repository component.
4. **Experimental/history only** — never install it from the default onboarding path.

This prevents overlapping power managers, duplicate kernel modules, custom sidecars and desktop-specific hacks.

## Reference hardware profile

The currently validated reference machine reports:

- Huawei product: `WRTB-WXX9`
- Intel Comet Lake-U UHD graphics
- NVIDIA GeForce MX250 / GP108M `10de:1d13`
- Intel CNVi Wi-Fi + Bluetooth
- Intel HDA audio
- IMC Networks UVC HD Camera `13d3:56c6`
- ELAN touchpad / touch / stylus stack
- mainline `huawei_wmi`
- Goodix `GXFP51A0` / GF3658 ST411 fingerprint reader

Other MateBook 13 revisions must be detected by hardware IDs rather than assumed compatible.

## Component policy

| Component | Linux status on the reference model | Repository policy |
| --- | --- | --- |
| Goodix GXFP51A0 fingerprint | Not supported by upstream libfprint | **Repository-managed** native libfprint driver + fprintd integration |
| NVIDIA MX250 | Supported by NVIDIA legacy R580 + PRIME | **Distro-native first**; validate PRIME/runtime PM, never use PCI-remove manager by default |
| Intel iGPU | Native | Verify only |
| Huawei hotkeys / Fn lock / mic-mute LED | Mainline `huawei_wmi` | Verify/load mainline module only |
| Battery charge thresholds | Mainline `huawei_wmi` sysfs | Expose/validate; do not choose user thresholds silently |
| Power profiles | Standard distro stack | Keep one provider such as power-profiles-daemon; do not stack TLP/auto-cpufreq automatically |
| Camera | UVC native | Verify only |
| Wi-Fi / Bluetooth | Intel native | Verify only |
| Touchpad / touch / stylus | HID/I2C native | Verify only |
| Audio | HDA/PipeWire native on the reference unit | Verify only; no codec hacks unless a specific model proves it needs them |
| Suspend / resume | Standard kernel/desktop lifecycle | Verify; fingerprint owns only its device-local lifecycle |
| BIOS / firmware | Vendor-controlled | Never flash automatically |

## Fingerprint checkpoint

The frozen fingerprint checkpoint is **rel71.18**.

See:

- [validated-checkpoint-71.18.md](../fingerprint/docs/validated-checkpoint-71.18.md)
- [recovery-architecture-2026-10-06.md](../fingerprint/docs/recovery-architecture-2026-10-06.md)

The key recovery boundary is target-only:

```text
spidev unbind
→ GPIO264 active-HIGH reset
→ settle
→ spidev rebind
→ libfprint/fprintd
```

No parent SPI controller rebind, GPIO112, firmware flashing, kernel pinning or desktop QML patch is part of the production design.

## Root onboarding command

The repository root installer is intentionally conservative:

```bash
./install.sh
```

It verifies Huawei hardware identity, validates mainline/native components, installs the GXFP51A0 module when present, and finishes with the root hardware doctor.

The root installer must never install the deprecated `gpu-power/` PCI-remove manager.

## Fresh-install contract

A future clean reinstall should not require remembering historical experiments. The public repository must contain everything needed to reconstruct the supported state:

- source;
- one-command installer;
- distro and hardware detection;
- idempotent install/upgrade behavior;
- doctor/validation;
- rollback where the repository replaces a system component;
- public-safe technical rationale;
- a known-good fingerprint checkpoint.

Private handoffs, hostnames, usernames and raw biometric/security material are not part of this contract.

## What remains intentionally distro-owned

The fingerprint path is the most complete cross-distro module today.

The MX250 is Pascal and requires the NVIDIA R580 legacy branch while package names/repositories differ by distribution. The project therefore uses the distribution's standard NVIDIA/PRIME stack when available and will not silently enable third-party repositories.

Likewise, battery thresholds and power profiles are user policy. The repository verifies the mainline Huawei interfaces but does not silently choose a charge range or stack competing power managers.
