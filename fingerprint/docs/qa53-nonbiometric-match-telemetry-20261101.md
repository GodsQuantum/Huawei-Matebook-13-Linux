# QA53 — diagnostic matcher, aucune modification biométrique
- Base QA49 actuellement installée sur Pegasus, uniquement pour instrumenter la même implémentation.
- Échec réel 2026-11-01 09:49 S3: ImageBase neuve, scores 2/7–3/7; 10:17 LOCK: ImageBase vieille 27 min, refresh rejeté au contact, scores 3/7, 2 erreurs de FDT_DOWN_PROOF.
- Le mode existant GXFP_MATCH_DIAGNOSTICS=1 produit des traces WARN de scores par vue, nombre de vues, nombre de keypoints du probe et des métriques overlap/ZNCC. Pas d'image, template, donnée TLS ou identifiant biométrique brut journalisé.
- Identiques : logique de matching, seuil7, match/PIXEL_RESCUE, TLS, FDT, ImageBase, capture, nettoyage, PAM, KDE, S3/S4.
- Seulement candidat de diagnostic; aucune amélioration de vitesse/fiabilité revendiquée.
- Hypothèses à confronter: orientation non compensée dans BRIEF-256 ; couverture du template ; qualité physique (pression/surface) ; référence ImageBase. Aucune modification de matcher avant validation genuine/impostor.
