#!/usr/bin/env bash
set -Eeuo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DO_FINGERPRINT=1
DO_GPU=1
DOCTOR_ONLY=0
GPU_DRIVER_PREINSTALLED=0

usage() {
  cat <<'EOF'
Usage: ./install.sh [options]

One-command, native-first Huawei MateBook 13 Linux onboarding.

Options:
  --doctor-only              Read-only hardware/software audit.
  --no-fingerprint           Skip GXFP51A0 installation even when detected.
  --no-gpu                   Skip MX250 on-demand power/offload installation.
  --gpu-driver-preinstalled  Never install NVIDIA packages; require R580 already present.
  -h, --help                 Show this help.

Default policy:
  - use mainline huawei_wmi for Huawei hotkeys, Fn-lock and battery thresholds;
  - keep the distro's normal desktop power-profile API;
  - install the repository driver only for the unsupported GXFP51A0 fingerprint reader;
  - on the validated MX250/Pascal model, install the repository's on-demand
    PCI power-gating layer because NVIDIA RTD3 is not available for Pascal;
  - never choose battery charge thresholds, flash firmware, replace the kernel,
    or stack multiple CPU/platform power managers.
EOF
}

for arg in "$@"; do
  case "$arg" in
    --doctor-only) DOCTOR_ONLY=1 ;;
    --no-fingerprint) DO_FINGERPRINT=0 ;;
    --no-gpu) DO_GPU=0 ;;
    --gpu-driver-preinstalled) GPU_DRIVER_PREINSTALLED=1 ;;
    -h|--help) usage; exit 0 ;;
    *) printf 'Unknown option: %s\n' "$arg" >&2; usage >&2; exit 2 ;;
  esac
done

if (( ! DOCTOR_ONLY && EUID == 0 )); then
  printf 'ERROR: run ./install.sh as the normal desktop user, not with sudo.\n' >&2
  exit 2
fi

vendor="$(cat /sys/class/dmi/id/sys_vendor 2>/dev/null || true)"
product="$(cat /sys/class/dmi/id/product_name 2>/dev/null || true)"
printf '==> Detected: %s %s\n' "${vendor:-unknown}" "${product:-unknown}"

if [[ "${vendor^^}" != *HUAWEI* ]]; then
  printf 'ERROR: refusing automatic MateBook onboarding on non-Huawei hardware.\n' >&2
  exit 3
fi

run_root() {
  if (( EUID == 0 )); then "$@"
  elif command -v sudo >/dev/null 2>&1; then sudo "$@"
  else
    printf 'ERROR: root privileges are required for: %q' "$1" >&2
    printf ' %q' "${@:2}" >&2
    printf '\n' >&2
    return 1
  fi
}

ensure_huawei_wmi() {
  if [[ -d /sys/devices/platform/huawei-wmi ]]; then
    printf '==> Mainline huawei_wmi interface already active\n'
    return 0
  fi
  if ! modprobe -n huawei_wmi >/dev/null 2>&1; then
    printf 'WARN: this kernel does not provide mainline huawei_wmi.\n' >&2
    return 0
  fi
  if ! lsmod | grep -q '^huawei_wmi '; then
    printf '==> Loading mainline huawei_wmi\n'
    run_root modprobe huawei_wmi
  fi
}

ensure_power_profile_api() {
  if command -v powerprofilesctl >/dev/null 2>&1; then
    printf '==> Desktop power-profile API already available\n'
    return 0
  fi

  printf '==> Installing the distro-native desktop power-profile provider\n'
  if command -v pacman >/dev/null 2>&1; then
    run_root pacman -S --needed --noconfirm power-profiles-daemon
  elif command -v dnf >/dev/null 2>&1; then
    # Fedora 41+ uses tuned-ppd as the desktop-compatible PPD API provider.
    run_root dnf install -y tuned-ppd
  elif command -v apt-get >/dev/null 2>&1; then
    run_root apt-get update
    run_root env DEBIAN_FRONTEND=noninteractive apt-get install -y power-profiles-daemon
  elif command -v zypper >/dev/null 2>&1; then
    run_root zypper --non-interactive install power-profiles-daemon
  else
    printf 'WARN: no known native power-profile package path for this distribution; leaving power policy untouched.\n' >&2
  fi
}

mx250_present() {
  # Once the power manager is working, its desired idle state deliberately
  # removes the MX250 from the PCI tree. Treat the installed hardware identity
  # as authoritative for upgrade/repair detection in that state.
  if [[ -r /etc/huawei-matebook-gpu-manager.conf ]] &&
     grep -Fqx 'DGPU_VENDOR="0x10de"' /etc/huawei-matebook-gpu-manager.conf &&
     grep -Fqx 'DGPU_DEVICE="0x1d13"' /etc/huawei-matebook-gpu-manager.conf; then
    return 0
  fi
  if command -v lspci >/dev/null 2>&1; then
    lspci -Dn 2>/dev/null | grep -qi '10de:1d13'
  else
    grep -Rilqx '0x1d13' /sys/bus/pci/devices/*/device 2>/dev/null
  fi
}

if (( ! DOCTOR_ONLY )); then
  ensure_huawei_wmi
  ensure_power_profile_api

  if (( DO_FINGERPRINT )) && [[ -e /sys/bus/spi/devices/spi-GXFP51A0:00 ]]; then
    printf '==> Installing/upgrading GXFP51A0 fingerprint support\n'
    "$ROOT/fingerprint/install.sh"
  else
    printf '==> Fingerprint module skipped (not requested or GXFP51A0 absent)\n'
  fi

  if (( DO_GPU )) && mx250_present; then
    printf '==> Installing/upgrading MX250/Pascal on-demand GPU power management\n'
    gpu_args=(--yes)
    (( GPU_DRIVER_PREINSTALLED )) && gpu_args+=(--no-driver-install)
    gpu_args+=(install)
    "$ROOT/gpu-power/huawei-matebook-13-gpu-manager.sh" "${gpu_args[@]}"
  else
    printf '==> GPU module skipped (not requested or reference MX250 10de:1d13 absent)\n'
  fi
fi

printf '==> Final read-only hardware validation\n'
"$ROOT/matebook13-doctor.sh"
