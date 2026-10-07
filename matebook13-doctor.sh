#!/usr/bin/env bash
set -uo pipefail

fail=0
warn=0

ok()   { printf 'OK   %s\n' "$*"; }
info() { printf 'INFO %s\n' "$*"; }
bad()  { printf 'FAIL %s\n' "$*"; fail=$((fail+1)); }
note() { printf 'WARN %s\n' "$*"; warn=$((warn+1)); }

read1() { [[ -r "$1" ]] && tr -d '\n' < "$1"; }

vendor="$(read1 /sys/class/dmi/id/sys_vendor)"
product="$(read1 /sys/class/dmi/id/product_name)"
version="$(read1 /sys/class/dmi/id/product_version)"

printf 'Huawei MateBook 13 Linux doctor\n'
printf 'Hardware: %s %s %s\n\n' "${vendor:-unknown}" "${product:-unknown}" "${version:-}"

if [[ "$vendor" == "HUAWEI" ]]; then ok "Huawei DMI vendor detected"; else bad "not a Huawei DMI system"; fi

if modprobe -n huawei_wmi >/dev/null 2>&1; then
  ok "mainline huawei_wmi module is available"
else
  note "huawei_wmi module is not available in this kernel"
fi

if [[ -d /sys/devices/platform/huawei-wmi ]]; then
  ok "Huawei WMI sysfs interface is present"
  [[ -r /sys/devices/platform/huawei-wmi/fn_lock_state ]] && ok "Fn-lock interface present" || note "Fn-lock interface not exposed"
  [[ -r /sys/devices/platform/huawei-wmi/charge_control_thresholds ]] && ok "battery charge-threshold interface present" || note "battery threshold interface not exposed"
else
  note "Huawei WMI is not currently instantiated"
fi

if command -v lspci >/dev/null 2>&1; then
  if lspci -Dn | grep -qi '10de:1d13'; then
    ok "NVIDIA MX250 10de:1d13 detected"
    command -v prime-run >/dev/null 2>&1 && ok "PRIME offload launcher present" || note "prime-run is missing"
    command -v nvidia-smi >/dev/null 2>&1 && ok "NVIDIA userspace tools present" || note "nvidia-smi is missing"
  else
    info "reference MX250 PCI ID not present; skipping MX250 checks"
  fi
fi

if [[ -e /sys/bus/spi/devices/spi-GXFP51A0:00 ]]; then
  ok "GXFP51A0 SPI fingerprint device detected"
  command -v fprintd-list >/dev/null 2>&1 && ok "fprintd client installed" || note "fprintd client missing"
  if [[ -x /usr/libexec/gxfp51a0-prestart-recover || -x /usr/local/libexec/gxfp51a0-prestart-recover ]]; then
    ok "GXFP51A0 pre-enumeration recovery helper installed"
  else
    note "GXFP51A0 recovery helper not installed"
  fi
else
  info "GXFP51A0 not present; fingerprint module skipped"
fi

if command -v lsusb >/dev/null 2>&1 && lsusb | grep -qi '13d3:56c6'; then ok "reference UVC camera detected"; else info "reference camera ID not detected"; fi

grep -q 'Huawei WMI hotkeys' /proc/bus/input/devices 2>/dev/null && ok "Huawei WMI hotkeys input device present" || note "Huawei WMI hotkeys input device not seen"
grep -qi 'Touchpad' /proc/bus/input/devices 2>/dev/null && ok "touchpad input device present" || note "touchpad not seen"

if command -v rfkill >/dev/null 2>&1; then
  rfkill list 2>/dev/null | grep -qi 'Wireless LAN' && ok "Wi-Fi rfkill device present" || note "Wi-Fi rfkill device not seen"
  rfkill list 2>/dev/null | grep -qi 'Bluetooth' && ok "Bluetooth rfkill device present" || note "Bluetooth rfkill device not seen"
fi

compgen -G '/dev/video*' >/dev/null && ok "video device node present" || note "no /dev/video* node found"
compgen -G '/dev/snd/*' >/dev/null && ok "audio device nodes present" || note "no ALSA device nodes found"

printf '\nSummary: failures=%d warnings=%d\n' "$fail" "$warn"
(( fail == 0 ))
