# Huawei MateBook 13 WRTB-WXX9 — audited Linux support matrix

Audit date: 2026-10-07

This document records why the root installer manages some components and deliberately leaves others to the distribution.

## Matrix

| Hardware / feature | Reference hardware | Linux / upstream state | Repo action |
| --- | --- | --- | --- |
| Platform extras | Huawei WRTB-WXX9 | `huawei_wmi` mainline | Verify/load only |
| Fn-lock | Huawei WMI | Mainline sysfs | Verify only |
| Battery thresholds | Huawei WMI | Mainline power-supply / Huawei WMI sysfs | Verify only; user chooses policy |
| Mic-mute / hotkeys | Huawei WMI input/LED | Mainline | Verify only |
| Intel graphics | 8086:9b41 | i915 | Verify only |
| NVIDIA dGPU | 10de:1d13 MX250 Pascal | PRIME works; NVIDIA RTD3 is Turing+ | Repo-managed on-demand PCI power gate |
| Fingerprint | GXFP51A0 / GF3658 ST411 | Not upstream libfprint | Repo-managed rel71.24 |
| Wi-Fi | Intel CNVi 8086:02f0 | iwlwifi | Verify only |
| Bluetooth | Intel 8087:0aaa | btusb/Intel firmware stack | Verify only |
| Camera | IMC 13d3:56c6 | UVC | Verify only |
| Touchpad | ELAN962C 04f3:3109 | HID/I2C input | Verify only |
| Touch / stylus | ELAN224A 04f3:2841 | HID/I2C input | Verify only |
| Audio | Intel HDA 8086:02c8 | ALSA + normal PipeWire desktop stack | Verify only |
| Suspend | platform deep sleep available | kernel/systemd | Verify; no repo-wide sleep hook |
| Firmware | Huawei UEFI + device firmware | distro/fwupd/vendor | Never automatic |

## Important GPU conclusion

The MX250 is GP108M/Pascal. NVIDIA's Linux README for PCIe Runtime D3 requires a Turing-or-newer GPU. Consequently, a design that merely enables `power/control=auto` and keeps the NVIDIA stack available cannot promise a fully powered-down MX250 on this reference machine.

The repository's power manager therefore uses a bounded and reversible sequence:

- Intel-only normal session defaults;
- NVIDIA R580 loaded only for selected dGPU applications;
- PRIME render offload for the application;
- explicit refusal to power down while any NVIDIA device node is owned;
- Intel/Mesa defaults for ordinary DRI/Vulkan/GLX/EGL clients, with NVIDIA overrides only inside the managed runner;
- unload NVIDIA;
- remove only the MX250 PCI function;
- PCI rescan on the next managed dGPU launch.

This is intentionally narrower than generic Optimus mode-switching tools and does not replace the desktop or the Intel graphics stack.

## Reference-machine observations

The audited reference unit reports:

- DMI product `WRTB-WXX9`
- BIOS 1.26
- Intel UHD 8086:9b41 using i915
- MX250 10de:1d13 / GP108M using NVIDIA 580.178.04
- NVIDIA RTD3 reported disabled
- `huawei_wmi` loaded
- Fn-lock sysfs present
- Huawei battery charge-control sysfs present
- UVC camera 13d3:56c6 present
- Intel Wi-Fi and Bluetooth present
- ELAN touchpad/touch/stylus present
- deep suspend selected
- one active high-level power-profile provider

These observations are validation evidence for the hardware profile, not hard-coded private machine state.

## Upstream references

- Linux mainline HUAWEI_WMI Kconfig:
  https://github.com/torvalds/linux/blob/master/drivers/platform/x86/Kconfig
- Huawei-WMI upstream-development README:
  https://github.com/aymanbagabas/Huawei-WMI
- Linux power-supply sysfs ABI:
  https://github.com/torvalds/linux/blob/master/Documentation/ABI/testing/sysfs-class-power
- NVIDIA R580 PCIe Runtime D3 documentation:
  https://download.nvidia.com/XFree86/Linux-x86_64/580.178.04/README/dynamicpowermanagement.html
- NVIDIA Unix legacy support timeframes (580.* last for Maxwell/Pascal/Volta):
  https://nvidia.custhelp.com/app/answers/detail/a_id/3142
- NVIDIA R580 driver archive:
  https://download.nvidia.com/XFree86/Linux-x86_64/580.178.04/
- NVIDIA PRIME Render Offload documentation:
  https://download.nvidia.com/XFree86/Linux-x86_64/580.178.04/README/
- Linux VGA Switcheroo documentation:
  https://dri.freedesktop.org/docs/drm/gpu/vga-switcheroo.html

## Non-goals

The repository does not:

- install an out-of-tree Huawei WMI driver when the kernel already provides it;
- replace i915, iwlwifi, btusb, UVC, HID/I2C or ALSA/PipeWire;
- choose charge limits;
- stack TLP + auto-cpufreq + tuned + power-profiles-daemon;
- flash firmware;
- force a kernel version;
- keep background browser/sidecar processes solely for hardware management.
