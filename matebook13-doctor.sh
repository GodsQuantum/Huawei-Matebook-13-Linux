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
printf 'Hardware: %s %s %s\n' "${vendor:-unknown}" "${product:-unknown}" "${version:-}"
printf 'Kernel: %s\n\n' "$(uname -r)"

[[ "${vendor^^}" == *HUAWEI* ]] && ok "Huawei DMI vendor detected" || bad "not a Huawei DMI system"

if modprobe -n huawei_wmi >/dev/null 2>&1; then
  ok "mainline huawei_wmi module is available"
else
  note "huawei_wmi module is not available in this kernel"
fi

if lsmod | grep -q '^huawei_wmi '; then
  ok "huawei_wmi module is loaded"
elif [[ -d /sys/devices/platform/huawei-wmi ]]; then
  ok "huawei_wmi platform interface is active (built-in or already instantiated)"
else
  note "huawei_wmi is not active"
fi

if [[ -d /sys/devices/platform/huawei-wmi ]]; then
  ok "Huawei WMI sysfs interface is present"
  [[ -r /sys/devices/platform/huawei-wmi/fn_lock_state ]] && ok "Fn-lock interface present" || note "Fn-lock interface not exposed"
  if [[ -r /sys/devices/platform/huawei-wmi/charge_control_thresholds ]] ||
     [[ -r /sys/class/power_supply/BAT0/charge_control_end_threshold ]]; then
    ok "battery charge-threshold interface present"
  else
    note "battery threshold interface not exposed"
  fi
else
  note "Huawei WMI is not currently instantiated"
fi

if [[ -r /sys/devices/system/cpu/cpu0/cpufreq/scaling_driver ]]; then
  cpu_driver="$(read1 /sys/devices/system/cpu/cpu0/cpufreq/scaling_driver)"
  case "$cpu_driver" in
    intel_pstate|intel_cpufreq) ok "native Intel CPU frequency driver active: $cpu_driver" ;;
    *) info "CPU frequency driver: $cpu_driver" ;;
  esac
fi

if command -v powerprofilesctl >/dev/null 2>&1; then
  profile="$(powerprofilesctl get 2>/dev/null || true)"
  ok "desktop power-profile API available${profile:+ (current: $profile)}"
else
  note "no powerprofilesctl-compatible desktop power-profile API"
fi

active_power_managers=()
systemctl is-active --quiet power-profiles-daemon.service 2>/dev/null && active_power_managers+=(power-profiles-daemon)
systemctl is-active --quiet tuned.service 2>/dev/null && active_power_managers+=(tuned)
systemctl is-active --quiet tlp.service 2>/dev/null && active_power_managers+=(tlp)
systemctl is-active --quiet auto-cpufreq.service 2>/dev/null && active_power_managers+=(auto-cpufreq)
if (( ${#active_power_managers[@]} > 1 )); then
  note "multiple platform/CPU power managers active: ${active_power_managers[*]}"
elif (( ${#active_power_managers[@]} == 1 )); then
  ok "single platform/CPU power manager active: ${active_power_managers[0]}"
fi

if [[ -r /sys/power/mem_sleep ]]; then
  sleep_modes="$(cat /sys/power/mem_sleep)"
  [[ "$sleep_modes" == *deep* ]] && ok "deep suspend is available: $sleep_modes" || info "suspend modes: $sleep_modes"
fi

if command -v lspci >/dev/null 2>&1; then
  intel_line="$(lspci -Dn 2>/dev/null | grep -Ei '(0300|0302):[[:space:]]+8086:' | head -n1 || true)"
  [[ -n "$intel_line" ]] && ok "Intel integrated GPU detected" || note "Intel integrated GPU not detected"

  if lspci -Dn 2>/dev/null | grep -qi '10de:1d13'; then
    ok "NVIDIA MX250 / GP108M 10de:1d13 detected"
    nvidia_ver="$(modinfo -F version nvidia 2>/dev/null | head -n1 || true)"
    if [[ "$nvidia_ver" == 580.* ]]; then
      ok "NVIDIA R580 legacy driver available: $nvidia_ver"
    elif [[ -n "$nvidia_ver" ]]; then
      note "NVIDIA driver is $nvidia_ver; Pascal requires the 580 legacy branch"
    else
      note "NVIDIA R580 kernel module not available"
    fi

    if [[ -r /etc/huawei-matebook-gpu-manager.conf ]] &&
       [[ -x /usr/local/sbin/huawei-matebook-dgpu-power ]] &&
       [[ -x /usr/local/bin/huawei-matebook-dgpu-run ]]; then
      ok "MX250 on-demand PCI power manager installed"
      if command -v GPU-control >/dev/null 2>&1 || [[ -x "$HOME/.local/bin/GPU-control" ]]; then
        ok "GPU-control user CLI installed"
      else
        note "GPU-control user CLI missing"
      fi
      if [[ -r "$HOME/.config/environment.d/61-huawei-matebook-intel-default.conf" ]]; then
        if grep -Fqx 'VK_LOADER_DRIVERS_SELECT=*intel*' "$HOME/.config/environment.d/61-huawei-matebook-intel-default.conf" &&
           grep -Fqx '__GLX_VENDOR_LIBRARY_NAME=mesa' "$HOME/.config/environment.d/61-huawei-matebook-intel-default.conf"; then
          ok "ordinary DRI/Vulkan/GLX applications are pinned to Intel/Mesa by default"
        else
          note "Intel-default graphics environment file is incomplete"
        fi
      else
        note "Intel-default GPU environment file is missing"
      fi
    else
      note "MX250 Pascal cannot use modern NVIDIA RTD3; repository PCI power-gating layer is not installed"
    fi

    users="$(for f in /dev/nvidia0 /dev/nvidiactl /dev/nvidia-modeset /dev/nvidia-uvm /dev/nvidia-uvm-tools /dev/nvidia-caps/*; do
      [[ -e "$f" ]] || continue
      fuser "$f" 2>/dev/null || true
    done | tr ' ' '\n' | grep -E '^[0-9]+$' | sort -u | paste -sd, -)"
    if [[ -n "$users" ]]; then
      info "NVIDIA is currently in use by PID(s): $users"
    elif lspci -Dn 2>/dev/null | grep -qi '10de:1d13'; then
      info "MX250 is enumerated but no users currently hold NVIDIA device nodes"
    fi
  else
    if [[ -r /etc/huawei-matebook-gpu-manager.conf ]]; then
      ok "MX250 is absent from PCI: full Integrated idle"
    else
      info "reference MX250 not currently enumerated/present"
    fi
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
grep -qi 'Stylus' /proc/bus/input/devices 2>/dev/null && ok "stylus input device present" || info "stylus input device not seen"

if command -v rfkill >/dev/null 2>&1; then
  rfkill list 2>/dev/null | grep -qi 'Wireless LAN' && ok "Wi-Fi rfkill device present" || note "Wi-Fi rfkill device not seen"
  rfkill list 2>/dev/null | grep -qi 'Bluetooth' && ok "Bluetooth rfkill device present" || note "Bluetooth rfkill device not seen"
fi

compgen -G '/dev/video*' >/dev/null && ok "video device node present" || note "no /dev/video* node found"
compgen -G '/dev/snd/*' >/dev/null && ok "audio device nodes present" || note "no ALSA device nodes found"

command -v fwupdmgr >/dev/null 2>&1 && ok "fwupd available for distro/vendor firmware updates" || info "fwupd not installed; firmware remains distro/vendor-managed"

printf '\nSummary: failures=%d warnings=%d\n' "$fail" "$warn"
(( fail == 0 ))
