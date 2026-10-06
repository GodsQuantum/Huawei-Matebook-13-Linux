#!/usr/bin/env bash
set -Eeuo pipefail

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
PKGDIR="$ROOT/packaging/arch"
KSCREEN_DIR="$ROOT/integration/kscreenlocker-upstream-s3"
PLM_DIR="$ROOT/integration/plasma-login-manager-6.7-pam-messages"
DOCTOR="$ROOT/gxfp51a0-doctor.sh"

DO_ENROLL=1
DO_VERIFY=1
DESKTOP_INTEGRATION=1
LEGACY_PLASMA_PATCHES=0
FINGER="right-index-finger"

usage() {
  cat <<'USAGE'
Usage: ./fingerprint/install-arch.sh [OPTIONS]

Zero-to-working installer for the validated Huawei MateBook 13
Goodix GXFP51A0 / GF3658 ST411 fingerprint reader.

Options:
  --no-enroll                 Do not launch interactive enrollment.
  --no-verify                 Do not launch interactive verification.
  --no-desktop-integration    Skip desktop integration checks.
  --legacy-plasma-patches     Explicitly build/apply the validated Plasma 6.7.5
                              compatibility packages/helpers. Native Plasma
                              fprintd/PAM integration is preferred by default.
  --finger NAME               Finger to enroll (default: right-index-finger).
  -h, --help                  Show this help.
USAGE
}

while (($#)); do
  case "$1" in
    --no-enroll) DO_ENROLL=0 ;;
    --no-verify) DO_VERIFY=0 ;;
    --no-desktop-integration) DESKTOP_INTEGRATION=0 ;;
    --legacy-plasma-patches) LEGACY_PLASMA_PATCHES=1 ;;
    --finger)
      shift
      [[ $# -gt 0 ]] || { echo "ERROR: --finger needs a value" >&2; exit 2; }
      FINGER="$1"
      ;;
    -h|--help) usage; exit 0 ;;
    *) echo "ERROR: unknown argument: $1" >&2; usage >&2; exit 2 ;;
  esac
  shift
done

if (( EUID == 0 )); then
  echo "ERROR: run as your normal user; sudo is used only for system changes." >&2
  exit 2
fi

for cmd in makepkg pacman sudo udevadm systemctl busctl sha256sum; do
  command -v "$cmd" >/dev/null || {
    echo "ERROR: missing required command: $cmd" >&2
    exit 3
  }
done

if ! grep -Rqs '^acpi:GXFP51A0:' /sys/bus/spi/devices/*/modalias 2>/dev/null; then
  echo "ERROR: ACPI/SPI device GXFP51A0 was not found; refusing install." >&2
  exit 4
fi

echo "==> Hardware: GXFP51A0 detected"
echo "==> Installing build/runtime prerequisites"
sudo pacman -S --needed --noconfirm   base-devel git python python-pip pkgconf   glib2 glib2-devel libgusb cairo libgudev openssl pixman   fprintd

echo "==> Building production libfprint-goodix51a0 (rel71 driver core)"
(
  cd "$PKGDIR"
  rm -rf src pkg
  rm -f libfprint-goodix51a0-*.pkg.tar.zst
  makepkg -s -f --noconfirm
)

PKG="$(find "$PKGDIR" -maxdepth 1 -type f   -name 'libfprint-goodix51a0-*.pkg.tar.zst' -print | sort -V | tail -n1)"
[[ -n "$PKG" && -f "$PKG" ]] || {
  echo "ERROR: libfprint package build produced no package." >&2
  exit 5
}

KSCREEN_PKG=""
PLM_PKG=""
if (( DESKTOP_INTEGRATION && LEGACY_PLASMA_PATCHES )); then
  if pacman -Qq kscreenlocker >/dev/null 2>&1; then
    kscreen_ver="$(pacman -Q kscreenlocker | awk '{print $2}')"
    if [[ "$kscreen_ver" == 6.7.5-* ]]; then
      echo "==> Building validated KScreenLocker 6.7.5 resume/PAM integration"
      (
        cd "$KSCREEN_DIR"
        rm -rf src pkg build
        rm -f kscreenlocker-6.7.5-*.pkg.tar.zst
        makepkg -s -f --noconfirm
      )
      KSCREEN_PKG="$(find "$KSCREEN_DIR" -maxdepth 1 -type f         -name 'kscreenlocker-6.7.5-*.pkg.tar.zst' -print | sort -V | tail -n1)"
      [[ -n "$KSCREEN_PKG" && -f "$KSCREEN_PKG" ]] || {
        echo "ERROR: KScreenLocker integration build produced no package." >&2
        exit 5
      }
    else
      echo "INFO: kscreenlocker $kscreen_ver is not 6.7.5; keeping distro package."
    fi
  fi

  if pacman -Qq plasma-login-manager >/dev/null 2>&1; then
    plm_ver="$(pacman -Q plasma-login-manager | awk '{print $2}')"
    if [[ "$plm_ver" == 6.7.5-* ]]; then
      echo "==> Building validated Plasma Login Manager 6.7.5 parallel auth integration"
      (
        cd "$PLM_DIR"
        rm -rf src pkg build
        rm -f plasma-login-manager-6.7.5-*.pkg.tar.zst
        makepkg -s -f --noconfirm
      )
      PLM_PKG="$(find "$PLM_DIR" -maxdepth 1 -type f         -name 'plasma-login-manager-6.7.5-*.pkg.tar.zst' -print | sort -V | tail -n1)"
      [[ -n "$PLM_PKG" && -f "$PLM_PKG" ]] || {
        echo "ERROR: Plasma Login Manager integration build produced no package." >&2
        exit 5
      }
    else
      echo "INFO: plasma-login-manager $plm_ver is not 6.7.5; keeping distro package."
    fi
  fi
fi

echo "==> Installing fingerprint stack"
sudo pacman -U --needed --noconfirm "$PKG"
[[ -z "$KSCREEN_PKG" ]] || sudo pacman -U --needed --noconfirm "$KSCREEN_PKG"
[[ -z "$PLM_PKG" ]] || sudo pacman -U --needed --noconfirm "$PLM_PKG"

# Production invariant: one boot-time prime only. No periodic keepalive and
# no external suspend/resume service.
sudo systemctl disable --now   gxfp51a0-fprintd-suspend.service   gxfp51a0-warm-keepalive.timer   gxfp51a0-warm-keepalive.service 2>/dev/null || true
sudo rm -f   /etc/systemd/system/sleep.target.wants/gxfp51a0-fprintd-suspend.service   /etc/systemd/system/timers.target.wants/gxfp51a0-warm-keepalive.timer

echo "==> Reloading native SPI/fprintd path"
sudo udevadm control --reload
sudo udevadm trigger --subsystem-match=spi
sudo udevadm settle --timeout=3 || true
sudo systemctl daemon-reload
sudo systemctl enable gxfp51a0-boot-prewarm.service 2>/dev/null || true
sudo systemctl restart fprintd.service
sudo systemctl start gxfp51a0-boot-prewarm.service || true

if (( DESKTOP_INTEGRATION && LEGACY_PLASMA_PATCHES )) && [[ -x /usr/libexec/gxfp51a0-kde-lockscreen-integrate ]]; then
  sudo /usr/libexec/gxfp51a0-kde-lockscreen-integrate --apply || true
  sudo /usr/libexec/gxfp51a0-kde-lockscreen-integrate --check || true
fi

if [[ -x "$DOCTOR" ]]; then
  "$DOCTOR" --pre-enroll
fi

if (( DO_ENROLL )); then
  if [[ -t 0 && -t 1 ]]; then
    if ! fprintd-list "$USER" 2>/dev/null | grep -q -- '-finger'; then
      echo
      echo "==> No enrolled fingerprint found. Starting enrollment for $FINGER."
      echo "    Follow the fprintd prompts until enrollment completes."
      fprintd-enroll -f "$FINGER" "$USER"
    else
      echo "==> Existing enrollment found; keeping it unchanged."
    fi
  else
    echo "INFO: non-interactive terminal; skipping enrollment. Run:"
    echo "      fprintd-enroll -f $FINGER $USER"
  fi
fi

if (( DO_VERIFY )) && [[ -t 0 && -t 1 ]]; then
  if fprintd-list "$USER" 2>/dev/null | grep -q -- '-finger'; then
    echo
    echo "==> Final live fingerprint verification"
    fprintd-verify "$USER" || {
      echo "ERROR: fprintd verification failed; installation is not validated." >&2
      exit 8
    }
  fi
fi

if [[ -x "$DOCTOR" ]]; then
  "$DOCTOR"
fi

cat <<EOF2

Installation complete.

Validated path:
  GXFP51A0 -> spidev -> libfprint rel71 core -> fprintd -> PAM/KDE

Password authentication remains available; this installer does not replace it.
Existing enrollments and the validated PMK cache are never deleted.

Desktop integration uses the distribution's native fprintd/PAM support by
default. Plasma 6 lock screens already expose a dedicated fingerprint PAM path.
The historical Plasma 6.7.5 compatibility packages remain available only via
--legacy-plasma-patches; they are not a driver dependency.

Diagnostics:
  $DOCTOR
EOF2
