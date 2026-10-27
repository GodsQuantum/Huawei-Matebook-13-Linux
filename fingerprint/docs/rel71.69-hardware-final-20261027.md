**REVISION 2026-10-27: REL71.69 IS EXPERIMENTAL / NOT S4-STABLE.** The documented successes below were genuine but did not predict the later 17:54 S4 resume failure with 38.69s calibration and zero images. [Confirmed evidence](rel71.69-s4-hibernate-failure-20261027.md), [issue #32](https://github.com/GodsQuantum/Huawei-Matebook-13-Linux/issues/32).

# rel71.69 — matériel réel, référence recommandée pour GXFP51A0/ST411 (27 octobre 2026)

**Décision utilisateur:** « LOCK 1 pose, bien ; retour après hibernation / batterie et empreinte première pose rapide ; puis veille S3 : première pose longue ; je pense qu'on entérine cette version ». Trois authentifications utilisateur sur capteur réel après installation QA rel71.69 avec CS→SCLK 50us et FDT proof/FRESH BG conservés.

| Type | Attestation physique | Score/7 | FDT FAST_READY | Détecté → succès | Réveil S3 → succès |
|---|---|---:|---:|---:|---:|
| LOCK 27/10 02:04 CET | Première pose | 7 | 51 ms | 1.374 s | — |
| Après hibernation S4 27/10 14:57 CET | Première pose | 11 | 85 ms | 1.826 s | — |
| S3 réel 27/10 14:58 CET | Première pose, ressentie longue, contact initial possiblement partiel | 15 | 83 ms | 2.054 s | 8.218 s |

**S4**: noyau journalise PM: hibernation entry 14:38:43 et ACPI S4 resume 14:47:36; il n'est pas exact d'appeler ce cycle S3 ou boot à froid. Des tentatives de vérification ont été préparées par KDE sans doigt pendant plusieurs minutes après S4 ; cela ne démontre pas une sortie S4 complète « rapide » mesurée de bout en bout, seulement première pose acceptée et délai doigt→succès. 

**S3**: noyau PM suspend entry (deep) 14:58:41, vrai wake 14:58:45.886; premier reset court natif GPIO264, pas de long-first, PREWARM_READY à 14:58:51.039 (5.153s), fingerprint FAST_READY à 14:58:51.819 (83ms), DETECTED_HOLD à 14:58:52.050, unique image biométrique scorée 15/7 à 14:58:53.169, terminal positif à 14:58:54.104 (8.218s depuis wake, 2.054s depuis DETECTED). Pas de TLS digest/GET_IMAGE timeout. Première pose utilisateur, validée strictement ; ne pas prétendre 3/3 S3 ni certification de BOOT froid.

**HOF:** rel71.55 reste le record brut S3 one-pose à 6.951s mais avait 1/3 S3 non first-touch. rel71.69 est désormais le meilleur compromis observé sur cette unité : 3 expériences distinctes first-touch, vrai S3 et S4, TLS et capture authentifiés, score15 sur S3. Le gain vient de l'installation CSI SPI 50us (ftrace pxa2xx CS→start 6us contrôle contre 76us avec opt-in; SDK fprintd init 7.971s→6.184s sur un seul A/B), pas de resets GPIO longs ni faiblesse matcher. Un échantillon de trois événements ne suffit pas pour un label multi-machines ou un chiffre de faux accept.

## Installation finale / portée des applications
Distribution native libfprint v1.94.100-goodix51a0-rel71.69 et fprintd ; d'autres applications accèdent à l'empreinte quand elles utilisent le framework natif fprintd/PAM (KDE lock/login, Polkit où configuré). L'installateur n'installe pas de nouveau démon ; les autres applications (navigateur web, portails propres à une app, gestionnaires de mots de passe ne reposant pas sur PAM) ne deviennent PAS magiquement compatibles. Préserver mot de passe et refus de mauvais doigt, ne pas modifier le PAM global ou le sudo à l'aveugle.

## Rollbacks & invariants
Pegasus a des paquets rel71.67 (SHA256 08681c32c1234b0fae0f6f61017d50df18f75bc1528a62fdb3d864b300e4d09c), rel71.64 (c3df31e8dd06bfc778c661a023333a632f2824b4e841bbdf524ea1c4aa46e932) ; tester l'empreinte enregistrée sans réenroler, mot de passe conservé. Certifier les vrais changements package/installer séparément du cœur immuable. Les tests de certification supplémentaires (plusieurs S3, BOOT froid, reboot, doigts non enrôlés) restent recommandés sans être déclenchés à distance.
