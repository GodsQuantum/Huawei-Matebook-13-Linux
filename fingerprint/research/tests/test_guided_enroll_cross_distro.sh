#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/../.." && pwd)"
python3 "$root/tools/gxfp51a0-enroll" --self-test
python3 - "$root/tools/gxfp51a0-enroll" <<'PYTEST'
import ast, sys
s=open(sys.argv[1], encoding='utf-8').read()
ast.parse(s)
assert 'DeleteEnrolledFinger' not in s
assert 'DeleteEnrolledFingers' not in s
assert 'EnrollStatus' in s and 'EnrollStart' in s
assert 'Release' in s and 'Claim' in s
assert 'enroll-completed' in s and 'enroll-stage-passed' in s
assert 'num-enroll-stages' in s
PYTEST
grep -Fq 'gxfp51a0-enroll' "$root/packaging/arch/PKGBUILD"
grep -Fq 'install_guided_helper' "$root/install-linux.sh"
grep -Fq 'gxfp51a0-enroll' "$root/tools/gxfp51a0-enroll.desktop"
bash -n "$root/install-linux.sh"
bash -n "$root/packaging/arch/PKGBUILD"
echo 'GUIDED_ENROLL_CROSS_DISTRO_SOURCE_TEST=PASS'
