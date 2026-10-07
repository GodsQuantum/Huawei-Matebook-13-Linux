#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DO_FINGERPRINT=1
DOCTOR_ONLY=0

usage() {
  cat <<'EOF'
Usage: ./install.sh [options]

Native-first Huawei MateBook 13 onboarding.

Options:
  --doctor-only       Do not install repository components; only validate hardware.
  --no-fingerprint    Skip the GXFP51A0 module even when detected.
  -h, --help          Show this help.

The default path never installs the deprecated gpu-power PCI-remove manager and
never changes battery charge thresholds or power profiles without an explicit
future module/option.
EOF
}

for arg in "$@"; do
  case "$arg" in
    --doctor-only) DOCTOR_ONLY=1 ;;
    --no-fingerprint) DO_FINGERPRINT=0 ;;
    -h|--help) usage; exit 0 ;;
    *) printf 'Unknown option: %s\n' "$arg" >&2; usage >&2; exit 2 ;;
  esac
done

vendor="$(cat /sys/class/dmi/id/sys_vendor 2>/dev/null || true)"
product="$(cat /sys/class/dmi/id/product_name 2>/dev/null || true)"
printf '==> Detected: %s %s\n' "$vendor" "$product"

if [[ "$vendor" != "HUAWEI" ]]; then
  printf 'ERROR: refusing automatic MateBook onboarding on non-Huawei hardware.\n' >&2
  exit 3
fi

if (( ! DOCTOR_ONLY )); then
  if modprobe -n huawei_wmi >/dev/null 2>&1 && ! lsmod | grep -q '^huawei_wmi '; then
    if (( EUID == 0 )); then
      modprobe huawei_wmi
    elif command -v sudo >/dev/null 2>&1; then
      sudo modprobe huawei_wmi
    else
      printf 'WARN: huawei_wmi is available but not loaded; no sudo available.\n' >&2
    fi
  fi

  if (( DO_FINGERPRINT )) && [[ -e /sys/bus/spi/devices/spi-GXFP51A0:00 ]]; then
    printf '==> Installing/upgrading GXFP51A0 fingerprint support\n'
    "$ROOT/fingerprint/install.sh"
  else
    printf '==> Fingerprint module skipped (not requested or GXFP51A0 absent)\n'
  fi

  printf '==> Native GPU/power policy\n'
  printf '    Standard distro PRIME + runtime PM only; deprecated gpu-power manager is not installed.\n'
fi

printf '==> Final hardware validation\n'
"$ROOT/matebook13-doctor.sh"
