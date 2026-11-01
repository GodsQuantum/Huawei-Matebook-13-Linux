# QA49 — échecs après S3 nocturne et LOCK (2026-11-01)

Source : retour humain « échec » sur deux scénarios distincts, confronté aux horodatages du journal fprintd sur Pegasus. Aucune image biométrique ni secret collecté.

## S3 à 09:49 CET

S3 confirmé par NATIVE_S3_RESUME_BEGIN. Préchauffage natif 3597 ms réussi, ImageBase propre et fraîche (288 ms), TLS valide et FAST_READY 74 ms. Au moins cinq captures successives avec scores 2/7, 3/7, 3/7, 2/7 et 3/7. FDT six zones et captures 746–770 ms, nettoyage réussi initialement. La fraîcheur de l'ImageBase ne suffit pas à rétablir le matching.

## LOCK à 10:17 CET

FAST_READY 84 ms, ImageBase 1 664 527 ms (~27,7 min). Recalibrage rejeté deux fois car doigt présent durant acquisition ; scores 3/7 sur plusieurs poses. Deux FDT_DOWN_PROOF incomplets (ACK sans trame de données) durant cleanup, récupérations MCU/TLS engagées. Le first-poll rapide QA49 n'était pas activé (FAST_READY touch=0x00).

## Conclusion

Le relevé porte sur **1 S3 échoué** et **1 LOCK échoué**, et non 2 LOCK. QA49 est maintenant à 1/2 LOCK première pose documentés et 0/1 S3 pour ces essais. La capture et le FDT fonctionnent, mais les empreintes restent trop peu corrélées à l'enrôlement. Ne pas assouplir seuil7, fusionner les vues ni supprimer les empreintes. Instrumenter keypoints et scores par vue pour départager couverture/orientation/contraste avec essais genuines/imposteurs.
