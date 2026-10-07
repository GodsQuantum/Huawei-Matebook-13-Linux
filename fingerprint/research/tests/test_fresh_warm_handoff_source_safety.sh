#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/../.." && pwd)"
src="$root/driver/goodix51a0/goodix51a0.c"

# No blind rel34 one-shot token is allowed back in production. Reuse is gated
# by an actual FDT liveness transaction on every new Claim.
! grep -Fq 'warm_handoff_ready' "$src"
! grep -Fq 'GX_WARM_HANDOFF_TTL_US' "$src"
! grep -Fq 'gx_warm_consume_fresh_handoff' "$src"

open_block="$(sed -n '/gx_dev_open (FpDevice \*dev)/,/^}/p' "$src")"
grep -Fq 'warm_candidate = gx_warm_available (self);' <<<"$open_block"
grep -Fq 'if (warm_candidate && !self->capture_recovery_pending)' <<<"$open_block"
grep -Fq 'if (gx_warm_fast_ready (self))' <<<"$open_block"
grep -Fq 'if (gx_warm_validate (self))' <<<"$open_block"
grep -Fq 'RESET_TRACE cold boundary before spidev open' <<<"$open_block"

fast_line="$(grep -n 'if (gx_warm_fast_ready (self))' "$src" | head -1 | cut -d: -f1)"
full_line="$(grep -n 'if (gx_warm_validate (self))' "$src" | tail -1 | cut -d: -f1)"
[[ "$fast_line" -lt "$full_line" ]]

close_block="$(sed -n '/gx_dev_close (FpDevice \*dev)/,/^}/p' "$src")"
grep -Fq '!self->force_cold_reset && gx_warm_available (self)' <<<"$close_block"
grep -Fq 'CLOSE_TRACE retained complete warm context' <<<"$close_block"
! grep -Fq 'self->production_ready && self->warm_valid' <<<"$close_block"

echo 'test_fresh_warm_handoff_source_safety: OK (FDT-gated fast reuse, no blind token)'
