# QA61 — immédiat si doigt déjà posé (état candidat)

## Symptôme établi
- QA60 premier LOCK : 2 poses, 4/7 puis7/7 (1338ms sur 2e pose).
- QA60 deuxième LOCK : au moins 6 poses, scores3/7,3/7,3/7,2/7,3/7 puis9/7 après rebase propre (1230ms sur 6e pose).
- QA60 troisième LOCK, doigt posé 2s après verrouillage : première pose positive, 1240ms depuis FDT detect ; ImageBase rebase abandonné lorsque le doigt était arrivé pendant capture.
- Hall record QA57 : 1157ms première pose. Les tentatives multiples ne sont jamais des records.

## Modification QA61
- GXFP_EARLY_HELD_FAST_AUTH_QA=1 opt-in, OFF par défaut, ON dans le paquet expérimental QA61.
- Le FDT valide à Claim mémorise uniquement l'indication de contact et un timestamp monotone RAM ; validité <=500ms dans le worker.
- Si cette preuve montre un doigt déjà posé, le worker saute la tentative redondante de rafraîchissement de fond qui serait nécessairement refusée. Il conserve la DERNIERE ImageBase propre (aucune capture de fond avec doigt) et poursuit le chemin natif image TLS, score>=7 et cleanup.
- Contact déjà prouvé y compris contact partiel : premier polling FDT 1ms uniquement avec flag QA49 déjà actif ; second FDT contrôle la réalité de la pose, sans authentifier le doigt à lui seul.
- Sans doigt préposé, sans preuve récente ou flag OFF, le chemin QA60 d'origine s'applique. Aucun service/timer externe, aucune réinitialisation/suppression d'empreinte, aucun seuil ou retries augmentés.
- Ce changement améliore le temps de réaction, mais ne garantit PAS le score de match quand la référence est très ancienne. La fiabilité une pose, doigt faux, S3/S4 et performance réelle restent à certifier.

## Validation
- QA61 tests nouveaux4/4 et baseline logiciel CT700 PASS ; build libfprint v1.94.100 + driver C, Ninja 116/116, linker et ABI Goodix/PAM PASS ; release dump biométrique ABSENT.
- Installation Pegasus et vérification biométrique humaine non effectuées au moment de la création du document.
- Rollback QA60 et QA59.2 paquets caches Pegasus préservés.
