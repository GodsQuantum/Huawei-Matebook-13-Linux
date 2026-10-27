# GXFP51A0 — Hall of Fame matériel

Source canonique : [HARDWARE_TRIALS.csv](HARDWARE_TRIALS.csv). Les premières poses proviennent du retour humain, les mesures des journaux horodatés. Les captures biométriques et journaux bruts ne sont pas publiés. L'heure du LOCK du 23 octobre vers 13 h 17 est approximative et n'est pas utilisée pour un record de durée.

## Records valides (réussite dès la première pose)

| Discipline | Temps | Version | Essai |
|---|---:|---|---|
| LOCK : doigt → succès | **1.374 s** | rel71.69 | lock-20261027-7169-0204 |
| LOCK : KDE → succès | **3.488 s** | rel71.55 | lock-20261024-1 |
| S3 : préchauffage | **4.254 s** | rel71.55 | s3-20261023-1 |
| S3 : réveil → succès | **6.951 s** | rel71.55 | s3-20261023-1 |

## Fiabilité par version

| Version | Situation | 1ère pose | Certification de ce scénario |
|---|---|---:|---|
| rel71.55 | LOCK | 5/6 | ÉCHEC de première pose |
| rel71.55 | S3 | 2/3 | ÉCHEC de première pose |
| rel71.55 | REBOOT | 1/1 | Nombre insuffisant |
| rel71.56 | S3 | 0/1 | ÉCHEC de première pose |
| rel71.56 | COLD | 0/1 | ÉCHEC de première pose |
| rel71.58 | LOCK | 1/1 | Nombre insuffisant |
| rel71.58 | S3 | 0/2 | ÉCHEC de première pose |
| rel71.59 | LOCK | 1/1 | Nombre insuffisant |
| rel71.59 | S3 | 0/1 | ÉCHEC de première pose |
| rel71.60 | LOCK | 0/1 | ÉCHEC de première pose |
| rel71.61 | LOCK | 0/1 | ÉCHEC de première pose |
| rel71.62 | LOCK | 1/1 | Nombre insuffisant |
| rel71.62 | S3 | 0/1 | ÉCHEC de première pose |
| rel71.63 | LOCK | 1/1 | Nombre insuffisant |
| rel71.63 | S3 | 0/1 | ÉCHEC de première pose |
| rel71.64 | LOCK | 1/1 | Nombre insuffisant |
| rel71.64 | BOOT | 1/1 | Nombre insuffisant |
| rel71.65 | S3 | 0/1 | ÉCHEC de première pose |
| rel71.66 | LOCK | 1/1 | Nombre insuffisant |
| rel71.66 | S3 | 0/1 | ÉCHEC de première pose |
| rel71.67 | LOCK | 2/2 | Nombre insuffisant |
| rel71.68 | LOCK | 1/2 | ÉCHEC de première pose |
| rel71.68 | S3 | 0/1 | ÉCHEC de première pose |
| rel71.69 | LOCK | 1/1 | Nombre insuffisant |
| rel71.69 | S3 | 1/1 | Nombre insuffisant |
| rel71.69 | S4 | 1/2 | ÉCHEC de première pose |

## Essais horodatés

| Essai | Heure (Europe/Paris) | Version | Mode | Poses | Première | Préchauffage S3/S4 | FDT | Doigt→succès | KDE→succès | Réveil→succès | Score |
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
| s3-20261024-2015 | 2026-10-24T20:15:37.851+02:00 | rel71.58 | S3 | 4+ | fail | 7.489 | 0.082 | 16.392 | — | 34.206 | 21/7 |
| s3-20261024-2132 | 2026-10-24T21:32:18.990+02:00 | rel71.58 | S3 | 2 | fail | 4.959 | 0.083 | 4.621 | — | 10.145 | 14/7 |
| s3-20261024-2143 | 2026-10-24T21:43:20.848+02:00 | rel71.58 | S3 | unknown | unknown | 5.096 | 0.084 | 1.983 | — | 9.829 | 9/7 |
| s3-20261024-7159 | 2026-10-24T23:17:09.795+02:00 | rel71.59 | S3 | multiple | fail | 4.077 | 0.084 | — | — | — | 0/7 |
| lock-20261024-7159 | 2026-10-24T23:18:49.328+02:00 | rel71.59 | LOCK | 1 | pass | — | 0.083 | 1.917 | 4.742 | — | 14/7 |
| lock-20261024-7155-rollback2 | 2026-10-24T23:28:02.582+02:00 | rel71.55 | LOCK | 1 | pass | — | 0.302 | 1.931 | 4.297 | — | 16/7 |
| lock-20261025-7160-1 | 2026-10-25T00:07:00.252+02:00 | rel71.60 | LOCK | 2 | fail | — | 0.083 | 7.885 | — | — | 8/7 |
| lock-20261025-7161-1 | 2026-10-25T02:02:04.683+02:00 | rel71.61 | LOCK | 2 | fail | — | 0.082 | 7.453 | — | — | 7/7 |
| s3-20261025-7162-natural-2106 | 2026-10-25T21:06:08.872+01:00 | rel71.62 | S3 | multiple | fail | 37.625 | 0.083 | — | — | — | —/7 |
| lock-20261025-7162-after-s3-2107 | 2026-10-25T21:07:13.540+01:00 | rel71.62 | LOCK | 1 | pass | — | 0.084 | 1.956 | 4.843 | — | 9/7 |
| lock-20261025-7163-2230 | 2026-10-25T22:30:16.402+01:00 | rel71.63 | LOCK | 1 | pass | — | 0.082 | 1.869 | — | — | 15/7 |
| s3-20261025-7163-2255 | 2026-10-25T22:55:23.880+01:00 | rel71.63 | S3 | 2 | fail | 4.668 | 0.083 | 4.339 | — | 12.555 | 15/7 |
| boot-20261026-7164-0844 | 2026-10-26T08:44:48.208+01:00 | rel71.64 | BOOT | 1 | pass | — | 0.083 | 1.811 | 7.238 | — | 20/7 |
| lock-20261026-7164-0846 | 2026-10-26T08:46:03.649+01:00 | rel71.64 | LOCK | 1 | pass | — | 0.086 | 1.994 | 4.797 | — | 17/7 |
| s3-20261026-7164-1120 | 2026-10-26T11:20:17.890+01:00 | rel71.64 | S3 | unknown | unknown | 3.789 | 0.083 | 2.995 | — | 9.541 | 20/7 |
| lock-20261026-7165-1249 | 2026-10-26T12:49:26.854306+01:00 | rel71.65 | LOCK | unknown | unknown | — | 0.083 | 1.975 | — | — | 15/7 |
| s3-20261026-7165-natural-1609 | 2026-10-26T16:09:27.932011+01:00 | rel71.65 | S3 | 3 | fail | 13.422 | 0.083 | 3.511 | — | 18.082 | 16/7 |
| lock-20261026-7166-1701 | 2026-10-26T17:01:46.587289+01:00 | rel71.66 | LOCK | 1 | pass | — | 0.083 | 1.875 | 5.539 | — | 13/7 |
| s3-20261026-7166-natural-2225 | 2026-10-26T22:25:48.846794+01:00 | rel71.66 | S3 | multiple | fail | 5.844 | 0.083 | — | — | — | —/7 |
| lock-20261026-7167-2323 | 2026-10-26T23:22:51.043754+01:00 | rel71.67 | LOCK | 1 | pass | — | 0.291 | 1.898 | 5.523 | — | 7/7 |
| lock-20261026-7167-2332 | 2026-10-26T23:32:13.237443+01:00 | rel71.67 | LOCK | 1 | pass | — | 0.083 | 1.878 | 4.757 | — | 14/7 |
| s3-20261026-7167-natural-2339 | 2026-10-26T23:39:41.921373+01:00 | rel71.67 | S3 | ~2 uncertain | unknown | 9.665 | 0.082 | 1.993 | — | 13.129 | 13/7 |
| lock-20261027-7168-0019 | 2026-10-27T00:19:58.428781+01:00 | rel71.68 | LOCK | 2 | fail | — | 0.083 | 6.872 | — | — | 12/7 |
| lock-20261027-7168-0020 | 2026-10-27T00:20:11.882466+01:00 | rel71.68 | LOCK | 1 | pass | — | 0.082 | 1.860 | — | — | 7/7 |
| s3-20261027-7168-natural-0021 | 2026-10-27T00:21:36.860422+01:00 | rel71.68 | S3 | many | fail | — | — | — | — | — | —/7 |
| lock-20261027-7169-0204 | 2026-10-27T02:04:33.795536+01:00 | rel71.69 | LOCK | 1 | pass | — | 0.051 | 1.374 | — | — | 7/7 |
| lock-20261027-7169-s4resume-1457 | 2026-10-27T14:57:25.932538+01:00 | rel71.69 | S4 | 1 | pass | — | 0.085 | 1.826 | — | — | 11/7 |
| s3-20261027-7169-natural-1458 | 2026-10-27T14:58:45.886110+01:00 | rel71.69 | S3 | 1 | pass | 5.153 | 0.083 | 2.054 | — | 8.218 | 15/7 |
| s4-20261027-7169-1719-unknown | 2026-10-27T17:19:09.649000+01:00 | rel71.69 | S4 | unknown | unknown | — | 0.083 | — | — | — | 20/7 |
| s4-20261027-7169-1754-fail | 2026-10-27T17:54:03.206357+01:00 | rel71.69 | S4 | unknown | fail | 38.689 | 0.083 | — | — | — | —/7 |

## Registre antérieur au 13 octobre 2026

L'archive d'origine couvre **31 configurations**, **125 références Git**, et **14 classements historiques**.
Voir [le tableau historique](LEGACY_HALL_OF_FAME_20261013.md) et [le résumé JSON](LEGACY_HALL_OF_FAME_20261013.json). Les 31 configurations ne sont pas 31 essais indépendants.


## Comparaison anti-régression des LOCK

- rel71.55 après rollback : médiane **1.746 s** sur 4 essais (doigt → authentification).
- rel71.58 : **1.902 s** sur 1 essai(s). Écart **+8.9 %** ; pas de gain revendiqué et échantillon insuffisant.

## Gates obligatoires et politique d'enregistrement

1. Ne pas mélanger différentes versions ou installations. Un score réussi après plusieurs poses ne compte PAS comme première pose.
2. Pour une même version : 5 LOCK consécutifs première pose, 3 vrais S3 deep (dont une suspension pendant Verify), 3 cold boots, 3 reboots, doigt inconnu rejeté, mot de passe valide, aucune désynchronisation du capteur.
3. Ne pas relâcher le seuil de correspondance **7**, ne pas supprimer le nettoyage TLS, ne pas imposer de reboot/veille sans la participation de l'utilisateur.
4. Comparer uniquement des mesures de même nature. FDT FAST_READY ne mesure ni l'affichage du greeter ni le délai total de sortie de veille. Aucun S3 échoué ne peut détenir un record de vitesse.
5. À chaque essai : lire journal, calculer les timings, attester les poses avec l'utilisateur, ajouter une ligne au CSV, réviser la fiche de preuve, régénérer cette page avec l'option --write du script gxfp51a0-hall-of-fame.py, lancer --check et make verify, commit et push.

**Statut :** rel71.55 est le rollback de référence mais a échoué à la première pose sur 1 des 3 S3 mesurés. rel71.56 S3 rapide rejeté (27,725 s). rel71.57 instrumentation non installée. rel71.58 correction S3 en QA, un LOCK validé, 3 S3 observés dont 2 à poses multiples et un dernier à attribution humaine incertaine. **Aucune version n'est certifiée production.**
