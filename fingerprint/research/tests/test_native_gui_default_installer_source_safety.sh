#!/usr/bin/env bash
set -euo pipefail

root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../../.." && pwd)"
arch="$root/fingerprint/install-arch.sh"
portable="$root/fingerprint/install-linux.sh"
controller="$root/install.sh"
pkgbuild="$root/fingerprint/packaging/arch/PKGBUILD"

# Source-only: never execute install scripts or hardware I/O.
grep -Eq '^pkgrel=[1-9][0-9]*(\.[0-9]+)*$' "$pkgbuild"
# Do not freeze the safety test to obsolete historical release IDs.
grep -Eq '^PORTABLE_RELEASE="rel[0-9]+\.[0-9]+-[a-zA-Z0-9.\-]+"$' "$portable"
grep -Fq 'fingerprint_expected_version()' "$controller"

for installer in "$arch" "$portable"; do
  bash -n "$installer"
  grep -Fq 'native graphical Settings' "$installer"
  if grep -Eq '^[[:space:]]*fprintd-(enroll|verify)[[:space:]]' "$installer"; then
    echo "ERROR: installer launches CLI biometric enrollment/verification" >&2
    exit 1
  fi
done

# Optional guided GUI is packaged, but never automatically launched.
grep -Fq 'gxfp51a0-enroll' "$pkgbuild"
grep -Fq 'gxfp51a0-enroll' "$portable"
! grep -Eq '^[[:space:]]*gxfp51a0-enroll[[:space:]]*$' "$portable"
echo 'NATIVE_GUI_DEFAULT_INSTALLER_SOURCE_TEST=PASS'
