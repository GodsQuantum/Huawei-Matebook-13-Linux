# QA56 — essais physiques LOCK et S3 du 1er novembre 2026

Source : journal réel du service fprintd sur Pegasus, 16:33:10–16:34:20 Europe/Paris, horodatages microsecondes récupérés par SSH Cloud9. Retour utilisateur : « LOCK : 1ere pose et rapide » et « S3 : 2 pose mais la deuxieme etait rapide ». Les poses physiques sont confirmées par le retour utilisateur ; aucun échantillon/image/template biométrique n'est inclus.

## LOCK : réussite à la première pose

- Package actif : libfprint-goodix51a0 1.94.100.goodix51a0.qa56-1 ; mode FDT-down strict typé et fenêtre QA56 200ms ; rotation QA54 inchangée.
- 16:33:18.353235 FAST_READY 51ms, aucun doigt sur le capteur. ImageBase vieillie à 771873ms, WARM_REBASE frais en 803ms **avant** VERIFY READY 16:33:19.180809.
- Première pose DETECTED_HOLD 16:33:19.842273, zones=6, mean220, drop134. Réponses FDT-down 0x32 : trois échanges valides, latences second rapport **61, 61 et 82ms**, tous dans l'ancienne fenêtre100ms ; window_ms=200 uniquement car le nouvel opt-in est actif.
- Baseline matcher 3/7, rattrapage par rotation QA54 angle −12°, rotated_score8 (autre échelle, ne pas comparer au score final), view8, overlap3629, ZNCC791/1000, accord861/1000 ; score d'authentification final **7/7**.
- Frame573ms, SIFT4ms, match42ms, capture620ms, cleanup571ms ok=1. AUTH_TRACE verified positive terminal 16:33:21.035477 ; contexte chaud conservé après Close.
- DETECTED_HOLD → terminal **1193.204ms** (CSV 1193) ; READY → terminal **1854.668ms** ; FAST_READY log → terminal **2682.242ms**. Champion QA54 : 1157.858ms, donc QA56 **+35.346ms (+3.05%)** sur un essai ; ce n'est pas un nouveau record de vitesse.

## S3 : échec pose 1, réussite pose 2

- ImageBase rafraîchie en 839ms avant sommeil, référence propre RAM-only sauvegardée, âge=76ms ; NATIVE_S3_PARK 16:33:47.567608 **idle=1 attempted=1 ack=1 warm=1** (contrairement au précédent QA55 warm=0).
- NATIVE_S3_RESUME_BEGIN 16:34:05.986337 idle=1. TLS initial et PMK cache passent ; RESUME_PERF_QA total_ms=3542 success=1, avec 910ms stage_reset et 2631ms cold_tls_background ; préchauffage prêt 16:34:09.529405.
- Doigt déjà présent durant la préparation ; utilisation d'une référence antérieure propre âgée21951ms et non d'une image contaminée. FAST_READY84ms. ImageBase âgée24138ms >20s ; WARM_REBASE correctement différée car doigt déjà présent.
- Pose1 DETECTED_HOLD **16:34:11.890969**, zones=6. Score initial **2/7**, capture933ms, cleanup752ms ok=1. Levée à 16:34:13.576726 puis relâchement à 16:34:14.663124. Recalibration **WARM_REBASE 1222ms** sans doigt avant réessai.
- Pose2 DETECTED_HOLD **16:34:16.086785**, zones=5. Baseline3/7, ROTATION_RESCUE_QA angle −12°, rotated_score11, view6, overlap4184, ZNCC857/1000, accord885/1000 → authentification effective **7/7**. Frame897ms, SIFT3ms, match21ms, capture921ms, cleanup757ms ok=1. AUTH_TRACE positive 16:34:17.766192.
- Pose2 DETECTED_HOLD → terminal **1679.407ms** (CSV 1679) ; première détection → succès **5875.223ms** ; RESUME_BEGIN → succès **11779.855ms**, inclut repositionnement, donc à ne pas présenter comme durée de matcher. QA54 S3 montrait 1711.333ms pour une capture positive mais poses humaines incertaines, non comparable comme première pose.
- Six échanges 0x32 distincts, tous ACK+rapport valide : 62,61,61ms (pose1), puis 61,61,61ms (pose2). Aucune erreur FDT_DOWN_PROOF ni nouvelle négociation TLS induite pendant ces deux poses.

## Décision et lacunes

- Observé : LOCK première pose + S3 deuxième pose ; **9/9** rapports FDT 0x32 valides, latences **61–82ms** ; **0** FDT_LATE_REPORT_QA accepted >100ms, **0** FDT_DOWN_PROOF incomplete. Il est impossible d'attribuer les résultats positifs à la fenêtre QA56 200ms : **aucune réponse >100ms n'a été observée**.
- L'acceptation dans les deux sessions dépend du sauvetage par rotation **déjà présent dans QA54**, et non d'un matcher modifié dans QA56. Les deux essais montrent une stabilité matérielle meilleure que QA55, mais ne démontrent pas un bénéfice causal de QA56 ni une certification FAR.
- Le premier contact S3 reste rejeté (2/7) ; tester davantage de LOCK/S3, de vraies reprises S4 et un doigt non enregistré avant toute promotion. Ne pas ralentir/affaiblir la validation des paquets ni baisser le seuil7.
- QA56 reste candidat expérimental sur Pegasus, PR #59 reste DRAFT ; QA54-1 et empreinte d'origine conservées comme rollback. Le Hall of Fame doit conserver le record QA54 à 1157.858ms.
