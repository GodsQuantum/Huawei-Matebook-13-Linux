#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/../.." && pwd)"
driver="$root/driver/goodix51a0/goodix51a0.c"
defs="$root/driver/goodix51a0/goodix51a0.h"
target="$root/driver/goodix51a0/gx51_target.c"
pkg="$root/packaging/arch/PKGBUILD"
hook="$root/packaging/arch/libfprint-goodix51a0.install"
portable="$root/install-linux.sh"
kscreen="$root/integration/kscreenlocker-upstream-s3/0001-upstream-992f3fa8-keep-pam-across-suspend.patch"

# Validated rel71 core: ordinary Close never sends Windows 0x60 sleep.
! grep -Fq '#define GOODIX_CMD_SLEEP' "$defs"
grep -Fq 'gxfp_build_sleep' "$target"
# The sleep opcode is allowed only in native suspend, not in warm Close.
python3 - "$driver" <<'PY2'
from pathlib import Path
import sys
s=Path(sys.argv[1]).read_text()
start=s.index('gx_dev_close (FpDevice *dev)')
end=s.index('gx_dev_suspend (FpDevice *dev)',start)
assert 'gxfp_build_sleep' not in s[start:end]
start=s.index('gx_dev_suspend (FpDevice *dev)')
end=s.index('gx_dev_resume (FpDevice *dev)',start)
assert 'gxfp_build_sleep' in s[start:end]
PY2
! grep -Fq 'gx_sensor_sleep' "$driver"

# Native libfprint PM boundary invalidates warm state and completes suspend/resume.
grep -Fq 'self->force_cold_reset = TRUE' "$driver"
grep -Fq 'fpi_device_suspend_complete (' "$driver"
grep -Fq 'FP_DEVICE_ERROR_NOT_SUPPORTED' "$driver"
grep -Fq 'fpi_device_resume_complete (dev, NULL)' "$driver"

# KScreenLocker owns PAM resume recovery: password survives, only the
# noninteractive fingerprint worker is restarted.
grep -Fq 'Resume: rearming fingerprint PAM' "$kscreen"
grep -Fq 'resumeAuthenticating' "$kscreen"
grep -Fq 'restartAuthentication' "$kscreen"

# Historical external sleep helper may remain as research provenance, but no
# production package/installer may ship or enable it.
! grep -Fq 'integration/systemd/gxfp51a0-fprintd-suspend.service' "$pkg"
grep -Fq 'disable --now gxfp51a0-fprintd-suspend.service' "$hook"
! grep -Fq 'systemctl enable gxfp51a0-fprintd-suspend.service' "$portable"
grep -Fq 'gxfp51a0-fprintd-suspend.service' "$portable"

echo 'test_sleep_lifecycle_source_safety: OK (native PM + KScreen PAM resume, no external hook)'
