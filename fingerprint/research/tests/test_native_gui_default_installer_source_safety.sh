#!/usr/bin/env bash
set -euo pipefail

root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../../.." && pwd)"
arch="$root/fingerprint/install-arch.sh"
portable="$root/fingerprint/install-linux.sh"
controller="$root/install.sh"
pkgbuild="$root/fingerprint/packaging/arch/PKGBUILD"

# Source-only: never execute install scripts or hardware I/O.
grep -Fq 'pkgrel=71.31' "$pkgbuild"
grep -Fq 'PORTABLE_RELEASE="rel71.31-native-s3-preview1"' "$portable"
grep -Fq 'rel71.30-portable1' "$controller"

for installer in "$arch" "$portable"; do
  bash -n "$installer"
  grep -Fq 'native graphical Settings' "$installer"
  if grep -Eq '^[[:space:]]*fprintd-(enroll|verify)[[:space:]]' "$installer"; then
    echo "ERROR: installer launches CLI biometric enrollment/verification" >&2
    exit 1
  fi
done

echo 'NATIVE_GUI_DEFAULT_INSTALLER_SOURCE_TEST=PASS'
