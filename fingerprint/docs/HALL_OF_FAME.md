# GXFP51A0 — Hall of Fame matériel

Source canonique : [HARDWARE_TRIALS.csv](HARDWARE_TRIALS.csv). Les premières poses proviennent du retour humain, les mesures des journaux horodatés. Les captures biométriques et journaux bruts ne sont pas publiés. L'heure du LOCK du 23 octobre vers 13 h 17 est approximative et n'est pas utilisée pour un record de durée.

## Records valides (réussite dès la première pose)

| Discipline | Temps | Version | Essai |
|---|---:|---|---|
| LOCK : doigt → succès | **1.711 s** | rel71.55 | lock-20261024-1 |
| LOCK : KDE → succès | **3.488 s** | rel71.55 | lock-20261024-1 |
| S3 : préchauffage | **4.254 s** | rel71.55 | s3-20261023-1 |
| S3 : réveil → succès | **6.951 s** | rel71.55 | s3-20261023-1 |

## Fiabilité par version

| Version | Situation | 1ère pose | Certification de ce scénario |
|---|---|---:|---|
| rel71.55 | LOCK | 4/5 | ÉCHEC de première pose |
| rel71.55 | S3 | 2/3 | ÉCHEC de première pose |
| rel71.55 | REBOOT | 1/1 | Nombre insuffisant |
| rel71.56 | S3 | 0/1 | ÉCHEC de première pose |
| rel71.56 | COLD | 0/1 | ÉCHEC de première pose |
| rel71.58 | LOCK | 1/1 | Nombre insuffisant |

## Essais horodatés

| Essai | Heure (Europe/Paris) | Version | Mode | Poses | Première | Préwarm S3 | FDT | Doigt→succès | KDE→succès | Réveil→succès | Score |
|---|---|---|---|---:|---|---:|---:|---:|---:|---:|---:|
| reboot-20261023-1 | 2026-10-23T13:12:03+02:00 | rel71.55 | REBOOT | 1 | pass | — | — | — | — | — | 10/7 |
| lock-20261023-1 | 2026-10-23T13:17:00+02:00 | rel71.55 | LOCK | 1 | pass | — | 0.083 | — | — | — | 15/7 |
| lock-20261023-2 | 2026-10-23T13:42:51.156+02:00 | rel71.55 | LOCK | 2 | fail | — | 0.084 | — | — | — | 10/7 |
| s3-20261023-1 | 2026-10-23T14:11:54.797+02:00 | rel71.55 | S3 | 1 | pass | 4.254 | 0.084 | 2.073 | — | 6.951 | 14/7 |
| cold-20261024-1 | 2026-10-24T12:41:23+02:00 | rel71.56 | COLD | 2 | fail | — | 0.082 | — | — | — | 7/7 |
| s3-20261024-fast | 2026-10-24T15:11:13.860+02:00 | rel71.56 | S3 | multiple | fail | 27.725 | 0.082 | 1.950 | — | 33.595 | 10/7 |
| lock-20261024-1 | 2026-10-24T15:21:10.733+02:00 | rel71.55 | LOCK | 1 | pass | — | 0.072 | 1.711 | 3.488 | — | 15/7 |
| lock-20261024-2 | 2026-10-24T15:25:02.751+02:00 | rel71.55 | LOCK | 1 | pass | — | 0.072 | 1.754 | 4.555 | — | 7/7 |
| lock-20261024-3 | 2026-10-24T15:36:11.402+02:00 | rel71.55 | LOCK | 1 | pass | — | 0.083 | 1.739 | 4.391 | — | 9/7 |
| s3-20261024-2 | 2026-10-24T18:24:49.821+02:00 | rel71.55 | S3 | 1 | pass | 6.127 | 0.083 | 1.955 | — | 10.450 | 7/7 |
| s3-20261024-3 | 2026-10-24T19:06:06.964+02:00 | rel71.55 | S3 | 3-4 | fail | 3.798 | 0.083 | 17.090 | — | 22.377 | 16/7 |
| lock-20261024-7158 | 2026-10-24T19:33:31.450+02:00 | rel71.58 | LOCK | 1 | pass | — | 0.293 | 1.902 | 4.007 | — | 11/7 |

## Comparaison anti-régression des LOCK

- rel71.55 après rollback : médiane **1.739 s** sur 3 essais (doigt → authentification).
- rel71.58 : **1.902 s** sur 1 essai(s). Écart **+9.4 %** ; pas de gain revendiqué et échantillon insuffisant.

## Gates obligatoires et politique d'enregistrement

1. Ne pas mélanger différentes versions ou installations. Un score réussi après plusieurs poses ne compte PAS comme première pose.
2. Pour une même version : 5 LOCK consécutifs première pose, 3 vrais S3 deep (dont une suspension pendant Verify), 3 cold boots, 3 reboots, doigt inconnu rejeté, mot de passe valide, aucune désynchronisation du capteur.
3. Ne pas relâcher le seuil de correspondance **7**, ne pas supprimer le nettoyage TLS, ne pas imposer de reboot/veille sans la participation de l'utilisateur.
4. Comparer uniquement des mesures de même nature. FDT FAST_READY ne mesure ni l'affichage du greeter ni le délai total de sortie de veille. Aucun S3 échoué ne peut détenir un record de vitesse.
5. À chaque essai : lire journal, calculer les timings, attester les poses avec l'utilisateur, ajouter une ligne au CSV, réviser la fiche de preuve, régénérer cette page avec l'option --write du script gxfp51a0-hall-of-fame.py, lancer --check et make verify, commit et push.

**Statut :** rel71.55 est le rollback de référence mais a échoué à la première pose sur 1 des 3 S3 mesurés. rel71.56 S3 rapide rejeté (27,725 s). rel71.57 instrumentation non installée. rel71.58 correction S3 en QA, un LOCK validé, aucun S3 mesuré. **Aucune version n'est certifiée production.**
