#!/usr/bin/env bash
set -Eeuo pipefail

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
PKGDIR="$ROOT/packaging/arch"
KSCREEN_DIR="$ROOT/integration/kscreenlocker-upstream-s3"
PLM_DIR="$ROOT/integration/plasma-login-manager-6.7-pam-messages"
DOCTOR="$ROOT/gxfp51a0-doctor.sh"

DESKTOP_INTEGRATION=1
LEGACY_PLASMA_PATCHES=0
usage() {
  cat <<'USAGE'
Usage: ./fingerprint/install-arch.sh [OPTIONS]

Driver-only installer for the Huawei MateBook 13
Goodix GXFP51A0 / GF3658 ST411 fingerprint reader.

Options:
  --no-enroll                 Deprecated compatibility flag (GUI enrollment is always used).
  --no-verify                 Deprecated compatibility flag (GUI verification is always used).
  --no-desktop-integration    Skip desktop integration checks.
  --legacy-plasma-patches     Explicitly build/apply the validated Plasma 6.7.5
                              compatibility packages/helpers. Native Plasma
                              fprintd/PAM integration is preferred by default.
  --finger NAME               Deprecated compatibility argument (never initiates CLI enrollment).
  -h, --help                  Show this help.
USAGE
}

while (($#)); do
  case "$1" in
    --no-enroll|--no-verify) : ;;
    --no-desktop-integration) DESKTOP_INTEGRATION=0 ;;
    --legacy-plasma-patches) LEGACY_PLASMA_PATCHES=1 ;;
    --finger)
      shift
      [[ $# -gt 0 ]] || { echo "ERROR: --finger needs a value" >&2; exit 2; }
      # Legacy argument accepted but enrollment is intentionally GUI-only.
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

expected_pkgver="$(sed -n 's/^pkgver=//p' "$PKGDIR/PKGBUILD" | head -n1)"
expected_pkgrel="$(sed -n 's/^pkgrel=//p' "$PKGDIR/PKGBUILD" | head -n1)"
expected_version="$expected_pkgver-$expected_pkgrel"
installed_version="$(pacman -Q libfprint-goodix51a0 2>/dev/null | awk '{print $2}' || true)"
DRIVER_CHANGED=1
PKG=""

if [[ "$installed_version" == "$expected_version" ]]; then
  DRIVER_CHANGED=0
  echo "==> GXFP51A0 driver already exact: $installed_version"
  echo "    Preserving the live fprintd/TLS session; no rebuild or sensor restart."
else
  echo "==> Installing build/runtime prerequisites"
  sudo pacman -S --needed --noconfirm \
    base-devel git python python-pip pkgconf \
    glib2 glib2-devel libgusb cairo libgudev openssl pixman \
    fprintd

  echo "==> Building production libfprint-goodix51a0 (rel71 driver core)"
  (
    cd "$PKGDIR"
    rm -rf src pkg
    rm -f libfprint-goodix51a0-*.pkg.tar.zst
    makepkg -s -f --noconfirm
  )

  PKG="$(find "$PKGDIR" -maxdepth 1 -type f \
    -name 'libfprint-goodix51a0-*.pkg.tar.zst' -print | sort -V | tail -n1)"
  [[ -n "$PKG" && -f "$PKG" ]] || {
    echo "ERROR: libfprint package build produced no package." >&2
    exit 5
  }
fi

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

if (( DRIVER_CHANGED )); then
  echo "==> Installing fingerprint stack"
  sudo pacman -U --needed --noconfirm "$PKG"
fi
[[ -z "$KSCREEN_PKG" ]] || sudo pacman -U --needed --noconfirm "$KSCREEN_PKG"
[[ -z "$PLM_PKG" ]] || sudo pacman -U --needed --noconfirm "$PLM_PKG"

# Production invariant: one boot-time prime only. No periodic keepalive and
# no external suspend/resume service.
sudo systemctl disable --now   gxfp51a0-fprintd-suspend.service   gxfp51a0-warm-keepalive.timer   gxfp51a0-warm-keepalive.service 2>/dev/null || true
sudo rm -f   /etc/systemd/system/sleep.target.wants/gxfp51a0-fprintd-suspend.service   /etc/systemd/system/timers.target.wants/gxfp51a0-warm-keepalive.timer

sudo systemctl daemon-reload
sudo systemctl enable gxfp51a0-boot-prewarm.service 2>/dev/null || true

latest_prewarm_result() {
  journalctl -b -t gxfp51a0-boot-prewarm --no-pager -o cat 2>/dev/null |
    grep 'PREWARM_RESULT=' | tail -n1 || true
}

prewarm_ready_once() {
  local cursor result
  cursor="$(journalctl -b -t gxfp51a0-boot-prewarm -n 0 \
    --show-cursor --no-pager 2>/dev/null | sed -n 's/^-- cursor: //p')"
  sudo systemctl start gxfp51a0-boot-prewarm.service || true
  if [[ -n "$cursor" ]]; then
    result="$(journalctl -b -t gxfp51a0-boot-prewarm \
      --after-cursor "$cursor" --no-pager -o cat 2>/dev/null |
      grep 'PREWARM_RESULT=' | tail -n1 || true)"
  else
    result="$(latest_prewarm_result)"
  fi
  printf '    %s\n' "${result:-PREWARM_RESULT=UNKNOWN}"
  [[ "$result" == *'PREWARM_RESULT=READY'* ]]
}

if (( DRIVER_CHANGED )); then
  echo "==> Reloading native SPI/fprintd path"
  sudo udevadm control --reload
  sudo udevadm trigger --subsystem-match=spi
  sudo udevadm settle --timeout=3 || true

  ready=0
  for attempt in 1 2; do
    echo "==> Semantic fingerprint prewarm attempt $attempt/2"
    sudo systemctl restart fprintd.service
    if prewarm_ready_once; then
      ready=1
      break
    fi
    (( attempt == 1 )) && echo "WARN: prewarm was not READY; retrying one full prestart recovery." >&2
  done
  if (( ! ready )); then
    echo "ERROR: GXFP51A0 did not reach PREWARM_RESULT=READY after bounded recovery." >&2
    exit 7
  fi
else
  echo "==> Fingerprint runtime unchanged; preserving current sensor session"
fi

if (( DESKTOP_INTEGRATION && LEGACY_PLASMA_PATCHES )) && [[ -x /usr/libexec/gxfp51a0-kde-lockscreen-integrate ]]; then
  sudo /usr/libexec/gxfp51a0-kde-lockscreen-integrate --apply || true
  sudo /usr/libexec/gxfp51a0-kde-lockscreen-integrate --check || true
fi

if [[ -x "$DOCTOR" ]]; then
  "$DOCTOR" --pre-enroll
fi

echo "==> Fingerprint enrollment and unlock tests: use the distribution native graphical Settings (KDE/GNOME)."
echo "    Existing templates are preserved; no CLI biometric enrollment or verification is launched."

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
