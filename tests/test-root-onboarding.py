#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
INSTALL = ROOT / 'install.sh'
DOCTOR = ROOT / 'matebook13-doctor.sh'
DOC = ROOT / 'docs' / 'ONE_COMMAND_ONBOARDING.md'

def require(cond, msg):
    if not cond:
        raise AssertionError(msg)

install = INSTALL.read_text(encoding='utf-8')
doctor = DOCTOR.read_text(encoding='utf-8')
doc = DOC.read_text(encoding='utf-8')

require(INSTALL.is_file(), 'root install.sh missing')
require(DOCTOR.is_file(), 'root doctor missing')
require(DOC.is_file(), 'onboarding architecture doc missing')
require('./fingerprint/install.sh' not in install, 'root installer must invoke fingerprint via ROOT path, not cwd-relative literal')
require('"$ROOT/fingerprint/install.sh"' in install, 'fingerprint module delegation missing')
require('gpu-power' in install and 'not installed' in install, 'deprecated GPU manager exclusion missing')
require('charge threshold' in install.lower(), 'silent battery-policy guard missing')
require('huawei_wmi' in install, 'mainline Huawei WMI path missing')
require('spi-GXFP51A0:00' in install, 'fingerprint hardware gate missing')
require('10de:1d13' in doctor, 'MX250 hardware validation missing')
require('13d3:56c6' in doctor, 'camera validation missing')
require('Huawei WMI hotkeys' in doctor, 'Huawei hotkey validation missing')
require('charge_control_thresholds' in doctor, 'battery threshold interface check missing')
require('PRESTART' not in doctor, 'doctor should inspect installed state, not perform recovery')
require('native first' in doc.lower(), 'native-first policy missing')
require('never install it from the default onboarding path' in doc, 'experimental/history exclusion missing')
require('71.18' in doc, 'validated fingerprint checkpoint missing')
require('never flash automatically' in doc.lower(), 'firmware safety policy missing')

print('ROOT_ONBOARDING_TEST=PASS')
