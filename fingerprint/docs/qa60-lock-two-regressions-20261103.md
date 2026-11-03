# QA60 — deux LOCK humains sur Pegasus, 3 novembre 2026

Pilote : libfprint-goodix51a0 qa60-1, fprintd 1.94.5-2.1, capteur ST411. Rapports utilisateur : 2 poses au premier LOCK, au moins 6 au second.

## LOCK 1 — 11:47:06
- FAST_READY 61 ms ; ImageBase âgée de 1 691 398 ms, doigt déjà posé, refresh différé.
- Premier DETECTED_HOLD 11:47:06.651727 : score 4/7 (rejet), capture 595 ms, FDT typed complet.
- Second DETECTED_HOLD 11:47:10.289184 : rotation +6 degrés, score terminal 7/7, capture 602 ms, match 98 ms, cleanup 631 ms, positif 11:47:11.627695, doigt-to-positif = 1338 ms (deuxième pose).
- Aucun diagnostic GPIO48 post FDT ACK-only déclenché.

## LOCK 2 — 11:47:50
- FAST_READY 295 ms ; ImageBase âgée de 1 735 132 ms, doigt déjà posé.
- Cinq premières captures scores : 3/7, 3/7, 3/7, 2/7, 3/7 (détections présentes, FDT ACK+DATA valides, images 574–615 ms). Tentatives de rebase échouées : doigt arrivé pendant capture du fond.
- WARM_REBASE réussie 11:48:04.317308, référence propre en 888ms (idle354 floor330).
- Sixième DETECTED_HOLD 11:48:04.808048 : score 9/7, capture578ms, match22ms, terminal 11:48:06.037603, 1230ms sur cette sixième pose. Le nombre physique >=6 est rapporté par utilisateur ; 6 détectées.
- Pas de FDT ACK-only, ni erreur TLS constatée. Le gain après rafraîchissement de la référence renforce l'hypothèse de fond périmé, sans isoler définitivement angle/contact.

## Comparaison Hall et décision
- Champion première pose : QA57+kde47 1157ms ; QA54 1158ms ; QA42 1181ms.
- QA60 0/2 première pose ; les 1338ms et 1230ms sur la pose finale ne sont PAS des records première pose.
- Ne pas réenrôler immédiatement : même index inscrit a obtenu 7/7 puis 9/7 ; seuil7 et mot de passe préservés.
- Prochain A/B non destructif : déclencher LOCK, patienter 1–2 secondes sans doigt sur capteur avant poser l'index ; contrôler la nouvelle référence et les scores. Ne pas réduire sécurité biométrique.
- Aucune modification au pilote, au service ou à l'enrôlement pour cette documentation. Preuves : journalctl -u fprintd horodaté Pegasus 11:47–11:48.
