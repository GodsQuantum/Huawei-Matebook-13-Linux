#!/usr/bin/env python3
# Native rel71.51 boot/lock/S3 source gate. Source checks are not real BOOT certification.
from pathlib import Path
import re
ROOT=Path(__file__).resolve().parents[1]
FP=ROOT/'fingerprint'
DRIVER=(FP/'driver/goodix51a0/goodix51a0.c').read_text()
PATCH=(FP/'driver/goodix51a0/libfprint-v1.94.100.patch').read_text()
ARCH=(FP/'install-arch.sh').read_text()
LINUX=(FP/'install-linux.sh').read_text()
PKG=(FP/'packaging/arch/PKGBUILD').read_text()
LIFECYCLE=(FP/'packaging/arch/libfprint-goodix51a0.install').read_text()
FPRINTD=(FP/'packaging/arch/fprintd-goodix51a0.conf').read_text()
KDE=FP/'integration/plasma-login-manager-6.8.1-qa'
PLM=(KDE/'0001-plasma-login-6.8.1-native-dual-fingerprint-password.patch').read_text()
PKGPLM=(KDE/'PKGBUILD').read_text()
assert "pkgrel=71.63" in PKG
assert "pkgver=1.94.100.goodix51a0" in PKG
assert "install=libfprint-goodix51a0.install" in PKG
assert "provides=('libfprint=1.94.100'" in PKG
assert "libfprint 1.94.100" in PKG
assert "v1.94.100" in (FP/'scripts/build-libfprint-v1.94.100.sh').read_text()
# Native lifecycle owns warm TLS reuse, without an external system-sleep daemon.
assert "dev_class->suspend = gx_dev_suspend;" in DRIVER
assert "dev_class->resume = gx_dev_resume;" in DRIVER
assert "gx_warm_crossed_sleep" in DRIVER
assert "CLOCK_BOOTTIME" in DRIVER and "CLOCK_MONOTONIC" in DRIVER
assert "NATIVE_PROBE_PREWARM" in DRIVER
assert "gx_cold_prepare (self) && gx_warm_available (self)" in DRIVER
assert "fpi_device_probe_complete (dev, NULL, NULL, NULL);" in DRIVER
assert "gx_transport_close (self)" in DRIVER
assert "force_cold_reset" in DRIVER
assert "gx_cancelled (w->dev, w->ssm)" in DRIVER
assert "gx_cancelled (dev, ssm)" in DRIVER
assert "GXFP51A0 S3_CANCEL" in DRIVER
assert "fpi_device_suspend_complete" in DRIVER
assert "fpi_device_resume_complete (dev, NULL)" in DRIVER
assert "G_IO_ERROR_CANCELLED" in DRIVER
assert "gx_capture_auth_same_press" in DRIVER
assert re.search(r"^#define\s+GX_MATCH_THRESHOLD\s+7\s*$",DRIVER,re.M)
assert re.search(r"^#define\s+GX_VERIFY_MAX_ATTEMPTS\s+3\b",DRIVER,re.M)
assert "GPIO112" not in DRIVER
assert "GPP_D16" not in DRIVER
assert "FPI_DEVICE_UDEV_SUBTYPE_SPIDEV" in DRIVER
assert '.spi_acpi_id = "GXFP51A0"' in DRIVER
assert "GOODIX51A0_UDEV_MODALIAS_GLOB" in (FP/'scripts/build-libfprint-v1.94.100.sh').read_text()
assert "Before=display-manager.service" in FPRINTD
assert "TimeoutStartSec=90s" in FPRINTD
assert "ExecStart=/usr/lib/fprintd --no-timeout" in FPRINTD
assert "LimitCORE=0" in FPRINTD
assert "gxfp51a0-boot-prewarm.service" not in PKG
assert "systemd/system-sleep/gxfp51a0-resume-prewarm" not in PKG
assert "gxfp51a0-warm-keepalive.timer" not in PKG
assert "systemctl enable gxfp51a0-boot-prewarm.service" not in LIFECYCLE
assert "systemctl try-restart fprintd.service" in LIFECYCLE
assert "systemctl disable --now gxfp51a0-fprintd-suspend.service" in LIFECYCLE
assert "udevadm control --reload" in LIFECYCLE
assert "pam_fprintd.so" not in FPRINTD
# Optional KDE PLM package never touches the user's stock PAM password stack.
assert "pkgver=6.8.1" in PKGPLM
assert "plasmalogin-fingerprint" in PLM
assert "pam_fprintd.so max-tries=1 timeout=30" in PLM
assert "pam_deny.so" in PLM
assert "pam_permit.so" not in "\n".join(line[1:] for line in PLM.splitlines() if line.startswith("+") and not line.startswith("+++"))
assert "m_fingerprintAuth" in PLM and "m_loginCommitted" in PLM
assert "onVisibleChanged:" in PLM
assert "fingerprintRetryTimer.stop()" in PLM
# Both portable and Arch installers retain a real rollback path; no broad
# PAM password changes or emergency default permit.
assert "fprintd" in ARCH and "fprintd" in LINUX
assert (FP/'uninstall-linux-source.sh').exists()
print("GOODIX51A0_NATIVE_BOOT_S3_SECURITY_SOURCE_TEST=PASS")
