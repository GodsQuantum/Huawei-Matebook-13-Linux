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
# Prevent stale or contradictory multilingual enrollment instructions.
python3 - "$root" <<'PYDOC'
from pathlib import Path
import sys
fp = Path(sys.argv[1]); repo = fp.parent
for p in [repo / "README.md", repo / "README.FR.md", repo / "README.ZH-CN.md",
          fp / "README.md", fp / "README.FR.md", fp / "README.ZH-CN.md"]:
    text = p.read_text(encoding="utf-8")
    assert "gxfp51a0-enroll" in text, p
    assert "fprintd" in text, p
    assert "KDE" in text, p
    for outdated in ("never a custom enrollment tool",
                     "jamais un outil d’enrôlement personnalisé",
                     "不使用自定义录入工具"):
        assert outdated not in text, (p, outdated)
assert "gxfp51a0-enroll" in (fp / "DRIVER_ONLY.md").read_text(encoding="utf-8")
PYDOC
echo 'GUIDED_ENROLL_CROSS_DISTRO_SOURCE_TEST=PASS'
