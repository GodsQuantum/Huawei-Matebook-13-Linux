#!/usr/bin/env bash
set -Eeuo pipefail

SCRIPT_PATH="$(readlink -f "${BASH_SOURCE[0]}")"
ROOT="$(cd "$(dirname "$SCRIPT_PATH")" && pwd)"
DO_FINGERPRINT=1
DO_GPU=1
DOCTOR_ONLY=0
GPU_DRIVER_PREINSTALLED=0
FORCE_MENU=0
ARGS_PROVIDED=$#
AUDIT_FILE=""

usage() {
  cat <<'EOF'
Usage: ./install.sh [options]

One-command, native-first Huawei MateBook 13 Linux onboarding.

With no arguments in an interactive terminal, the installer opens the Whiptail
hardware audit/menu. Non-interactive execution keeps the historical full-install
behavior for automation and CI.

Options:
  --menu                     Force the Whiptail audit/install menu.
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
    --menu) FORCE_MENU=1 ;;
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

fingerprint_present() {
  [[ -e /sys/bus/spi/devices/spi-GXFP51A0:00 ]]
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

fingerprint_expected_version() {
  local pkgbuild="$ROOT/fingerprint/packaging/arch/PKGBUILD" pkgver pkgrel portable
  if command -v pacman >/dev/null 2>&1 && [[ -r "$pkgbuild" ]]; then
    pkgver="$(sed -n 's/^pkgver=//p' "$pkgbuild" | head -n1)"
    pkgrel="$(sed -n 's/^pkgrel=//p' "$pkgbuild" | head -n1)"
    printf '%s-%s\n' "$pkgver" "$pkgrel"
    return
  fi
  portable="$(sed -n 's/^PORTABLE_RELEASE="\(.*\)"/\1/p' "$ROOT/fingerprint/install-linux.sh" | head -n1)"
  printf '%s\n' "${portable:-rel71.24}"
}

fingerprint_installed_version() {
  if command -v pacman >/dev/null 2>&1; then
    pacman -Q libfprint-goodix51a0 2>/dev/null | awk '{print $2}' || true
  elif [[ -r /var/lib/gxfp51a0-local-install/release ]]; then
    cat /var/lib/gxfp51a0-local-install/release 2>/dev/null || true
  fi
}

fingerprint_status_code() {
  fingerprint_present || { echo absent; return; }
  local expected installed
  expected="$(fingerprint_expected_version)"
  installed="$(fingerprint_installed_version)"
  [[ -z "$installed" ]] && { echo missing; return; }
  [[ "$installed" == "$expected" ]] && { echo current; return; }
  echo outdated
}

fingerprint_status_text() {
  local code expected installed
  code="$(fingerprint_status_code)"
  expected="$(fingerprint_expected_version)"
  installed="$(fingerprint_installed_version)"
  case "$code" in
    absent) printf 'not detected on this machine' ;;
    missing) printf 'not installed — repository version %s' "$expected" ;;
    current) printf 'current — %s' "$installed" ;;
    outdated) printf 'update available — %s -> %s' "${installed:-unknown}" "$expected" ;;
  esac
}

gpu_expected_version() {
  sed -n 's/^VERSION="\(.*\)"/\1/p' "$ROOT/gpu-power/huawei-matebook-13-gpu-manager.sh" | head -n1
}

gpu_installed_version() {
  if [[ -r /etc/huawei-matebook-gpu-manager.conf ]]; then
    sed -n 's/^MANAGER_VERSION="\(.*\)"/\1/p' /etc/huawei-matebook-gpu-manager.conf | head -n1
  fi
}

gpu_status_code() {
  mx250_present || { echo absent; return; }
  local expected installed
  expected="$(gpu_expected_version)"
  installed="$(gpu_installed_version)"
  [[ -z "$installed" ]] && { echo missing; return; }
  if [[ "$installed" == "$expected" &&
        -x /usr/local/sbin/huawei-matebook-dgpu-power &&
        -x /usr/local/bin/huawei-matebook-dgpu-run ]]; then
    echo current
  else
    echo outdated
  fi
}

gpu_status_text() {
  local code expected installed
  code="$(gpu_status_code)"
  expected="$(gpu_expected_version)"
  installed="$(gpu_installed_version)"
  case "$code" in
    absent) printf 'reference MX250 not detected' ;;
    missing) printf 'not installed — repository version %s' "$expected" ;;
    current) printf 'current — GPU Manager %s' "$installed" ;;
    outdated) printf 'install/update required — %s -> %s' "${installed:-none}" "$expected" ;;
  esac
}

ensure_whiptail() {
  command -v whiptail >/dev/null 2>&1 && return 0
  printf '==> Installing the small Whiptail TUI dependency\n'
  if command -v pacman >/dev/null 2>&1; then
    run_root pacman -S --needed --noconfirm libnewt
  elif command -v apt-get >/dev/null 2>&1; then
    run_root apt-get update
    run_root env DEBIAN_FRONTEND=noninteractive apt-get install -y whiptail
  elif command -v dnf >/dev/null 2>&1; then
    run_root dnf install -y newt
  elif command -v zypper >/dev/null 2>&1; then
    run_root zypper --non-interactive install newt
  elif command -v apk >/dev/null 2>&1; then
    run_root apk add --no-cache newt
  fi
  command -v whiptail >/dev/null 2>&1
}

install_user_launcher() {
  (( EUID == 0 )) && return 0
  local bindir="$HOME/.local/bin" launcher="$HOME/.local/bin/HUAWEI"
  mkdir -p "$bindir"
  ln -sfn "$ROOT/install.sh" "$launcher"
  if [[ ":$PATH:" != *":$bindir:"* ]]; then
    printf 'INFO: HUAWEI launcher installed at %s; add %s to PATH if needed.\n' "$launcher" "$bindir"
  else
    printf '==> CLI launcher ready: HUAWEI\n'
  fi
}

prepare_menu_audit() {
  AUDIT_FILE="$(mktemp)"
  "$ROOT/matebook13-doctor.sh" >"$AUDIT_FILE" 2>&1 || true
}

cleanup_menu_audit() {
  [[ -n "${AUDIT_FILE:-}" ]] && rm -f "$AUDIT_FILE"
}

audit_summary() {
  local fp gpu summary
  fp="$(fingerprint_status_text)"
  gpu="$(gpu_status_text)"
  summary="$(tail -n1 "$AUDIT_FILE" 2>/dev/null || true)"
  cat <<EOF
Detected: ${vendor:-unknown} ${product:-unknown}

Fingerprint GXFP51A0: $fp
MX250 / GPU Manager: $gpu
Hardware doctor: ${summary:-see detailed audit}

The menu is read-only until you choose an install/update action.
EOF
}

show_audit_box() {
  whiptail --title "Huawei MateBook 13 — hardware audit"     --scrolltext --textbox "$AUDIT_FILE" 28 100
}

show_shortcuts_box() {
  local help_file
  help_file="$(mktemp)"
  cat >"$help_file" <<'EOF'
HUAWEI MATEBOOK 13 LINUX — COMMANDS / SHORTCUTS

MAIN CONTROL CENTER
  HUAWEI
    Open the audit-first Whiptail control center.

  HUAWEI --doctor-only
    Read-only hardware/software audit. Changes nothing.

  HUAWEI --menu
    Force the Whiptail menu. Requires a real interactive terminal.

  HUAWEI --no-fingerprint
    Run onboarding but skip fingerprint installation/update.

  HUAWEI --no-gpu
    Run onboarding but skip MX250 GPU Manager installation/update.

  HUAWEI --gpu-driver-preinstalled
    Never install an NVIDIA package; require a compatible R580 driver already
    provided by the distribution.

GPU MANAGER
  GPU-control
    Show the GPU Manager overview/dashboard.

  GPU-control menu
    Open the GPU Manager's own interactive terminal menu.

  GPU-control overview
    Explicit alias for the same overview/dashboard.

  GPU-control status
    Detailed current MX250, NVIDIA, lease, user and PCI power state.

  GPU-control list
    List desktop applications and Steam apps currently managed for NVIDIA.

  GPU-control run -- COMMAND [ARGS...]
    ONE-OFF NVIDIA launch. The MX250 is enabled only for that command and is
    released/powered down when the command exits.
    Example:
      GPU-control run -- handy

  GPU-control add APP.desktop
    ALWAYS use the MX250 when that desktop application is launched normally.
    The manager creates a reversible local .desktop wrapper.
    Example:
      GPU-control add Handy.desktop

  GPU-control remove APP.desktop
    Remove the permanent NVIDIA wrapper and restore the application's normal
    Intel/default launch behavior.
    Example:
      GPU-control remove Handy.desktop

  GPU-control add
    Interactive terminal picker for a desktop application.

  GPU-control steam-add APPID
    Enable on-demand NVIDIA for one Steam game.

  GPU-control steam-remove APPID
    Remove on-demand NVIDIA for one Steam game.

  GPU-control steam-all-on
    Enable on-demand NVIDIA for all detected Steam games.

  GPU-control steam-all-off
    Disable that all-games Steam policy.

  GPU-control apply
    Reapply all saved desktop/Steam GPU routing rules.

  GPU-control install
  GPU-control repair
  GPU-control upgrade
    Install or transactionally repair/upgrade GPU Manager itself.

  GPU-control doctor
    Read-only GPU Manager integrity/configuration audit.

  GPU-control test
    Full reversible MX250 smoke test:
      wake PCI -> load NVIDIA -> run test -> release -> return to full idle.

  GPU-control uninstall
    Remove GPU Manager integration and restore saved application desktop state.

  GPU-control --lang en|fr|zh COMMAND
    Select GPU Manager CLI/menu language for the command.

  GPU-control --no-driver-install install
    Install/repair GPU Manager only if NVIDIA R580 is already present.

HOW APPLICATION DETECTION WORKS
  GPU Manager does NOT guess which applications "look GPU-heavy".

  During install/repair it can import applications whose .desktop explicitly
  asks for the discrete GPU with:
    PrefersNonDefaultGPU=true
    X-KDE-RunOnDiscreteGpu=true

  Other applications stay on Intel by default so simple Vulkan enumeration
  cannot keep the MX250 powered for no reason.

  Use:
    GPU-control run -- app
  for an occasional NVIDIA launch, or:
    GPU-control add App.desktop
  to make that application use NVIDIA whenever launched from the desktop/menu.

CURRENT RECOMMENDATION FOR HANDY
  Keep Handy on Intel at autostart.
  Use:
    GPU-control run -- handy
  when you explicitly want Handy on the MX250.

All GPU Manager application changes are reversible.
EOF
  whiptail --title "Huawei MateBook 13 — help / shortcuts"     --scrolltext --textbox "$help_file" 30 108
  rm -f "$help_file"
}

interactive_menu() {
  ensure_whiptail || {
    printf 'ERROR: Whiptail could not be installed; rerun with explicit CLI flags.\n' >&2
    exit 4
  }

  prepare_menu_audit
  trap cleanup_menu_audit EXIT

  whiptail --title "Huawei MateBook 13 Linux"     --msgbox "$(audit_summary)" 20 100

  while true; do
    local choice fp gpu
    fp="$(fingerprint_status_text)"
    gpu="$(gpu_status_text)"
    choice="$(whiptail --title "Huawei MateBook 13 Linux"       --menu "Audit complete. Choose what to install, update or repair."       23 110 8       "RECOMMENDED" "Apply only missing/outdated detected components"       "FINGERPRINT" "Fingerprint — $fp"       "GPU" "MX250 — $gpu"       "ALL" "Install/update/repair all detected repository components"       "AUDIT" "Show the detailed read-only hardware audit"       "SHORTCUTS" "Explain HUAWEI and GPU-control commands with examples"       "EXIT" "Quit without changing the system"       3>&1 1>&2 2>&3)" || exit 0

    case "$choice" in
      AUDIT)
        show_audit_box
        ;;
      SHORTCUTS)
        show_shortcuts_box
        ;;
      EXIT)
        exit 0
        ;;
      RECOMMENDED)
        DO_FINGERPRINT=0
        DO_GPU=0
        case "$(fingerprint_status_code)" in missing|outdated) DO_FINGERPRINT=1 ;; esac
        case "$(gpu_status_code)" in missing|outdated) DO_GPU=1 ;; esac
        if (( ! DO_FINGERPRINT && ! DO_GPU )); then
          whiptail --title "Huawei MateBook 13 Linux"             --msgbox "Everything managed by this repository is already current.\n\nUse Fingerprint or GPU explicitly if you want to run a repair/idempotence pass." 12 82
          continue
        fi
        break
        ;;
      FINGERPRINT)
        if ! fingerprint_present; then
          whiptail --title "Fingerprint" --msgbox "GXFP51A0 is not detected on this machine." 10 70
          continue
        fi
        DO_FINGERPRINT=1
        DO_GPU=0
        break
        ;;
      GPU)
        if ! mx250_present; then
          whiptail --title "GPU" --msgbox "The reference MX250 (10de:1d13) is not detected on this machine." 10 76
          continue
        fi
        DO_FINGERPRINT=0
        DO_GPU=1
        break
        ;;
      ALL)
        DO_FINGERPRINT=1
        DO_GPU=1
        break
        ;;
    esac
  done

  cleanup_menu_audit
  trap - EXIT
}

perform_selected_actions() {
  ensure_huawei_wmi
  ensure_power_profile_api

  if (( DO_FINGERPRINT )) && fingerprint_present; then
    printf '==> Installing/upgrading GXFP51A0 fingerprint support\n'
    "$ROOT/fingerprint/install.sh"
  else
    printf '==> Fingerprint module skipped (not requested or GXFP51A0 absent)\n'
  fi

  if (( DO_GPU )) && mx250_present; then
    printf '==> Installing/upgrading MX250/Pascal on-demand GPU power management\n'
    local gpu_args=(--yes)
    (( GPU_DRIVER_PREINSTALLED )) && gpu_args+=(--no-driver-install)
    gpu_args+=(install)
    "$ROOT/gpu-power/huawei-matebook-13-gpu-manager.sh" "${gpu_args[@]}"
  else
    printf '==> GPU module skipped (not requested or reference MX250 10de:1d13 absent)\n'
  fi
}

if (( FORCE_MENU )); then
  if [[ ! -t 0 || ! -t 1 ]]; then
    printf 'ERROR: --menu requires an interactive terminal. Use explicit CLI flags for automation.\n' >&2
    exit 4
  fi
  interactive_menu
elif (( ARGS_PROVIDED == 0 && ! DOCTOR_ONLY )) && [[ -t 0 && -t 1 ]]; then
  interactive_menu
fi

if (( ! DOCTOR_ONLY )); then
  perform_selected_actions
  install_user_launcher
fi

printf '==> Final read-only hardware validation\n'
"$ROOT/matebook13-doctor.sh"
