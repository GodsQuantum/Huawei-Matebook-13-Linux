#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/../.." && pwd)"
driver="$root/driver/goodix51a0/goodix51a0.c"
block="$(sed -n '/^fpi_device_goodix51a0_finalize (/,/^}/p' "$driver")"

grep -Fq 'FINALIZE_TRACE abandoning host warm TLS context' <<<"$block"
grep -Fq 'gx_warm_abandon (self)' <<<"$block"
grep -Fq 'gx_transport_close (self)' <<<"$block"

if grep -Fq 'gx_transport_open' <<<"$block"; then
  echo 'finalize must not reopen SPI transport'
  exit 1
fi
if grep -Fq 'gx_warm_discard (self)' <<<"$block"; then
  echo 'finalize must not send TLS close_notify through gx_warm_discard'
  exit 1
fi

echo 'test_finalize_warm_teardown_source_safety: OK'
