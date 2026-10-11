# rel71.36 / Plasma Login Manager 6.7.5-3.10 — 2026-10-11

## Actual physical observations (Pegasus, CachyOS 7.2.9-2)

- User reported: first login after reboot via fingerprint **FAILED**; ordinary KDE lock succeeded with one pose; real deep S3 unlock succeeded on the third pose.
- Remote Desktop Commander initially showed Pegasus offline. Via authorized SSH from Cloud9 the running kernel was confirmed as 7.2.9-2-cachyos and stock fprintd active.
- Login greeter started at 10:46:48 and fingerprint PAM at 10:46:52. The real PAM cue "Placez votre index droit sur le lecteur d’empreintes" was delivered at 10:47:15, not when the password interface first appeared.
- First native TLS handshake failed at 10:47:01. During 10:47:03–10:47:14 the sensor repeatedly rejected contaminated calibration frames while a finger was touching it. The driver finally preserved production_ready=1 at 10:47:25, after the user chose password login.
- Native post-S3 park was acknowledged at 10:50:51. SPI-closed S3 reset at 10:50:57; sensor required further clean calibration and returned warm production_ready=1 at 10:51:09. No change to matcher threshold or enrolled template.
- These logs justify improving greeter readiness communication, but do NOT demonstrate instant availability or repeated first-boot success.

## UI-only patch 0010, build and delivery

- Existing project PLM package is 6.7.5-3.9 with nine reviewed KDE backports. The PLM greeter already runs password and fingerprint as parallel, separate PAM flows. Do not regress this.
- Added 0010-initializing-cue.patch: displays a passive initialization notice in French or English on a fingerprint login request; the *actual* PAM information message replaces it when the fingerprint verifier can accept a finger. Password login remains available independently.
- New package version 6.7.5-3.10. Replayed upstream KDE official source tarball + all ten patches and compared generated source byte for byte to the edited checkout.
- Existing PKGBUILD had stale SHA256 values for patches 0001–0006; repaired to actual retained source file hashes, retaining KDE upstream tarball hash and patch 0007–0010 hashes.
- Native Arch base-devel build succeeded including QML compilation. Binary-only artifact SHA256: d0f533c8dfe233de0cb5bf0a726ce589bc7ba2c45c8474a832c16d22d1695b07.
- Saved under canonical private/candidates/plasma-login-manager-6.7.5-3.10/ with SHA256SUMS and STATUS.md, Syncthing delivered a readable file on Pegasus; checksum also verified on Pegasus.
- Initial remote pacman operation timed out while Pegasus was unreachable; checked installed version before retrying and confirmed old version 3.9 remained. The second pacman -U completed successfully with snapshots 1559/1560. Pegasus now confirms plasma-login-manager 6.7.5-3.10, libfprint-goodix51a0 1.94.100.goodix51a0-71.36, kernel 7.2.9-2, services plasmalogin and fprintd active; pacman -Qkk plasma-login-manager reports 210 files and 0 modified. Existing user KDE session was not restarted or logged out. New greeter interface remains PHYSICALLY UNTESTED until the user's next login.
- Source branch fingerprint-native-s3-driver is experimental, not stable main. Do not promote stable release until hardware after-boot login, regular lock and repeated S3 tests are all verified with the new package, or honestly classify remaining limitations.

## Invariants

No additional service, boot timer, system-sleep hook or helper. No change to rel71.36 libfprint, fprintd, PAM, Goodix PMK, right-index enrollment, threshold 7, Bitwarden PIN, bootloader, kernel, NFS mounts or unrelated machines. UI readiness patch is NOT a fix for TLS initialization latency; the native boot code still takes substantial time and needs later validated improvement if necessary.
