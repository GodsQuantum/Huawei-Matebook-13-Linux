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
