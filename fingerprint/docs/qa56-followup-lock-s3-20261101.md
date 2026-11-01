# QA56 — deuxième série physique : quatre LOCK et deux S3 (1 novembre 2026)

**Preuve :** journaux `fprintd` Pegasus entre 16:50:11 et 16:51:53 CET, récupérés en lecture seule via SSH/Cloud9. Version effectivement installée `libfprint-goodix51a0 1.94.100.goodix51a0.qa56-1`, service actif. Retour utilisateur : « j'ai fait plusieurs lock et 2 S3, etudie les resultats ». Les mesures de capture sont traçables ; le nombre de poses physiques exact n'a PAS été précisé par l'utilisateur pour cette série. Les résultats `first_touch=unknown` signifient que seule la première capture logicielle est attestée, pas la première pose physique. Aucun élément biométrique brut archivé.

## Quatre LOCK 16:50

| Session | Détection initiale | Nombre captures | Score final (seuil 7) | Détection de la pose acceptée → terminal | Détection initiale → terminal | Résultat |
|---|---|---:|---:|---:|---:|---|
| #1 | 16:50:11.532834 | 2 | 16/7 | 1719.019 ms | 6953.521 ms | 1er score3/7 rejeté ; 2e accepté |
| #2 | 16:50:23.993116 | 1 | 10/7 | 1696.057 ms | 1696.057 ms | acceptation directe |
| #3 | 16:50:32.084544 | 1 | 13/7 | 1713.279 ms | 1713.279 ms | acceptation directe |
| #4 | 16:50:39.274858 | 1 | 7/7 | 1724.726 ms | 1724.726 ms | rotation QA54 −12° sauve score nominal3 |

FAST_READY = 84,83,84,84ms. Captures physiques dans les cinq tentatives entre 899–923ms, cleanup 750–767ms et `ok=1`. Dans LOCK#1, entre les deux captures, une ImageBase renouvelée a été **jetée** : `WARM_REBASE discarded: finger landed during background capture`, le doigt était arrivé avant la fin de la calibration. Une seconde capture a cependant obtenu score16 sans TLS reset.

## Deux reprises S3 16:51

### S3 #1 (16:51:15.823045 → 16:51:22.675219)

- Avant suspension : `NATIVE_S3_PARK idle=1 attempted=1 ack=1 warm=1` à 16:50:49.131609, référence de fond propre âgée87ms.
- Préparation au réveil native 4147ms, `FAST_READY`84ms. FDT et TLS ouverts. Une seule capture détectée à 16:51:20.881670 : score nominal4 puis rotation −12° score7/7. Capture945ms, image903ms.
- **Erreur critique après l'image** : FDT-down final `replies=1 ack=1 valid_data=0` après la fenêtre **200ms**, à 16:51:22.674938 ; `FDT_DOWN_PROOF incomplete`, cleanup847ms `ok=0`, invalidation de contexte. Le mécanisme `POSTAUTH_CLEANUP_DEFER_QA` conserve néanmoins l'authentification préalablement obtenue, terminal positif à 16:51:22.675219.
- Détection → terminal **1793.549ms**. Réveil commencé → succès **6852.174ms** ; cette durée serait inférieure au record S3 wake→success 6951ms, mais **n'est pas éligible** à un record première pose faute de confirmation du nombre de poses physiques et à cause du cleanup échoué. Il n'y a pas de validation S3 complète.

### S3 #2 (16:51:41.703230 → 16:51:53.020634)

- Après l'échec du cleanup S3 #1, park à 16:51:33.678370 : `idle=1 attempted=0 ack=0 warm=0` ; pas de préparation propre conservée. Ceci **n'est pas** le bug `idle=0` du MateBook 2020.
- Prewarm native 4155ms, `FAST_READY`83ms, fond propre antérieur restauré mais vieux ~58.245s; actualisation `WARM_REBASE` différée du fait d'un doigt présent.
- Capture 1 à 16:51:47.548953, score4/7 insuffisant ; **nouveau défaut FDT au cleanup** à 16:51:49.434474, `replies=1 ack=1 valid_data=0` malgré 200ms ; reinitialisation GPIO264 / MCU et reconfiguration TLS, sans baisse de la sécurité.
- Capture 2 à 16:51:51.288276, match initial3/7, rotation QA54 −6° score9, score effectif7/7. Image898ms, cleanup763ms `ok=1`, positif à 16:51:53.020634.
- Durée de la pose validée **1732.358ms**, première détection→succès **5471.681ms**, début du réveil→succès **11317.404ms**. Deux événements `DETECTED_HOLD`, mais leur attribution à deux placements *physiques* n'est pas attestée : le pilote remet son compteur à 1/3 après la récupération MCU.

## Protocole et causalité QA56

- Cinq captures LOCK et trois captures S3 = **8 captures**, chacune émettant trois commandes 0x32 attendues = **24 échanges**. **22 complets/24** (91,7%), deux rapports secondaires absents aux cleanups des deux S3. Les 22 réponses valides ont un temps ACK→report **60–76ms**, donc toutes seraient passées sous la fenêtre QA54 de 100ms.
- Aucune ligne `FDT_LATE_REPORT_QA accepted=1` : le chemin >100ms propre à QA56 n'a encore sauvé aucun échange observé. Il n'est pas prouvé que QA56 améliore le protocole comparé à QA54 ; la rotation qui sauve les faibles scores est héritée de QA54.
- Les deux échecs FDT post-capture s'accompagnent d'un `warm=0` au park suivant ou d'une recovery MCU/TLS. La priorité technique reste le protocole de fin de capture 0x50→0x32 et la récupération du contexte chaud ; **ne pas** résoudre en acceptant un ACK sans rapport validé, en rejouant à l'aveugle 0x32 ou en abaissant le seuil biométrique7.
- S3 #1 conserve un résultat positif malgré un cleanup défaillant (comportement QA54 déjà existant). Le bénéfice de ce compromis pour l'authentification est observé, mais la fiabilité transport reste NON certifiée.
- Sur l'ensemble QA56 observé aujourd'hui : **5 LOCK** = quatre succès sur une capture logicielle et un après deux ; **3 S3** = un succès sur une capture et deux après deux captures, les poses physiques de cette deuxième série restant à confirmer. Les 2 échecs FDT sur les 2 derniers S3 empêchent une promotion de QA56 en production. Les anciennes mesures QA54 1157.858ms LOCK restent la référence.
- Aucune modification du pilote, paquet, enrôlement ou fprintd dans cette analyse. QA56 reste en essai avec son rollback QA54-1 vérifié ; PR de code #59 reste DRAFT. Les métriques sont ajoutées au registre canonique mais les tests `first_touch=unknown` ne peuvent pas devenir des records.
