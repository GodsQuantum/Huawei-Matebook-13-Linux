## QA54 — base canonique de développement (2026-11-01)

- Base choisie: QA54 testée sur Pegasus, libfprint-goodix51a0 1.94.100.goodix51a0.qa54-1. Paquet SHA256 d3fe51eb826b009e80612a88e48642c0ef3d47c80fe37a29ef15e3fa88e1b174. Fichiers du pilote et des tests identiques aux sources QA54 du commit 8ec86a6.
- LOCK première pose: DETECT→terminal 1157,858 ms, score 12/7 sans rotation. S3: DETECT→terminal 1711,333 ms, score initial 3/7 récupéré par +12 degrés sur une unique capture (score brut15, recouvrement4391, ZNCC812, accord873). Nombre total de poses de l'utilisateur S3 incertain. Voir docs/qa54-lock-and-s3-first-physical-20261101.md et docs/HARDWARE_TRIALS.csv.
- QA54 devient la base de travail des futures branches. Flag GXFP_ROTATION_RESCUE_QA=1 active seulement sur Pegasus; désactivé par défaut sur toute nouvelle installation. Maintenir le seuil SIGFM7, TLS et FDT stricts. Une observation ne certifie ni fiabilité ni FAR; tests répétés et mauvaise empreinte manquants.
- Rollback paquet QA49 préservé sur Cloud9 (private/candidates/pr49-held-first-poll-20261101); image enrolment root-only sur Pegasus. Aucune suppression ou recréation d'empreinte automatique.
- Nouvelle issue #6, commentaire du 2026-11-01: https://github.com/GodsQuantum/Huawei-Matebook-13-Linux/issues/6#issuecomment-6557944671 : identify actif interrompu par sleep (2020 s2idle) laisse fprintd busy et empêche native resume. L'API libfprint exige une annulation complète d'opération pour un suspend non pris en charge. Chercher fix natif en branche expérimentale séparée; ne pas ajouter de hook system-sleep.
- Source opérationnelle de vérité: Cloud9 Workstations/Pegasus/Fingerprint-GXFP51A0/HANDOFF_CURRENT.md. Ce préambule et le HOF supplantent les vieilles sections historiques gardées ci-dessous pour provenance.

---
## CURRENT (2026-10-28): rel71.74 on reference Pegasus, S4 still unverified
- Source of truth: GitHub main and release tag matebook13-rel71.74; normal HUAWEI and fingerprint installers use pkgrel71.74, all 11 tested native fprintd flags. Earlier rel71.69, rel71.55 and below are historical evidence only.
- Hardware: operator reported LOCK one physical pose and S3 one physical pose under rel71.74. The later attempt to enter S3/S4 was aborted by the systemd freezer BEFORE sleep (KDE kded6 and kioworker waiting in autofs_wait on /mnt/Cloud9 after Wi-Fi stopped). Unlock after aborted S4 is NOT S4 certification. Issue #32 remains open.
- Safety: keep TLS-GCM, matcher threshold 7, enrolled templates, password fallback and verified rollback packages. No automatic reboot, hibernation, global PAM change or NFS unmount to produce biometric QA.
- Audit: four-probe FDT baseline currently accepts 1/4 via if (!nb), a reliability review item not a proven biometric bypass. See docs/superpowers/plans/2026-10-28-gxfp71.74-safe-optimization.md.

---
## 2026-10-27, 17:54 S4 FAILURE CONFIRMED — no production change
User reports GXFP51A0 fingerprint unavailable after battery-associated hibernation. Kernel confirms KDE-triggered S4 17:41:47→17:54:03, NOT S3, NOT poweroff; battery at 17:57 was 8% charging. Native fprintd rel71.69 PID507163 and enrolled right index intact, flags 3 default. PRE-SLEEP background saved 17:41:44, resume 17:54:03, contaminated FDT touch=0x3f repeatedly at 17:54:09/19/30/35, clean prewarm ready 17:54:41 (38.69s). VERIFY_READY 17:54:43 then CLOSE 17:54:45, no detected finger/image/score. True user-visible S4 FAIL. Contrast S4 at 17:19 also showed 18+s calibration but one positive scan score20/7 at 17:19:49. No proof whether physical finger or electrical ghost created FDT touch; do NOT blame user. Source hypothesis: gx_s3_try_early_finger_reference only checked BEFORE background capture, never when live contact contaminates in-progress capture. Need lab fix to reuse RAM-only age-bounded, verified clean S4 snapshot on midcapture contamination, preserving strict TLS image and threshold7. Avoid fixed claimed without repeated physical S4. Release rel71.69 marked GitHub PRERELEASE experimental, issue #32 created. Production driver unchanged; DO NOT reboot/S4 force, alter PAM/global/security or touch enrolled finger. Preserve rollback rel71.67/71.64.

# GXFP51A0 / libfprint — état de maintenance

**27 octobre 2026 : rel71.69 publiée et testée sur MateBook 13 de référence.** L'ancien handoff cumulatif est conservé à l'identique dans docs/HANDOFF_FULL_UNTIL_20261027_PRE_HYGIENE.md ; ne pas suivre ses anciennes tâches.

## Release et preuves
- Source [GitHub GodsQuantum/Huawei-Matebook-13-Linux](https://github.com/GodsQuantum/Huawei-Matebook-13-Linux) sur main ; release officielle matebook13-rel71.69. Installation depuis ./fingerprint/install.sh ou HUAWEI.
- Rel71.69 : vrai LOCK première pose, score7/7, détection→succès1,374s ; après hibernation S4 première pose score11/7, 1,826s ; après S3 réel première pose score15/7, préchauffage5,153s, réveil→succès8,218s, doigt→succès2,054s.
- CS SPI50 µs : traçage noyau, 6→76 µs CS actif→début transfert ; pas une preuve à l'oscilloscope. TLS-GCM, FDT et seuil matcher7 conservés.
- Le paquet rel71.69 contient le même libfprint que celui testé, config native fprintd GXFP_S3_FRESH_BG_QA=1 GXFP_FDT_DOWN_PROOF_QA=1 GXFP_SPI_CS_SETUP_QA=1.
- SHA256 paquet : 38039bc3b745f2a2661c1c04ba3264f8792163d975f929679b6d7b3fd69a4ee4 ; source : fc1c30e4b3778b52086e684e560b6b361f46ac57c9a8f4d77b49ac76dd89a755.
- Repli : paquet vérifié rel71.67, puis rel71.64 conservés sur la machine de référence ; empreinte index droit conservée. Répétitions S3, cold boot, faux doigt, autres ordinateurs restent **non certifiés**.

## Travailler sur ce pilote
Lire ../docs/PROJECT_MAP.md, ../AGENTS.md puis README.md et docs/rel71.69-hardware-final-20261027.md, docs/HALL_OF_FAME.md. Source driver/goodix51a0/, package packaging/arch/, tests research/ et scripts/, histoire docs/ et Git. Ne pas modifier les règles PAM global/sudo, kernel, PMK, GPIO112, seuil7, réveil ou daemon natif sans nécessité démontrée. Aucune trace biométrique ou clé dans Git. Avant tout changement : Git status, tests, retour arrière, expérience humaine pour résultats réels.
