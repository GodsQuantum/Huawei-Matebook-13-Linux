# Native fingerprint integration — Linux / KDE / GNOME

The Goodix GXFP51A0 driver exposes the standard Linux chain:

GXFP51A0 (SPI) → libfprint-2 → fprintd (system D-Bus) → PAM / desktop.

That is the same generic integration as a fingerprint reader supported
directly by a distribution. There is no proprietary "fingerprint detection"
service, GUI shell wrapper, persistent polling daemon or application patch.

Use the built-in GUI for enrolling and managing prints:
- KDE Plasma: System Settings → Users → Fingerprint Authentication.
- GNOME: Settings → Users → Fingerprint Login.
- Programmatic discovery: fprintd-list and the D-Bus Fprint Manager
  GetDefaultDevice method. The driver supports the standard enroll,
  verify and identify libfprint operations, with threshold=7.

## Per-application reality

| Consumer | Native mechanism | Status/limitation |
|---|---|---|
| KDE lock screen | kde-fingerprint PAM service | Supported and physically tested on Pegasus |
| KDE Plasma Login Manager | separate plasmalogin-fingerprint backend on compatible builds | QA integration installed on Pegasus; not certified cross-distro |
| GNOME GDM | gdm-fingerprint PAM service | Source-build compatibility; hardware not validated |
| Polkit administrative GUI | polkit-1 PAM service if configured | Enabled on Pegasus, password fallback remains |
| sudo or console login | distro-owned PAM stack | Deliberately not globally modified |
| KWallet / KSecretD | key derived from password at PAM session | A fingerprint match alone does not supply the password needed to decrypt a wallet |
| Browser passkeys / WebAuthn | platform or external FIDO2 authenticator | libfprint reader alone is not a WebAuthn security key |
| Apps with fingerprint support | fprintd, native PAM/Polkit or an explicitly supported authentication API | Native discovery only if the app implements that API |
| Third-party password managers | application-specific support | Cannot be forced to use system biometrics by changing the driver |

No autoediting of system-auth, global sudo, display-manager PAM, or wallet
encryption. This would risk a login lockout or weaken password handling.

## Cross-distro diagnostic

On the checkout or after installation:

    python3 fingerprint/scripts/gxfp51a0-desktop-native-doctor.py --json
    gxfp51a0-desktop-native-doctor --json

It detects supported native PAM services in /etc, /usr/lib, /usr/lib64
and /lib on Arch/CachyOS, Debian/Ubuntu, Fedora, openSUSE and Alpine.
It inspects the standard KDE screen-lock option and checks PAM module
presence without acquiring the reader, altering config or touching the
fingerprint data. The separate existing gxfp51a0-doctor performs live
D-Bus device discovery and checks enrollments.

The report does not infer that a service enables biometrics merely because
it includes system-auth: it cannot safely know the downstream distro stack.
Runtime device enumeration plus an enrolled print must be verified separately.

## Required validation

1. fprintd-list shows Goodix GXFP51A0 and an enrolled print.
2. KDE fingerprint user settings can enroll/delete normally.
3. KDE lock-screen one physical touch, password fallback.
4. Polkit GUI one touch, cancel and password fallback.
5. Plasma login first boot and KWallet behavior tested separately.
6. No changes to GPU, display stack, BIOS, kernel or global PAM.
7. Every new hardware version uses the historical Hall of Fame and per-version
   test gates; build success is not device certification.

Do not claim that apps which lack standard biometric interfaces can
discover or accept fingerprint login magically.
