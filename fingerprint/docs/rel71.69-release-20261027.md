# Release rel71.69 — 27 octobre 2026

Référence native Goodix GXFP51A0 ST411 pour Huawei MateBook 13. Code du pilote identique aux 3 essais réels réussis ; la version finale embarque les réglages natifs au lieu du fichier QA externe.

## Preuves matérielles
- LOCK 1 pose, score 7/7, 1,374 s après la détection.
- Réveil après hibernation S4 : 1 pose, score 11/7, 1,826 s après la détection ; durée totale du S4 non mesurée.
- Vrai deep S3 : 1 pose, score 15/7, préchauffage 5,153 s, réveil→succès 8,218 s, détection→succès 2,054 s.
- CS SPI natif : 76 µs entre CS actif et début des données contre 6 µs auparavant, selon ftrace sur pxa2xx. Comparaison unique fprintd : 7,971 s → 6,184 s pour initialiser.
- Seuil de vérification 7, TLS GCM, référence FDT vérifiée et mot de passe de secours conservés.

## Installation
Pilote seul : depuis un clone GitHub, lancer ./fingerprint/install.sh sous le compte de session. Paquet Arch/CachyOS : libfprint-goodix51a0-1.94.100.goodix51a0-71.69-x86_64.pkg.tar.zst ; SHA-256 38039bc3b745f2a2661c1c04ba3264f8792163d975f929679b6d7b3fd69a4ee4. La configuration du paquet porte exactement GXFP_S3_FRESH_BG_QA=1, GXFP_FDT_DOWN_PROOF_QA=1, GXFP_SPI_CS_SETUP_QA=1. Les noms historiques QA sont conservés pour maintenir le coeur identique aux essais. Source multi-distributions sous Linux install-linux.sh avec les mêmes valeurs sur les systèmes utilisant systemd, et dans le wrapper non-systemd.

Le binaire se présente à libfprint/fprintd. KDE/Plasma Login Manager et Polkit peuvent s'en servir avec leur pile PAM native. Les applications qui ne consomment pas PAM/fprintd nécessitent leur propre prise en charge ; sudo et system-auth ne sont pas modifiés globalement.

## Installation de référence sur Pegasus
Package Arch définitif réinstallé avec pacman, snapshots Snapper pre1942/post1943. Ancien override local /etc/fprintd QA supprimé après avoir vérifié les mêmes valeurs dans le fichier du paquet. fprintd actif et package 32/32 fichiers intacts. Doigt index droit enregistré conservé. Paquets de retour arrière 71.67 SHA256 08681c32c1234b0fae0f6f61017d50df18f75bc1528a62fdb3d864b300e4d09c et 71.64 SHA256 c3df31e8dd06bfc778c661a023333a632f2824b4e841bbdf524ea1c4aa46e932 préservés.

## Limites
Release de référence sur la machine mesurée, PAS une certification exhaustive de tous les BOOT, reboot, S3 répétés, doigt inconnu ni machines étrangères. Aucun S3 ou reboot imposé par un agent après cette installation. Evidence : rel71.69-hardware-final-20261027.md et HALL_OF_FAME.md.
