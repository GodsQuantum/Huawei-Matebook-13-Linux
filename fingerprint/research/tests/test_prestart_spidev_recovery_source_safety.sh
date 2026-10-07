#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/../.." && pwd)"
helper="$root/integration/prestart-recover/gxfp51a0-prestart-recover.c"
transport="$root/driver/goodix51a0/gx51_transport.c"
dropin="$root/packaging/arch/fprintd-goodix51a0.conf"
pkg="$root/packaging/arch/PKGBUILD"
installer="$root/install-linux.sh"
build="/tmp/gxfp51a0-prestart-recover-test.$$"
trap 'rm -f "$build"' EXIT

cc -std=c11 -O2 -Wall -Wextra -Werror -pedantic \
  -I"$root/driver/goodix51a0" \
  "$helper" "$transport" -o "$build"

python3 - "$helper" "$dropin" "$pkg" "$installer" <<'PY'
from pathlib import Path
import sys
helper, dropin, pkg, installer = [Path(x).read_text() for x in sys.argv[1:]]

assert 'spi-GXFP51A0:00' in helper
assert '/sys/bus/spi/drivers/spidev' in helper
assert '/sys/bus/platform/drivers/pxa2xx' not in helper
assert 'pxa2xx-spi.4' not in helper
assert 'GPIO112' not in helper
assert 'GX51_RESET_LINE 112' not in helper

main = helper[helper.index('main (void)'):]
u = main.index('write_sysfs (GX_UNBIND')
r = main.index('gx51_reset_gpio264 ()')
b = main.index('write_sysfs (GX_BIND')
assert u < r < b
assert 'sleep_ms (1000)' in main
assert 'PRESTART_RECOVERY=READY unbind+GPIO264+rebind' in main
assert 'PRESTART_RECOVERY=FAILED' in main
assert 'systemctl' not in helper
assert 'modprobe' not in helper
assert 'no firmware operation' in helper

p = dropin.index('ExecStartPre=/usr/libexec/gxfp51a0-prestart-recover')
s = dropin.index('ExecStartPre=-/usr/bin/udevadm settle --timeout=3')
assert p < s
assert 'ReadWritePaths=-/sys/bus/spi/drivers/spidev' in dropin
assert 'ReadWritePaths=-/sys/bus/spi/devices/spi-GXFP51A0:00' in dropin

assert 'gxfp51a0-prestart-recover.c' in pkg
assert 'usr/libexec/gxfp51a0-prestart-recover' in pkg
assert 'prestart-recover.service' not in pkg

assert 'PRESTART_RECOVERY_FILE="$LIBEXEC_DIR/gxfp51a0-prestart-recover"' in installer
assert 'ExecStartPre=$PRESTART_RECOVERY_FILE' in installer
assert '"$PRESTART_RECOVERY_FILE"' in installer
assert 'ReadWritePaths=-/sys/bus/spi/drivers/spidev' in installer
PY

echo 'test_prestart_spidev_recovery_source_safety: OK'
