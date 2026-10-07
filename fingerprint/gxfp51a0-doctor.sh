#!/usr/bin/env bash
set -Eeuo pipefail

PRE_ENROLL=0
[[ "${1:-}" == "--pre-enroll" ]] && PRE_ENROLL=1

failures=0
warnings=0

ok()   { printf 'PASS  %s\n' "$*"; }
warn() { printf 'WARN  %s\n' "$*"; warnings=$((warnings + 1)); }
fail() { printf 'FAIL  %s\n' "$*"; failures=$((failures + 1)); }

have() { command -v "$1" >/dev/null 2>&1; }

echo "GXFP51A0 doctor"
echo "kernel: $(uname -r)"
echo

spi_sys=""
for m in /sys/bus/spi/devices/*/modalias; do
  [[ -r "$m" ]] || continue
  if grep -q '^acpi:GXFP51A0:' "$m"; then
    spi_sys="$(dirname "$m")"
    break
  fi
done

if [[ -n "$spi_sys" ]]; then
  ok "ACPI/SPI GXFP51A0 detected at $(basename "$spi_sys")"
else
  fail "GXFP51A0 ACPI/SPI device not detected"
fi

if [[ -n "$spi_sys" && -L "$spi_sys/driver" ]]; then
  driver="$(basename "$(readlink -f "$spi_sys/driver")")"
  [[ "$driver" == "spidev" ]] && ok "SPI device bound to spidev" || fail "SPI device bound to $driver, expected spidev"
else
  fail "SPI driver binding unavailable"
fi

spidev_node=""
if [[ -n "$spi_sys" ]]; then
  for n in "$spi_sys"/spidev/spidev*; do
    [[ -e "$n" ]] || continue
    candidate="/dev/$(basename "$n")"
    [[ -e "$candidate" ]] && { spidev_node="$candidate"; break; }
  done
fi
if [[ -n "$spidev_node" ]]; then
  ok "SPI character device present: $spidev_node"
else
  fail "spidev character device not present"
fi

if have systemctl && systemctl cat fprintd.service >/dev/null 2>&1; then
  systemctl is-active --quiet fprintd.service && ok "fprintd service active" || fail "fprintd service inactive"
fi

lib=""
for p in   /usr/local/lib/libfprint-2.so.2.0.0   /usr/local/lib64/libfprint-2.so.2.0.0   /usr/local/lib/*/libfprint-2.so.2.0.0   /usr/lib/libfprint-2.so.2.0.0   /usr/lib64/libfprint-2.so.2.0.0   /usr/lib/*/libfprint-2.so.2.0.0; do
  [[ -f "$p" ]] || continue
  if grep -aFq 'GXFP51A0' "$p" 2>/dev/null; then
    lib="$p"
    break
  fi
done
if [[ -n "$lib" ]]; then
  ok "GXFP51A0 libfprint driver present: $lib"
else
  fail "GXFP51A0-enabled libfprint not found in standard locations"
fi

if have busctl && busctl --system status net.reactivated.Fprint >/dev/null 2>&1; then
  if busctl --system call net.reactivated.Fprint /net/reactivated/Fprint/Manager       net.reactivated.Fprint.Manager GetDefaultDevice >/dev/null 2>&1; then
    ok "fprintd exposes a default fingerprint device"
  else
    fail "fprintd has no default fingerprint device"
  fi
fi

pam_module=""
for p in /usr/lib/security/pam_fprintd.so /usr/lib64/security/pam_fprintd.so          /lib/security/pam_fprintd.so /lib/*/security/pam_fprintd.so          /usr/lib/*/security/pam_fprintd.so; do
  [[ -f "$p" ]] && { pam_module="$p"; break; }
done
[[ -n "$pam_module" ]] && ok "pam_fprintd installed: $pam_module" || fail "pam_fprintd module not found"

if [[ -d /usr/share/plasma || -d /usr/share/plasma/shells ]]; then
  kde_pam=""
  for p in /etc/pam.d/kde-fingerprint /usr/lib/pam.d/kde-fingerprint; do
    [[ -f "$p" ]] && { kde_pam="$p"; break; }
  done
  [[ -n "$kde_pam" ]] && ok "KDE fingerprint PAM stack present: $kde_pam" || warn "KDE detected but kde-fingerprint PAM stack not found"

  qml=/usr/share/plasma/shells/org.kde.plasma.desktop/contents/lockscreen/LockScreenUi.qml
  if [[ -f "$qml" ]]; then
    grep -q 'GXFP51A0 .*fingerprint integration' "$qml"       && ok "KDE lockscreen immediate fingerprint integration present"       || warn "KDE lockscreen has no GXFP51A0 immediate-start integration"
  fi
fi

if have systemctl; then
  ext=0
  systemctl is-enabled --quiet gxfp51a0-fprintd-suspend.service 2>/dev/null && ext=1 || true
  systemctl is-enabled --quiet gxfp51a0-warm-keepalive.timer 2>/dev/null && ext=1 || true
  systemctl is-enabled --quiet gxfp51a0-warm-keepalive.service 2>/dev/null && ext=1 || true
  systemctl is-enabled --quiet gxfp51a0-resume-prewarm.service 2>/dev/null && ext=1 || true
  (( ext == 0 ))     && ok "no obsolete periodic/external sleep lifecycle service enabled"     || fail "obsolete GXFP51A0 periodic/external lifecycle service is enabled"

  if systemctl is-enabled --quiet gxfp51a0-boot-prewarm.service 2>/dev/null; then
    unit="$(systemctl cat gxfp51a0-boot-prewarm.service 2>/dev/null || true)"
    if grep -Fq 'Type=oneshot' <<<"$unit" && ! grep -Fq '.timer' <<<"$unit"; then
      ok "one-shot GXFP51A0 boot prime enabled"
    else
      fail "GXFP51A0 boot prime is not a bounded one-shot service"
    fi

    last_prewarm="$(journalctl -b -t gxfp51a0-boot-prewarm \
      --no-pager -o cat 2>/dev/null | grep 'PREWARM_RESULT=' | tail -n1 || true)"
    if [[ "$last_prewarm" == *'PREWARM_RESULT=READY'* ]]; then
      ok "latest semantic fingerprint prewarm is READY"
    elif [[ "$last_prewarm" == *'PREWARM_RESULT=FAILED'* ]]; then
      fail "latest semantic fingerprint prewarm FAILED"
    else
      warn "no semantic PREWARM_RESULT found in the current boot journal"
    fi
  else
    warn "one-shot GXFP51A0 boot prime is not enabled"
  fi
fi

if [[ "$(uname -r)" == "7.2.8-2-cachyos" ]]; then
  warn "CachyOS 7.2.8-2 Clang/ThinLTO build is known to degrade this reference unit; prefer a newer fixed build or CachyOS GCC variant"
fi

if (( ! PRE_ENROLL )); then
  if have fprintd-list; then
    if fprintd-list "$USER" 2>/dev/null | grep -q -- '-finger'; then
      ok "at least one fingerprint enrolled for $USER"
    else
      fail "no fingerprint enrolled for $USER"
    fi
  else
    fail "fprintd-list command not found"
  fi
fi

echo
echo "doctor: failures=$failures warnings=$warnings"
(( failures == 0 ))
