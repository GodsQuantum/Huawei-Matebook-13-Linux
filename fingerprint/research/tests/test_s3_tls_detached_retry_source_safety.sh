#!/usr/bin/env bash
set -euo pipefail
R="$(cd "$(dirname "$0")/../.." && pwd)"
python3 - "$R/driver/goodix51a0/goodix51a0.c" <<'PYTEST'
from pathlib import Path
import sys
s=Path(sys.argv[1]).read_text()
a=s.index('gx_tls_session (FpiDeviceGoodix51A0 *self)')
b=s.index('/* Production-only stale-cache recovery',a)
f=s[a:b]
assert 'NATIVE_S3_TLS_DETACHED_RETRY' in f
a=f.index('if (self->s3_first_tls_pending && self->s3_rel61_tls_fallback)')
b=f.index('gx_gpio_reset (self);',a)
assert f.index('gx_transport_close (self);',a)<b<f.index('gx_transport_open (FP_DEVICE (self)',a)
assert 'normal in-process TLS retry' in f
PYTEST
echo "test_s3_tls_detached_retry_source_safety: PASS"
