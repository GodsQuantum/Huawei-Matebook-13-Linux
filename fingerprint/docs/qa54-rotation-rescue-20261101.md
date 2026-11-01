# QA54 — expérimentation rotation-gated et diagnostic de qualité

Version dérivée de la QA49 fonctionnelle sur Pegasus ; QA53 (observabilité) intégrée. Ne pas toucher aux empreintes d'origine.

## Changement fonctionnel opt-in

GXFP_ROTATION_RESCUE_QA=1 active, **uniquement en Verify**, une recherche sur **la même capture biométrique** pour des rotations bornées ±6°/±12°. Elle ne s'exécute que si le score historique est entre 3 et 6 sur 7 ; recherche toujours une seule vue originale enregistrée, et exige cumulativement score FAST/BRIEF>=7, recouvrement>=1500px, ZNCC>=0,750 et accord photométrique>=0,845. Le score n'est pas additionné entre les vues ; aucune capture supplémentaire, aucun nouvel enrôlement nécessaire. Identify reste avec la voie historique sans l'exploration des rotations.

Si rien ne passe, score/acceptation QA49 inchangés. Rotation CPU augmente le temps des captures faibles, à mesurer sur Pegasus.

Aucune donnée de fingerprint sérialisée, partagée ni enregistrée. L'image de travail transformée est effacée de la RAM. Le mode reste **expérimental**, car pas de vrai corpus genuine/impostor pour le ST411 : le seuil7 inchangé ne prouve pas à lui seul le maintien du FAR lorsque plusieurs orientations sont testées.

## État

Exiger build CT700, test de non-régression C synthétique, réversibilité paquet QA49, puis essai réel avec consentement opérateur ; pas de promotion en production sans historique d'échecs/succès et contrôle d'un doigt non enregistré.
