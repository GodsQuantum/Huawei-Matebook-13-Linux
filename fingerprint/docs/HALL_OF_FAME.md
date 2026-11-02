# GXFP51A0 — Hall of Fame matériel

Source canonique : [HARDWARE_TRIALS.csv](HARDWARE_TRIALS.csv). Les premières poses proviennent du retour humain, les mesures des journaux horodatés. Les captures biométriques et journaux bruts ne sont pas publiés. L'heure du LOCK du 23 octobre vers 13 h 17 est approximative et n'est pas utilisée pour un record de durée.

## Records valides (réussite dès la première pose)

| Discipline | Temps | Version | Essai |
|---|---:|---|---|
| LOCK : doigt → succès | **1.157 s** | rel71.74-qa57+kde47 | lock-20261101-qa57-194519 |
| LOCK : KDE → succès | **3.488 s** | rel71.55 | lock-20261024-1 |
| S3 : préchauffage | **3.538 s** | rel71.74-qa57+kde47 | s3-20261101-qa57-214932 |
| S3 : réveil → succès | **5.918 s** | rel71.74-qa59+kde47 | s3-20261102-qa59-132231 |

## Champion de vitesse : référence pour les futures QA

- Record matériel **1.157 s** en première pose : **rel71.74-qa57+kde47** (lock-20261101-qa57-194519).
- Examiner d'abord le code du champion, jamais uniquement la dernière branche QA ; comparer aussi à la version en production.
- Ne pas confondre un gain de READY→DETECT avec une authentification plus rapide ou plus fiable. Les échecs du matcher disqualifient un candidat même si la détection est accélérée.
- Conserver les correctifs indépendants validés (dont KDE sans souris), TLS, FDT strict et seuil biométrique 7.
- Ce record est une observation matérielle unique : ne jamais le prendre pour une certification de fiabilité ou de sécurité biométrique.
- Promotion uniquement après comparaisons mesurées : première pose, READY→DETECT, DETECT→succès, capture, matching, nettoyage, KDE bout-en-bout et validation LOCK/S3/S4 avec doigt inconnu rejeté.
- Source du précédent champion QA42 : branche `qa/gxfp71.74-fdt-typed32-20261030`, commit moteur `b98b437` (incluant le correctif `dcdb043`), avec KDE #47 séparé. Les QA43, QA48 et QA50 ont régressé ; QA49 n'a pas amélioré la latence observée.
- Fiabilité de l’ancien champion QA42 : seulement **1/5 LOCK** à la première pose dans le registre. Ce chrono n'est **pas** une certification de production.

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
| rel71.70 | LOCK | 1/1 | Nombre insuffisant |
| rel71.70 | S4 | 0/1 | ÉCHEC de première pose |
| rel71.71 | LOCK | 1/1 | Nombre insuffisant |
| rel71.71 | S3 | 0/1 | ÉCHEC de première pose |
| rel71.71 | S4 | 1/1 | Nombre insuffisant |
| rel71.73 | S3 | 0/1 | ÉCHEC de première pose |
| rel71.73 | S4 | 0/1 | ÉCHEC de première pose |
| rel71.74-qa42 | LOCK | 1/5 | ÉCHEC de première pose |
| rel71.74-qa42 | S3 | 1/1 | Nombre insuffisant |
| rel71.74-qa42 | BOOT | 1/1 | Nombre insuffisant |
| rel71.74-qa42+kde47 | LOCK | 1/2 | ÉCHEC de première pose |
| rel71.74-qa49+kde47 | LOCK | 1/2 | ÉCHEC de première pose |
| rel71.74-qa49+kde47 | S3 | 0/1 | ÉCHEC de première pose |
| rel71.74-qa50+kde47 | LOCK | 0/1 | ÉCHEC de première pose |
| rel71.74-qa54+kde47 | LOCK | 1/1 | Nombre insuffisant |
| rel71.74-qa55+kde47 | LOCK | 0/1 | ÉCHEC de première pose |
| rel71.74-qa55+kde47 | S3 | 0/1 | ÉCHEC de première pose |
| rel71.74-qa56+kde47 | LOCK | 1/2 | ÉCHEC de première pose |
| rel71.74-qa56+kde47 | S3 | 0/1 | ÉCHEC de première pose |
| rel71.74-qa57+kde47 | LOCK | 1/1 | Nombre insuffisant |
| rel71.74-qa57+kde47 | S3 | 1/1 | Nombre insuffisant |
| rel71.74-qa57+fprintd-mr259 | S3 | 0/1 | ÉCHEC de première pose |
| rel71.74-qa59+kde47 | S3 | 1/2 | ÉCHEC de première pose |

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
| lock-20261027-7170-2034 | 2026-10-27T20:34:32.356875+01:00 | rel71.70 | LOCK | 1 | pass | — | 0.053 | 1.400 | — | — | 7/7 |
| s4-20261027-7170-2053 | 2026-10-27T20:53:04.622000+01:00 | rel71.70 | S4 | 2 (operator) | fail | 5.493 | 0.083 | 3.052 | — | — | 7/7 |
| lock-20261027-7171-2235 | 2026-10-27T22:35:30.395620+01:00 | rel71.71 | LOCK | 1 | pass | — | 0.052 | 1.254 | — | — | 15/7 |
| s3-20261027-7171-natural-2238 | 2026-10-27T22:38:19.843832+01:00 | rel71.71 | S3 | 2 | fail | 2.403 | 0.072 | 1.563 | — | 9.448 | 11/7 |
| s4-20261027-7171-2302 | 2026-10-27T23:02:21.003027+01:00 | rel71.71 | S4 | 1 | pass | 4.826 | 0.083 | 1.748 | — | 8.308 | 8/7 |
| s3-20261027-7173-2354-regression | 2026-10-27T23:53:59+01:00 | rel71.73 | S3 | 5+ (operator) | fail | 4.740 | 0.071 | — | — | — | 17/7 |
| s4-20261027-7173-2357-regression | 2026-10-27T23:57:04+01:00 | rel71.73 | S4 | 5-6 (operator) | fail | 18.379 | 0.083 | — | — | — | 12/7 |
| lock-20261030-qa42-0144 | 2026-10-30T01:44:28.208327+01:00 | rel71.74-qa42 | LOCK | 1 | pass | — | 0.052 | 1.181 | — | — | 13/7 |
| lock-20261030-qa42-1018-fail | 2026-10-30T10:18:03.371885+01:00 | rel71.74-qa42 | LOCK | multiple (operator) | fail | — | 0.051 | — | — | — | —/7 |
| lock-20261030-qa42-1026-fail | 2026-10-30T10:26:25.669543+01:00 | rel71.74-qa42 | LOCK | unknown (no success) | fail | — | 0.051 | — | — | — | —/7 |
| boot-20261031-qa42-2138 | 2026-10-31T21:38:22.075314+01:00 | rel71.74-qa42 | BOOT | 1 | pass | — | 0.084 | 1.227 | — | — | 12/7 |
| s3-20261031-qa42-2312 | 2026-10-31T23:12:13.855733+01:00 | rel71.74-qa42 | S3 | 1 | pass | 5.498 | 0.085 | 1.828 | — | 9.881 | 7/7 |
| lock-20261031-qa42-2317 | 2026-10-31T23:17:51.805924+01:00 | rel71.74-qa42 | LOCK | 4 (operator) | fail | — | 0.084 | 1.790 | — | — | 14/7 |
| lock-20261031-qa42-2343-hidden-ui-fail | 2026-10-31T23:43:39.438447+01:00 | rel71.74-qa42 | LOCK | multiple (operator; exact unknown) | fail | — | 0.083 | 1.758 | — | — | 16/7 |
| lock-20261101-qa42-kde47-hidden-ui-first-pose | 2026-11-01T00:00:14.998207+01:00 | rel71.74-qa42+kde47 | LOCK | 1 | pass | — | 0.083 | 1.800 | — | — | 14/7 |
| lock-20261101-qa42-kde47-two-poses-0148 | 2026-11-01T01:48:01.584950+01:00 | rel71.74-qa42+kde47 | LOCK | 2 | fail | — | 0.240 | 1.302 | — | — | 7/7 |
| lock-20261101-qa49-held-poll-0310 | 2026-11-01T03:10:00.468928+01:00 | rel71.74-qa49+kde47 | LOCK | 1 | pass | — | 0.052 | 1.328 | — | — | 7/7 |
| lock-20261101-qa50-failed-0357 | 2026-11-01T03:57:04.139771+01:00 | rel71.74-qa50+kde47 | LOCK | 5+ | fail | — | 0.052 | — | — | — | 4/7 |
| s3-20261101-qa49-overnight-0949 | 2026-11-01T09:49:28.625864+01:00 | rel71.74-qa49+kde47 | S3 | 5+ detected | fail | 3.597 | 0.074 | — | — | — | 3/7 |
| lock-20261101-qa49-1017 | 2026-11-01T10:17:35.768375+01:00 | rel71.74-qa49+kde47 | LOCK | 3+ detected | fail | — | 0.084 | — | — | — | 3/7 |
| lock-20261101-qa54-1212 | 2026-11-01T12:12:39.943902+01:00 | rel71.74-qa54+kde47 | LOCK | 1 | pass | — | 0.052 | 1.158 | — | — | 12/7 |
| s3-20261101-qa54-1213 | 2026-11-01T12:13:59.438957+01:00 | rel71.74-qa54+kde47 | S3 | 1-2 uncertain operator; 1 detected | unknown | 3.787 | 0.083 | 1.711 | — | 7.384 | 7/7 |
| lock-20261101-qa55-1557-regression | 2026-11-01T15:57:18.020806+01:00 | rel71.74-qa55+kde47 | LOCK | 5 (detected; success on 5th) | fail | — | 0.072 | 22.043 | — | — | 7/7 |
| s3-20261101-qa55-1558-fail | 2026-11-01T15:58:31.575078+01:00 | rel71.74-qa55+kde47 | S3 | 8+ detected, no success | fail | 3.555 | 0.083 | — | — | — | 3/7 |
| lock-20261101-qa56-1633 | 2026-11-01T16:33:19.842273+01:00 | rel71.74-qa56+kde47 | LOCK | 1 | pass | — | 0.051 | 1.193 | — | — | 7/7 |
| s3-20261101-qa56-1634 | 2026-11-01T16:34:11.890969+01:00 | rel71.74-qa56+kde47 | S3 | 2 | fail | 3.542 | 0.084 | 1.679 | — | 11.780 | 7/7 |
| lock-20261101-qa56-165011 | 2026-11-01T16:50:11.532834+01:00 | rel71.74-qa56+kde47 | LOCK | 2 detected; second success | fail | — | 0.084 | 1.719 | — | — | 16/7 |
| lock-20261101-qa56-165023 | 2026-11-01T16:50:23.993116+01:00 | rel71.74-qa56+kde47 | LOCK | 1 detected; physical unconfirmed | unknown | — | 0.083 | 1.696 | — | — | 10/7 |
| lock-20261101-qa56-165032 | 2026-11-01T16:50:32.084544+01:00 | rel71.74-qa56+kde47 | LOCK | 1 detected; physical unconfirmed | unknown | — | 0.084 | 1.713 | — | — | 13/7 |
| lock-20261101-qa56-165039 | 2026-11-01T16:50:39.274858+01:00 | rel71.74-qa56+kde47 | LOCK | 1 detected; physical unconfirmed | unknown | — | 0.084 | 1.725 | — | — | 7/7 |
| s3-20261101-qa56-165120 | 2026-11-01T16:51:20.881670+01:00 | rel71.74-qa56+kde47 | S3 | 1 detected; physical unconfirmed | unknown | 4.147 | 0.084 | 1.794 | — | 6.852 | 7/7 |
| s3-20261101-qa56-165147 | 2026-11-01T16:51:47.548953+01:00 | rel71.74-qa56+kde47 | S3 | 2 detected; physical unconfirmed | unknown | 4.155 | 0.083 | 1.732 | — | 11.317 | 7/7 |
| lock-20261101-qa57-194519 | 2026-11-01T19:45:19.316071+01:00 | rel71.74-qa57+kde47 | LOCK | 1 | pass | — | 0.060 | 1.157 | — | — | 9/7 |
| s3-20261101-qa57-195500 | 2026-11-01T19:55:00.285666+01:00 | rel71.74-qa57+kde47 | S3 | 2 detected; physical count unconfirmed | unknown | 3.781 | 0.084 | 1.227 | — | — | 7/7 |
| s3-20261101-qa57-214932 | 2026-11-01T21:49:32.205312+01:00 | rel71.74-qa57+kde47 | S3 | 1 | pass | 3.538 | 0.083 | 1.173 | — | 6.609 | 21/7 |
| s3-20261101-qa57-fprintd259-222809 | 2026-11-01T22:28:09.499958+01:00 | rel71.74-qa57+fprintd-mr259 | S3 | 5 | fail | 3.530 | 0.083 | 16.356 | — | 23.045 | 7/7 |
| s3-20261102-qa59-132144 | 2026-11-02T13:21:44.318871+01:00 | rel71.74-qa59+kde47 | S3 | 3 detected; user 3-4 | fail | 3.537 | 0.088 | 19.492 | — | 25.991 | 7/7 |
| s3-20261102-qa59-132231 | 2026-11-02T13:22:31.373465+01:00 | rel71.74-qa59+kde47 | S3 | 1 | pass | 3.544 | 0.083 | 1.248 | — | 5.918 | 7/7 |
| s3-20261102-qa59-132429 | 2026-11-02T13:24:29.030792+01:00 | rel71.74-qa59+kde47 | S3 | 2 user reported; 1 detected mixed deep+s2idle | unknown | 3.538 | 0.085 | 1.232 | — | 7.556 | 7/7 |

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
