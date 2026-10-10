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

# Legacy boot helper remains useful as a reproducible historical reference.
# Native-only production must not ship, enable or start any GXFP service.
grep -Fq 'Type=oneshot' "$service"
grep -Fq 'PREWARM_RESULT=READY' "$helper"
! grep -Fq 'integration/boot-prewarm/' "$pkg"
! grep -Fq 'gxfp51a0-boot-prewarm.service' "$pkg"
! grep -Fq 'systemctl enable gxfp51a0-boot-prewarm.service' "$hook"
! grep -Fq 'BOOT_PREWARM_UNIT_FILE=' "$portable"
! grep -Fq 'BOOT_PREWARM_HELPER_FILE=' "$portable"
! grep -Fq 'systemctl start gxfp51a0-boot-prewarm.service' "$portable"
! grep -Fq 'warm-keepalive' "$pkg"
echo 'test_boot_prewarm_source_safety: OK (legacy reference only; no extra service shipped)'
