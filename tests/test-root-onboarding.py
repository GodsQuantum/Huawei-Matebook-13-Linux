#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
INSTALL = ROOT / "install.sh"
DOCTOR = ROOT / "matebook13-doctor.sh"
DOC = ROOT / "docs" / "ONE_COMMAND_ONBOARDING.md"
GPU = ROOT / "gpu-power" / "huawei-matebook-13-gpu-manager.sh"

def require(cond, msg):
    if not cond:
        raise AssertionError(msg)

install = INSTALL.read_text(encoding="utf-8")
doctor = DOCTOR.read_text(encoding="utf-8")
doc = DOC.read_text(encoding="utf-8")
gpu = GPU.read_text(encoding="utf-8")

require(INSTALL.is_file(), "root install.sh missing")
require(DOCTOR.is_file(), "root doctor missing")
require(DOC.is_file(), "onboarding architecture doc missing")
require(GPU.is_file(), "GPU manager missing")

require('"$ROOT/fingerprint/install.sh"' in install, "fingerprint module delegation missing")
require('"$ROOT/gpu-power/huawei-matebook-13-gpu-manager.sh"' in install, "MX250 manager delegation missing")
require("--no-fingerprint" in install and "--no-gpu" in install, "module skip flags missing")
require("--gpu-driver-preinstalled" in install, "preinstalled NVIDIA driver path missing")
require("charge thresholds" in install.lower(), "silent battery-policy guard missing")
require("huawei_wmi" in install, "mainline Huawei WMI path missing")
require("spi-GXFP51A0:00" in install, "fingerprint hardware gate missing")
require("0x1d13" in install, "MX250 hardware gate missing")
require("/etc/huawei-matebook-gpu-manager.conf" in install, "PCI-absent installed MX250 upgrade detection missing")
require("run ./install.sh as the normal desktop user" in install, "normal-user install contract missing")

require("10de:1d13" in doctor, "MX250 hardware validation missing")
require("R580" in doctor, "Pascal/R580 validation missing")
require("GPU-control" in doctor, "GPU manager validation missing")
require("power-profiles-daemon" in doctor and "tlp" in doctor and "auto-cpufreq" in doctor,
        "power-policy conflict audit missing")
require("/sys/power/mem_sleep" in doctor, "suspend-mode audit missing")
require("13d3:56c6" in doctor, "camera validation missing")
require("Huawei WMI hotkeys" in doctor, "Huawei hotkey validation missing")
require("charge_control_thresholds" in doctor, "battery threshold interface check missing")
require("PRESTART" not in doctor, "doctor should inspect installed state, not perform fingerprint recovery")
require("fwupdmgr" in doctor, "firmware-management visibility missing")

require("native-first" in doc.lower() or "native first" in doc.lower(), "native-first policy missing")
require("71.24" in doc, "current fingerprint checkpoint missing")
require("Pascal" in doc and "RTD3" in doc, "MX250 power-management rationale missing")
require("never flash automatically" in doc.lower(), "firmware safety policy missing")

require('VERSION="3.2.0"' in gpu, "GPU manager v3.2 contract missing")
require("install_intel_default_env" in gpu, "Intel-default user-session policy missing")
require("VK_LOADER_DRIVERS_SELECT=*intel*" in gpu, "Intel Vulkan default missing")
require("unset VK_LOADER_DRIVERS_SELECT" in gpu, "dGPU runner must remove Intel Vulkan filter")
require("unset DRI_PRIME" in gpu, "dGPU runner must remove Intel DRI default")
require("NVreg_DynamicPowerManagement=0x02" not in gpu, "Pascal must not claim unsupported NVIDIA RTD3")

print("ROOT_ONBOARDING_TEST=PASS")
