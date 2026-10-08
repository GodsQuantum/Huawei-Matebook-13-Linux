# GXFP51A0 / GF3658 ST411 — standalone fingerprint driver

This is the **driver-only** distribution. It does not install Huawei GPU Manager, a custom GUI, or the HUAWEI control center.

**Default: rel71.30 public preview (2026-10-08).** The reference machine (Huawei MateBook 13 / GXFP51A0 / ST411) completed native KDE enrollment; eight subsequent verification captures scored 7/7 or higher. In a later operator-confirmed wrong-finger check, three attempts with an unenrolled finger scored 2, 3 and 4 (rejected), then the enrolled index scored 16 (accepted). This is a **single-machine field test**, not proof that other models, firmwares, or distributions are compatible. **rel71.24** is still the fully cold-boot/deep-S3-validated rollback.

## Supported hardware and limitations

- ACPI device ID: `GXFP51A0`, Goodix GF3658 / ST411, chip `0x2504`.
- Reference firmware: `GF_ST411SEC_APP_14115`, image 80 × 64.
- Other Goodix and Huawei revisions are **not** automatically supported.
- The native libfprint driver uses standard fprintd (D-Bus) and the desktop's existing fingerprint PAM integration.
- Arch/CachyOS binary is x86_64-specific. Linux source building targets Arch, Debian/Ubuntu, Fedora, openSUSE and Alpine, but hardware/runtime checks on those distros need community validation.
- No firmware flashing, no GPIO112 writes, no global PAM override, no biometric capture export.
- **Ubuntu installation uses this SOURCE archive, not the Arch .pkg.tar.zst binary.** In Ubuntu 24.04/26.04, the script selects `apt-get`, builds pinned libfprint 1.94.100 under an isolated `/usr/local` scope, and performs a staged ABI check against the Ubuntu `fprintd` executable before installation. Installation still requires working SPI/ACPI and an intact distribution PAM/GNOME/KDE environment; Ubuntu sensor/runtime success has **not** been measured on a real Ubuntu laptop.
- **Full Huawei/GPU installer is separate** (the complete repository's `./install.sh`): it requires Huawei DMI, the actual MX250 PCI ID `10de:1d13` for dGPU setup, systemd and a working proprietary NVIDIA R580 kernel module for the running kernel. Ubuntu 24.04 and 26.04 provide `nvidia-driver-580` packages, but available repository components, Secure Boot/DKMS and PRIME operation must be checked per machine. The driver-only archive does not include that installer.

## Option A — GitHub driver-only source archive (all supported distros)

Download the **`gxfp51a0-driver-only-rel71.30.tar.gz`** attachment from the rel71.30 GitHub release, then extract it. Its top-level directory contains **only** `fingerprint/` and no MateBook-specific GPU/application manager.

From the extracted archive root:

```bash
./fingerprint/install.sh
```

Run as your normal user; install requires `sudo` for system integration. The installer validates the GXFP51A0 ACPI device ID before making a normal installation. Arch/CachyOS builds the native pacman package; the portable installer uses an isolated `/usr/local` libfprint and distro fprintd integration.

You can alternatively clone the full repo and execute `./fingerprint/install.sh` without running the main `./install.sh` HUAWEI manager.

## Option B — Arch/CachyOS precompiled driver

Download the prebuilt `libfprint-goodix51a0-1.94.100.goodix51a0-71.30-x86_64.pkg.tar.zst` release asset. Compare its SHA-256 with the release notes and install via `sudo pacman -U` after reviewing package contents and conflicts. This replaces the distro `libfprint` package with the reviewed driver package; do not force package conflicts. The package contains the bounded pre-enumeration helper and boot prime in addition to libfprint; it is **not** just a bare .so.

## Fingerprints — native GUI only

**Do not enroll/delete fingers through the installer or a custom tool.** Open your distribution's graphical settings:

- **KDE Plasma:** System Settings → Users → Configure Fingerprint Authentication.
- **GNOME:** Settings → System → Users → Fingerprint Login (wording may vary).

Remove an existing print **only when deliberately re-enrolling that finger**; leave others intact. rel71.30 forms a connected 20-accepted-view enrollment and will request a replacement placement if the image is disconnected. The D-Bus `num-enroll-stages` value may be 21 because fprintd adds an internal identification stage.

Use native lock-screen verification. Password login remains available.

## Community test report

Please reply to the rel71.30 GitHub release discussion or the existing hardware issue. A useful **privacy-safe** report includes:

1. Device manufacturer/model/year, ACPI HID, sensor model, firmware version if known.
2. Distribution, kernel, fprintd and package versions.
3. Whether GUI enrollment completed, rough number of rejected placements, and first-placement unlock successes out of 10.
4. Whether other fingers are correctly rejected; latency and any GET_IMAGE/FDT errors.
5. Cold boot and deep-S3 resume observations, if tested.

**Never post fingerprint templates, PMKs, Windows firmware blobs, complete biometric dumps, passwords, or unique per-device key material.**

See [technical status](docs/candidate-71.30.md) and [rollback instructions](docs/validated-checkpoint-71.24.md).

---

## Français — installation rapide

Cette archive contient **uniquement le pilote et son installateur**, pas le gestionnaire HUAWEI ni les outils GPU. Depuis la racine de l'archive extraite, lancer `./fingerprint/install.sh`. Ensuite, enregistrer et supprimer les empreintes **uniquement dans les paramètres graphiques natifs** (KDE → Utilisateurs ; GNOME → Utilisateurs). Aucun enrôlement n'est lancé automatiquement en ligne de commande.

rel71.30 est la préversion recommandée : 8/8 vérifications réussies sur le MateBook 13 de référence, seuil de sécurité 7 inchangé. **La compatibilité sur d'autres machines et après démarrage à froid / deep-S3 reste à confirmer.** Conserver rel71.24 comme solution de repli validée. Publier les retours avec configuration et statistiques agrégées, jamais avec des empreintes ou secrets.
