#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
INSTALL = ROOT / "install.sh"

text = INSTALL.read_text(encoding="utf-8")

required = {
    "--menu option": "--menu" in text,
    "whiptail bootstrap": "ensure_whiptail()" in text and "whiptail" in text,
    "audit before menu": "prepare_menu_files" in text and "audit_summary" in text,
    "recommended action": '"RECOMMENDED"' in text,
    "fingerprint action": '"FINGERPRINT"' in text,
    "gpu action": '"GPU"' in text,
    "all action": '"ALL"' in text,
    "detailed audit": '"AUDIT"' in text and "show_audit_box" in text,
    "shortcuts help": '"SHORTCUTS"' in text and "show_shortcuts_box" in text,
    "one-off GPU help": "GPU-control run -- handy" in text,
    "persistent GPU help": "GPU-control add Handy.desktop" in text,
    "GPU detection policy explained": "does NOT guess" in text and "PrefersNonDefaultGPU=true" in text,
    "fingerprint current detection": "fingerprint_status_code" in text and "fingerprint_expected_version" in text,
    "gpu current detection": "gpu_status_code" in text and "gpu_expected_version" in text,
    "current components skipped by recommended": 'case "$(fingerprint_status_code)" in missing|outdated)' in text,
    "CLI launcher": "install_user_launcher" in text and "$HOME/.local/bin/HUAWEI" in text,
    "launcher targets canonical installer": 'ln -sfn "$ROOT/install.sh" "$launcher"' in text,
    "symlink-aware repository root": 'readlink -f "${BASH_SOURCE[0]}"' in text,
    "interactive no-arg behavior": "ARGS_PROVIDED == 0" in text and "-t 0" in text and "-t 1" in text,
    "forced menu rejects non-TTY": "--menu requires an interactive terminal" in text,
    "native whiptail output fd": "--output-fd 3" in text and '3>"$CHOICE_FILE"' in text and "3>&1 1>&2 2>&3" not in text,
    "choice file shares TUI temp dir": 'CHOICE_FILE="$MENU_TMP_DIR/choice"' in text,
    "responsive whiptail sizing": '--textbox "$AUDIT_FILE" 0 0' in text and '--menu ' in text and ' 0 0 0 ' in text,
    "recommended is default": '--default-item "RECOMMENDED"' in text,
    "keyboard help": "Esc/Cancel" in text and "Enter" in text,
    "single TUI temp directory": 'MENU_TMP_DIR="$(mktemp -d)"' in text and "cleanup_menu_files" in text,
    "shortcuts file shares TUI temp dir": 'SHORTCUTS_FILE="$MENU_TMP_DIR/shortcuts.txt"' in text,
    "EXIT trap cleans TUI temp": "trap 'cleanup_menu_files' EXIT" in text,
    "component-only skips platform baseline": "DO_PLATFORM=0" in text and "Platform baseline skipped (component-only action)" in text,
    "non-interactive compatibility": "Non-interactive execution keeps the historical full-install" in text,
    "Arch whiptail package": "libnewt" in text,
    "Debian whiptail package": "install -y whiptail" in text,
    "Fedora/OpenSUSE/Alpine newt package": text.count("newt") >= 4,
}

failed = [name for name, ok in required.items() if not ok]
if failed:
    for name in failed:
        print(f"FAIL {name}")
    raise SystemExit(1)

print("INSTALL_TUI_TEST=PASS")
