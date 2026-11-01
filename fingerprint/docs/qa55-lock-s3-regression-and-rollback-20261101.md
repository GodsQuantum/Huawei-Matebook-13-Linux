# Pegasus QA55 — régression observée sur LOCK et S3, rollback QA54

**Source** : journaux du service fprintd Pegasus 2026-11-01, entre 15:57:14 et 15:59:08 Europe/Paris, horodatages à la microseconde, vérifiés via SSH depuis Cloud9. Retour utilisateur : « Analyse les résultats du lock c bizarre » ; « S3 : échec ». Paquet réellement installé pendant les essais : libfprint-goodix51a0 1.94.100.goodix51a0.qa55-1, option native GXFP_CANCEL_PENDING_POLL_QA=1 et toutes les options QA54. Aucun élément biométrique brut conservé.

## LOCK 15:57 — réussite très tardive, non première pose

- 15:57:14.438008 : FAST_READY **72 ms**. Référence de fond rafraîchie en **869 ms**.
- 15:57:18.020806 première détection. Cinq poses détectées avant l'acceptation : scores nominaux **3/7, 3/7, 4/7, 5/7**, puis **4/7** finalement **effectif 7/7** par PIXEL_RESCUE (view15, overlap2252, zncc_milli824, agreement896). La pose finale est 15:57:38.778169, résultat positif 15:57:40.063407 soit **1285,238 ms** sur la pose finale.
- Premier contact détecté → succès : **22 042,601 ms**, arrondi **22043 ms** au CSV. L'ancien record QA54 1157,858 ms concerne un succès **dès la première pose** : les résultats ne doivent pas être comparés comme s'ils mesuraient le même événement.
- 15:57:30.576206 FDT_DOWN_PROOF incomplete replies1/2 lors de la fin de capture, avec recovery MCU/TLS. Recalibration perturbée par la présence du doigt (CALIBRATION_LIFT_REQUIRED), ImageBase avec contact rejetée et référence de secours précédente restaurée. 15:57:40.063217 nouvelle erreur FDT_DOWN_PROOF 1/2 au cleanup malgré authentification effective, dont résultat préservé par POSTAUTH_CLEANUP_DEFER_QA. **D'où le LOCK inhabituellement lent**. La capture elle-même restait ~571–572ms avant récupération, matching initial ~100ms sur poses de faible score.
- Ce test ne prouve pas que le nouveau callback d'annulation QA55 cause ces bas scores; ce callback n'intervient que lors de cancellation, pas dans l'acceptation d'une pose normale.

## S3 15:58 — authentification échouée malgré reprise native

- Avant veille 15:58:06.653732 : `NATIVE_S3_PARK idle=1 attempted=0 ack=0 warm=0`. Le contexte était déjà non chaud, conséquence cohérente avec FDT_DOWN_PROOF incomplet au LOCK; le capteur n'a donc pas pu envoyer un SLEEP ACK préparé comme dans le cas sain QA54.
- 15:58:25.878675 : `NATIVE_S3_RESUME_BEGIN idle=1` — **pas** la course `idle=0` que QA55 cherche à corriger sur MateBook 2020.
- Préparation native `RESUME_PERF_QA ... total_ms=3555 success=1` ; TLS config + handshake premiers essais OK, `RESUME_PREWARM_READY` à 15:58:29.435007.
- La capture du fond pendant le réveil a rencontré un doigt présent, rejetée ; récupération `NATIVE_S3_EARLY_FINGER_VERIFIED_REFERENCE` et `S4_MIDCAPTURE_SAVED_REFERENCE` depuis référence ancienne d'**887239 ms (~14,79 minutes)**. Au Claim, FAST_READY **83ms**, ImageBase âgée d'**889205ms**; `WARM_REBASE deferred: finger already present`.
- 1ère pose DETECTED_HOLD à **15:58:31.575078**, score 3/7. Après retrait, refresh **1215ms**; scores restés bas (**2–3/7**). Le capteur a détecté au moins **8 contacts** et enregistré 7 scores faibles + un échec de transport avant que l'utilisateur annule. La 8e détection est à 15:58:57.420359 et subit FDT_DOWN_PROOF incomplet avant image. Échecs FDT 1/2 également à 15:58:55.555926 et 15:58:58.052356; déclenchements MCU/session recovery.
- 15:59:08.083846 QA55 `CANCEL_PENDING_POLL_QA state=1 removed_fdt_source=1` : annulation finale propre. Aucun événement `positive terminal` après S3. Score final du CSV=3/7 = meilleur score nominal observé.
- La détection FDT est complète la plupart du temps (6 zones) et les trames sont capturées (environ 897–905ms); faibles scores photométriques et correspondances insuffisantes. Causes possibles : condition du doigt / fond / orientation, à confronter avec un retest QA54. **La différence de code QA55 ne suffit pas à attribuer ces échecs à sa fonction cancel.**

## Action, rollback et suites

- **Rollback effectivement réalisé** le 1er novembre : SHA256 archive QA54 vérifié (d3fe51eb826b009e80612a88e48642c0ef3d47c80fe37a29ef15e3fa88e1b174), paquet QA54-1 restauré via pacman, drop-in `70-gxfp-qa55-native-cancel.conf` supprimé, systemd rechargé, fprintd redémarré. `pacman -Qkk` 32/32 sans modification, `fprintd` actif, empreinte index droit intacte, option QA54 `GXFP_ROTATION_RESCUE_QA=1` conservée.
- QA55 reste **DRAFT et NON CERTIFIÉE** pour deployment humain tant que ses essais LOCK/S3 échouent. 12/12 CI initiaux et harness C prouvent que le code compile et que le cancel des timers fonctionne, **pas** la fiabilité biométrique.
- Priorités : retest QA54 avec vraie première pose LOCK/S3 et logs exacts; rechercher pourquoi FDT 0x32 n'envoie pas systématiquement son second rapport malgré fenêtre100ms et pourquoi un contexte warm invalide au park force la vieille référence. Ne pas baisser threshold7, ni supprimer le cleanup obligatoire; ne pas réenrôler automatiquement. Conserver le rollback et ne pas confondre le problème de sleep-during-identify 2020 (idle=0) avec ce S3 2021 (idle=1).
