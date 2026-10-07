#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/../.." && pwd)"
pkg="$root/packaging/arch/PKGBUILD"
hook="$root/packaging/arch/libfprint-goodix51a0.install"
portable="$root/install-linux.sh"

helper="$root/integration/boot-prewarm/gxfp51a0-boot-prewarm"
service="$root/integration/boot-prewarm/gxfp51a0-boot-prewarm.service"

test -x "$helper"
test -f "$service"

# Production ships exactly one bounded pre-display-manager Claim.
grep -Fq 'integration/boot-prewarm/gxfp51a0-boot-prewarm' "$pkg"
grep -Fq 'gxfp51a0-boot-prewarm.service' "$pkg"
grep -Fq 'graphical.target.wants/gxfp51a0-boot-prewarm.service' "$pkg"
grep -Fq 'systemctl enable gxfp51a0-boot-prewarm.service' "$hook"
grep -Fq 'integration/boot-prewarm/gxfp51a0-boot-prewarm' "$portable"
grep -Fq 'BOOT_PREWARM_UNIT_FILE=' "$portable"
grep -Fq 'systemctl start gxfp51a0-boot-prewarm.service' "$portable"

# It remains one-shot: no timer, periodic Claim loop, or external sleep hook.
grep -Fq 'Type=oneshot' "$service"
grep -Fq 'busctl --system --timeout=45s call' "$helper"
grep -Fq 'PREWARM_RESULT=READY' "$helper"
grep -Fq 'PREWARM_RESULT=FAILED' "$helper"
grep -Fq 'timeout --signal=TERM --kill-after=2s 48s' "$helper"
! grep -Fq '.timer' "$service"
! grep -Fq 'warm-keepalive' "$pkg"
! grep -Fq 'timers.target.wants/gxfp51a0-warm-keepalive.timer' "$pkg"
grep -Fq 'disable --now' "$hook"
grep -Fq 'gxfp51a0-fprintd-suspend.service' "$hook"

echo 'test_boot_prewarm_source_safety: OK (one-shot prime, no periodic keepalive)'
