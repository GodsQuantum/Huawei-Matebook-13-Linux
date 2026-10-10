#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/../.." && pwd)"
bootstrap="$root/install.sh"
i="$root/install-linux.sh"
u="$root/uninstall-linux-source.sh"
a="$root/install-arch.sh"
hook="$root/packaging/arch/libfprint-goodix51a0.install"
doctor="$root/gxfp51a0-doctor.sh"

sh -n "$bootstrap"
bash -n "$i"
bash -n "$u"
bash -n "$a"
bash -n "$hook"
bash -n "$doctor"

# POSIX bootstrap makes the one-command installer work on minimal Alpine,
# where Bash is intentionally not present before dependency installation.
grep -Fq 'command -v apk' "$bootstrap"
grep -Fq 'apk add --no-cache bash' "$bootstrap"
grep -Fq 'exec bash "$ROOT/install-linux.sh"' "$bootstrap"

# Automatic dependency families + generic escape hatch.
for needle in 'distro=generic' 'apt-get' 'dnf install' 'zypper'               'apk add --no-cache' 'distro=alpine' '--no-install-deps'; do
  grep -Fq -- "$needle" "$i"
done

# Arch build-only validation must never perform a partial package upgrade.
grep -Fq 'pacman -Syu --needed --noconfirm' "$i"
! grep -Fq 'pacman -Sy --noconfirm' "$i"

# Cross-distro staged build, dynamic libdir, and ABI gate.
grep -Fq 'MESON_PREFIX="$PREFIX"' "$i"
grep -Fq 'introspect "$BUILD_DIR" --buildoptions' "$i"
grep -Fq 'LIBDIR_REL=' "$i"
grep -Fq 'FPRINTD_ABI_COMPATIBILITY=PASS' "$i"
grep -Fq 'ldd -r "$FPRINTD_BIN"' "$i"
grep -Fq 'LD_BIND_NOW=1' "$i"
grep -Fq 'UDEV_RULES_DIR=' "$root/scripts/build-libfprint-v1.94.100.sh"
grep -Fq -- '-Dudev_hwdb=disabled' "$root/scripts/build-libfprint-v1.94.100.sh"

# Native driver lifecycle: standard resident fprintd, NO extra GXFP services.
grep -Fq 'Environment=LD_LIBRARY_PATH=$LIBDIR' "$i"
! grep -Fq 'BOOT_PREWARM_HELPER_FILE=' "$i"
! grep -Fq 'BOOT_PREWARM_UNIT_FILE=' "$i"
! grep -Fq 'PRESTART_RECOVERY_FILE=' "$i"
! grep -Fq 'systemctl start gxfp51a0-boot-prewarm.service' "$i"
! grep -Fq 'integration/boot-prewarm/' "$root/packaging/arch/PKGBUILD"
! grep -Fq 'ExecStartPre=/usr/libexec/gxfp51a0-prestart-recover' "$root/packaging/arch/fprintd-goodix51a0.conf"
! grep -Fq 'RESUME_HELPER_FILE=' "$i"
! grep -Fq 'RESUME_HOOK_FILE=' "$i"
! grep -Fq 'KEEPALIVE_HELPER_FILE=' "$i"
! grep -Fq 'KEEPALIVE_TIMER_FILE=' "$i"

# Non-systemd isolates the custom library to fprintd, never global ld.so.
grep -Fq '/etc/dbus-1/system-services' "$i"
grep -Fq 'net.reactivated.Fprint.service' "$i"
grep -Fq 'library scope: fprintd only (D-Bus activation wrapper)' "$i"
! grep -Fq 'LDCONF_FILE=' "$i"
! grep -Eq 'run_root[[:space:]]+ldconfig' "$i"

# Parallel-auth invariant: never silently serialize password behind pam_fprintd.
grep -Fq 'parallel_pam_service' "$i"
grep -Fq 'kde-fingerprint' "$i"
grep -Fq 'gdm-fingerprint' "$i"
grep -Fq 'will NOT' "$i"
grep -Fq 'modify a global PAM stack' "$i"
! grep -Fq 'pam-auth-update --enable fprintd' "$i"
! grep -Fq 'authselect enable-feature with-fingerprint' "$i"
! grep -Fq 'pam-config --add --fprintd' "$i"

# Desktop integration is native-first and distro-owned. KDE-specific QML/package
# compatibility is explicit opt-in, never silently applied on future desktops.
grep -Fq -- '--legacy-kde-helper' "$i"
grep -Fq 'LEGACY_KDE_HELPER=0' "$i"
grep -Fq -- '--legacy-plasma-patches' "$a"
grep -Fq 'LEGACY_PLASMA_PATCHES=0' "$a"
grep -Fq 'native fprintd/PAM' "$i"
grep -Fq 'native fprintd/PAM' "$a"

# Production gate: driver doctor + native KDE/GNOME GUI enrollment; never
# force the user through a custom CLI and never delete existing enrollments.
grep -Fq 'gxfp51a0-doctor.sh' "$i"
grep -Fq 'native graphical Settings' "$i"
grep -Fq 'native graphical Settings' "$a"
grep -Fq -- '--pre-enroll' "$i"
! grep -Eq '^[[:space:]]*fprintd-(enroll|verify)[[:space:]]' "$i"
! grep -Eq '^[[:space:]]*fprintd-(enroll|verify)[[:space:]]' "$a"
grep -Fq 'PORTABLE_RELEASE="rel71.31-native-s3-preview1"' "$i"

# Never delete biometric templates or the validated per-unit PMK cache.
! grep -Eq 'rm .*goodix51a0-pmk' "$i"
! grep -Eq 'rm .*template' "$i"
! grep -Eq 'fprintd-delete' "$i"
! grep -Eq 'fprintd-delete' "$a"

# Rollback restores distro files and both init paths.
grep -Fq 'STATE_DIR/backup' "$u"
grep -Fq 'command -v systemctl' "$u"
grep -Fq 'DBUS_SERVICE_FILE=' "$u"
grep -Fq 'FPRINTD_WRAPPER_FILE=' "$u"
grep -Fq 'org.freedesktop.DBus.ReloadConfig' "$u"

# Core Arch package stays desktop-agnostic. KDE/QML compatibility tooling may
# exist in the source tree, but the driver package must neither ship nor invoke it.
! grep -Fq 'integration/kde-lockscreen/' "$root/packaging/arch/PKGBUILD"
! grep -Fq 'gxfp51a0-kde-lockscreen-integrate' "$hook"

# Native Arch package contains only the driver and fprintd runtime glue.
! grep -Fq 'systemctl enable gxfp51a0-boot-prewarm.service' "$hook"
! grep -Fq 'integration/boot-prewarm/' "$root/packaging/arch/PKGBUILD"
! grep -Fq 'integration/prestart-recover/' "$root/packaging/arch/PKGBUILD"
! grep -Fq 'integration/systemd/gxfp51a0-fprintd-suspend.service' "$root/packaging/arch/PKGBUILD"

echo 'test_portable_installer_source_safety: OK (production zero-to-working path)'
