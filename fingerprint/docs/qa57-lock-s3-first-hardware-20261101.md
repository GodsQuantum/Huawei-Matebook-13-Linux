# QA57 — première série matérielle Pegasus, LOCK et S3, 1 novembre 2026

**Source de vérité** : `journalctl -u fprintd -o short-iso-precise` sur Pegasus, récupéré par Cloud9/SentinelX après le retour utilisateur : « LOCK : 1ere pose mais lente un peu ; S3 : pose ok mais lente ». Paquet installé `libfprint-goodix51a0 1.94.100.goodix51a0.qa57-1`. Aucun échantillon biométrique ou image conservé.

## LOCK 19:45:19 — première pose vérifiée

- Session `FAST_READY` **60ms** à **19:45:18.083921**. Sur ouverture, `WARM_REBASE` tenté : **discarded** à 19:45:18.937988, car le doigt a atterri pendant la capture du fond. L'ancien fond est conservé ; `VERIFY READY` à 19:45:18.943428.
- Unique `DETECTED_HOLD` **19:45:19.316071** (6/6 zones, mean227, drop127), sans autre contact avant le succès.
- Trois commandes FDT-down 0x32 valides, chaque ACK suivi d'un rapport distinct en **61ms** (`window_ms=100`), aucun échec FDT et aucun reset de transport. Score du matcher **9/7** direct, sans recours à rotation/pixel. Frame **572ms**, SIFT **2ms**, matcher **15ms**, capture **591ms** ; cleanup **565ms ok=1**.
- Auth positive à **19:45:20.472974** et contexte chaud conservé : détection→positif **1156.903ms** (arrondi CSV **1157ms**). Ancien HOF QA54 : **1157.858ms** (arrondi **1158ms**). Nouveau meilleur chrono **0.955ms** plus rapide, **non statistiquement significatif** avec un seul échantillon.
- READY→succès **1529.546ms**, premier FAST_READY journal→succès **2389.053ms**; ces durées comprennent d'autres étapes et ne doivent pas être comparées au temps doigt→succès. `protocol timing_scale` passe de 200% à 250% lors d'un ACK manqué en ouverture; pourtant image demeure rapide à 572ms. C'est le bénéfice observable de la séparation du plancher de `capture_gap_scale`.

## S3 réel 19:45:42–19:55:03, premier contact FDT manquant

- Suspension Linux deep réelle : `systemd-logind` suspend demandé **19:45:42.411159**, kernel `PM: suspend entry (deep)` **19:45:43.642818**, `suspend exit` **19:45:47.735924**. `NATIVE_S3_PARK` **19:45:43.464300** avec **idle=1 attempted=1 ack=1 warm=1**, référence propre âgée87ms. Le timeout/validation des ACK de préparation pousse `timing_scale` 250→300%, toujours sans relâcher la sécurité.
- `NATIVE_S3_RESUME_BEGIN` **19:45:47.745227**, préchauffage de reprise **3781ms** (`stage_reset_ms=910`, `cold_tls_background_ms=2870`, `success=1`), `PREWARM_READY` 19:45:51.527334. `FAST_READY` **84ms** à 19:45:52.864986.
- **Point de méthode :** le verrouillage KDE lance ensuite périodiquement des tentatives Verify pendant une longue attente sans doigt (~toutes les 32s). Les journaux n'indiquent aucune détection physique avant **19:55:00.285666**. On ne peut donc pas présenter l'intervalle réveil→succès (~9min16s) comme temps de latence du capteur ; il contient surtout une attente utilisateur/système non attribuée.
- Premier `DETECTED_HOLD` **19:55:00.285666**, contact 4/6 zones, mean278, drop77, et échec de preuve strict **FDT_DOWN_PROOF incomplete** à **19:55:00.484411**, `replies=1, ack=1, valid_data=0`, fenêtre 100ms. **Aucune image n'est prise**, score=0. L'espacement de capture reste **100% / 30000us** malgré l'erreur (log `lifecycle recovery desync`), puis reset GPIO264, transport rouvert et TLS/PMK restaurés (config256ms, handshake54ms).
- Après récupération, autre `DETECTED_HOLD` **19:55:02.521419**, contact 6/6 zones, mean197, drop156. FDT 0x32 complète 3/3 (62,61,61ms) ; frame **571ms**, SIFT **2ms**, matcher **92ms**, capture **666ms** ; score brut **5/7**, sauvetage pixel existant QA54 (`PIXEL_RESCUE`, ZNCC764/1000, accord861/1000) → **effectif 7/7**, seuil inchangé. Cleanup **560ms ok=1** ; terminal positif **19:55:03.748817**, contexte TLS recouvré.
- Détection deuxième contact→succès **1227.398ms** ; première détection→succès **3463.151ms**. Le nombre de poses physiques n'a pas été explicitement dénombré par l'utilisateur ; deux contacts logiciels après une récupération MCU/TLS ne suffisent pas à garantir une certification première pose. CSV `first_touch=unknown`, sans classement S3 de vitesse.
- 7 échanges FDT-down attendus sur les captures de ces essais (3 LOCK + 1 S3 incomplet + 3 S3 finaux) : **6/7 valides**, erreur exclusivement au début du premier contact S3. Cela maintient le bloqueur de fiabilité FDT sur reprise. Il n'est pas justifié d'accepter ACK seul, de rejouer aveuglément 0x32 ou de désactiver TLS.

## Décision

- **QA57 garde le record LOCK chronométrique (1156.903ms)** et réussit le S3 après récupération, mais **pas de promotion de la PR pilote #62** : cadence première pose/S3 répétée, doigt inconnu rejeté, S4 et stabilité FDT restent à vérifier. Son gain matériel démontré dans cet essai est la réduction de temps image malgré `timing_scale=250–300%`.
- Ne pas confondre vitesse d'un échantillon et fiabilité. QA54 est la base code stable/rollback (1157.858ms) ; QA56 n'a pas montré d'intérêt de son délai200ms et garde ses deux nettoyages post-S3 incomplets. QA57 expérimental reste installé avec `GXFP_DECOUPLE_ACK_PACING_QA=1`, seuil7 et rotation/pixel conservés ; paquet QA54 SHA vérifié et S3 sans reboot.
- Investiguer ensuite le rapport FDT 0x32 absent après S3 / redémarrage de session et le délai de préparation de fond. Ne pas modifier la sécurité du protocole pour gagner quelques millisecondes.
