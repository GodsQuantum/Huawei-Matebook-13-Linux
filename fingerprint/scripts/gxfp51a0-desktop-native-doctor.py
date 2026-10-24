#!/usr/bin/env python3
"""Read-only cross-distribution fingerprint desktop integration audit.

Discovery is from standard fprintd/udev/PAM/desktop files; a driver cannot make
apps support biometric APIs when they only implement password prompts.
"""
import argparse
import configparser
import json
import os
import shutil
from pathlib import Path


SERVICES = {
    "kde_lock": ("kde-fingerprint",),
    "plasma_login": ("plasmalogin-fingerprint",),
    "gnome_login": ("gdm-fingerprint",),
    "polkit": ("polkit-1",),
    "sudo": ("sudo",),
}


def audit(root: Path, home: Path):
    def path(p):
        return root / p.lstrip("/")

    # /etc wins over /usr/lib under the normal Linux PAM search order.
    def pam(service):
        for directory in ("/etc/pam.d", "/usr/lib/pam.d",
                          "/usr/lib64/pam.d", "/lib/pam.d"):
            p = path(f"{directory}/{service}")
            if p.is_file():
                return p
        return None

    def direct_fingerprint(service):
        p = pam(service)
        if not p:
            return {"pam_exists": False, "native_pam_fprintd": False}
        lines = [l.strip() for l in p.read_text(errors="replace").splitlines()]
        active = [l for l in lines if l and not l.startswith("#")]
        # A single PAM service may include a separate system-auth stack.
        # We deliberately do not infer direct biometrics from that include:
        # doing so could claim nonexistent login support.
        return {
            "pam_exists": True,
            "native_pam_fprintd": any("pam_fprintd.so" in l.split() for l in active),
            "password_stack_kept": any(
                l.startswith(("auth", "account")) and
                ("system-auth" in l or "pam_unix.so" in l) for l in active
            ),
        }

    fingerprints = {}
    for action, names in SERVICES.items():
        options = [direct_fingerprint(name) for name in names]
        fingerprints[action] = {
            "native_service": bool(options[0]["pam_exists"]),
            "fingerprint_directly_enabled": bool(options[0]["native_pam_fprintd"]),
            "service": names[0],
            "password_stack_kept": bool(options[0].get("password_stack_kept")),
        }

    config = configparser.ConfigParser(interpolation=None)
    config.optionxform = str
    lockrc = home / ".config/kscreenlockerrc"
    if lockrc.is_file():
        config.read(lockrc)
    lock_setting = config.get("Authenticators", "Fingerprint", fallback=None)
    if lock_setting is None:
        gui_selection = "distro_or_KDE_default"
    else:
        gui_selection = "enabled" if lock_setting.lower() in ("1", "true", "yes") else "disabled"

    pam_module_candidates = (
        list(path("/usr/lib").glob("*/security/pam_fprintd.so"))
        + list(path("/usr/lib64").glob("*/security/pam_fprintd.so"))
        + [path("/usr/lib/security/pam_fprintd.so"),
           path("/usr/lib64/security/pam_fprintd.so"),
           path("/lib/security/pam_fprintd.so")]
    )
    installed_pam_module = any(x.is_file() for x in pam_module_candidates)
    result = {
        "schema": "gxfp51a0-desktop-native-v1",
        "read_only": True,
        "fprintd_client_present": bool(shutil.which("fprintd-list")) if root == Path("/") else None,
        "pam_fprintd_module_present": installed_pam_module,
        "pam": fingerprints,
        "kde_fingerprint_selection": gui_selection,
        "gui_enrollment": "KDE Settings > Users > Fingerprint; GNOME Settings > Users",
        "caveats": [
            "fprintd device availability requires runtime D-Bus enumeration and a real enrolled print",
            "GUI applications must natively use PAM, fprintd, polkit, or a supported authentication API",
            "a password-encrypted KWallet cannot be unlocked using an ordinary fingerprint alone",
            "WebAuthn passkeys require a compatible FIDO2/passkey authenticator, not an arbitrary fprintd driver",
            "password fallback and distro-managed PAM files must remain intact",
            "Plasma login manager may need its separately supported fingerprint PAM backend",
        ],
    }
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", default="/", help="root tree for offline testing (read-only)")
    parser.add_argument("--home", default=os.path.expanduser("~"))
    parser.add_argument("--json", action="store_true")
    args = parser.parse_args()
    result = audit(Path(args.root), Path(args.home))
    if args.json:
        print(json.dumps(result, indent=2, sort_keys=True))
    else:
        print("Native fingerprint desktop audit (read-only)")
        print("fprintd client:", result["fprintd_client_present"])
        print("pam_fprintd module:", result["pam_fprintd_module_present"])
        for name, entry in result["pam"].items():
            status = ("native fingerprint ready" if entry["fingerprint_directly_enabled"]
                      else "fingerprint not directly configured")
            print(f"{name}: {status} (PAM={entry['service']})")
        print("KDE setting:", result["kde_fingerprint_selection"])
        print("Note: no driver can force unsupported apps to offer biometric login.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
