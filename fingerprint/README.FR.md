**Sécurité — 19 novembre 2026.** Après un blocage du déverrouillage par empreinte et par mot de passe au réveil d’une veille S3 profonde, le QML KDE v12 facultatif est en quarantaine et Pegasus utilise le QML officiel de Plasma 6.8.3. KDE mémorise `Regular` (mot de passe) pour éviter le plantage #528285. `Fingerprint=false` masque la méthode par empreinte dans le sélecteur. L’authentification par empreinte peut encore tourner en parallèle. L’utilisateur a confirmé un déverrouillage par mot de passe après un réveil S3 profond confirmé par le noyau. La répétabilité reste à vérifier. [Rapport d’incident](docs/real-hardware-certification.md).

**Corrections de fiabilité — 19 novembre 2026.** QA63.2 protège la calibration en cas de contact partiel et attend un retrait confirmé avant de poursuivre une tentative déjà scorée. Un [candidat KDE 6.8.3 natif distinct](integration/kscreenlocker-upstream-s3/README.md#plasma-683-native-readiness-candidate) conserve les demandes d’authentification arrivées trop tôt et peut reprendre la conversation d’empreinte implicite après expiration. L’installateur du pilote ne remplace pas KDE. De nouveaux essais physiques LOCK/S3 restent nécessaires ; les résultats historiques QA63 ne certifient pas cette révision.

**Installateurs actuels : QA63.2 (19 novembre 2026), paquet Arch QA63-3.**
**Essais historiques avec v12 (18 novembre).** Le correctif facultatif,
actif à l’époque, lançait l’authentification par empreinte sans déplacement
de la souris et masquait l’invite. Après réenregistrement de l’index droit,
plusieurs déverrouillages ont réussi. Une pose a obtenu un score inférieur
à 7. Après deux réveils S3 profonds confirmés par le noyau, le déverrouillage
par empreinte a réussi après repositionnement. La validation sur trois cycles,
la première pose après S3, la consommation en veille, le démarrage à froid
et S4 restent à vérifier.

Les essais QA63 antérieurs ont réussi à la première pose sur trois LOCK et deux S3 profonds réels. Les contrôles doigt inconnu avec ±18°, S4 répété, BOOT et REBOOT ne sont pas encore certifiés. Conserver le mot de passe. rel71.74 reste la release historique étiquetée.

**Version historique publiée sur la machine de référence : rel71.74 (28/10/2026).** LOCK à la première pose, S3 à la première pose rapporté par l'utilisateur. Le dernier S3/S4 a été annulé avant la veille à cause de KDE et du montage NFS Cloud9. Les hibernations répétées et les autres machines ne sont pas certifiées. Le pilote conserve la référence propre pour le doigt déjà posé, TLS-GCM, seuil 7 et mot de passe. [Preuves et limites](docs/rel71.74-production-20261028.md).

**Ancien avertissement biométrie (27/10/2026) :** rel71.69 est une **préversion expérimentale**. Après une hibernation S4 réelle, le capteur a mis 38,69 s à se préparer et le déverrouillage biométrique a échoué. Les réussites ponctuelles en LOCK/S3/S4 ne démontrent pas la fiabilité. Voir [incident confirmé #32](https://github.com/GodsQuantum/Huawei-Matebook-13-Linux/issues/32). Conserver le mot de passe.

# Goodix GXFP51A0 / GF3658 ST411 sous Linux

> **Note historique QA (rel71.55, 23 octobre 2026) :** les corrections natives S3 et la connexion biométrique parallèle de Plasma Login Manager 6.8.1 ont réussi leurs tests logiciels. **La veille S3, le redémarrage et le démarrage à froid ne sont PAS encore certifiés sur le matériel.** À cette époque, Pegasus testait rel71.55 sans certification matérielle ; la version installée aujourd’hui est QA63 ; le retour arrière vers rel71.49 et KDE 6.8.1 d'origine est conservé. [Critères de certification](docs/real-hardware-certification.md).

Pilote libfprint natif expérimental pour le Goodix SPI GXFP51A0 présent dans la
famille Huawei MateBook 13 2021.

> English: [README.md](README.md) · 简体中文: [README.ZH-CN.md](README.ZH-CN.md)

## Version expérimentale — fiabilité S4 non établie — rel71.69 (27 octobre 2026)

Pilote libfprint natif avec temporisation SPI CS de 50 µs vérifiée par traçage noyau. Trois succès matériels à la première pose sur le MateBook 13 (LOCK, hibernation S4, veille profonde S3). S3 : réveil→succès 8,218 s, doigt→succès 2,054 s, score 15/7. L'intégration KDE/Polkit utilise fprintd et préserve le mot de passe. Le seuil 7 et TLS-GCM sont intacts. Voir [preuve rel71.69](docs/rel71.69-hardware-final-20261027.md) et [Hall of Fame](docs/HALL_OF_FAME.md). Certification multi-appareils et démarrages froids non réalisée. Les notes rel71.30/71.24 ci-dessous sont historiques.

## État historique — 8 octobre 2026

### Préversion recommandée rel71.30

À l'époque, rel71.30 était installée par défaut (aujourd'hui remplacée par QA63.2). Après réenrôlement dans KDE, les 8 captures de vérification relevées sur le MateBook 13 de référence ont toutes atteint le seuil 7. rel71.24 reste le dernier rollback validé en cold-boot et deep-S3 ; Un premier contrôle négatif a été effectué : majeur droit non enregistré refusé (scores 2/3/4), puis index droit accepté immédiatement (score 16, seuil 7). Cela ne constitue pas une mesure statistique du taux de fausses acceptations. Des essais sur d'autres machines restent nécessaires. rel71.18 reste le rollback historique.

Le recovery décisif s'exécute avant l'énumération libfprint :

```text
restart fprintd
→ unbind uniquement spi-GXFP51A0:00 de spidev
→ pulse GPIO264 actif-HIGH ~10 ms
→ LOW + libération complète de la requête GPIO
→ settle détaché
→ rebind uniquement spi-GXFP51A0:00
→ udev settle
→ fprintd résident
```

Un capteur déjà bloqué par des erreurs TLS digest répétées a été récupéré sans reboot ni power-cycle. Les Claims warm restent à **82–83 ms FAST_READY** ; les locks KDE normaux et un cycle deep-S3 manuel ont été validés avec les enrollments existants.

La décision biométrique reste conservatrice : seuil fixe **7**, meilleure vue unique parmi les 20 vues d'enrollment, aucune fusion de scores, aucun retry de la même pose conditionné au score. Une image utilisable reçoit un seul score ; un score <7 impose de lever puis repositionner le doigt.

Voir [`docs/validated-checkpoint-71.24.md`](docs/validated-checkpoint-71.24.md) et [`docs/recovery-architecture-2026-10-06.md`](docs/recovery-architecture-2026-10-06.md).

## Installation

La réexécution est idempotente : une release déjà exacte ne rebuild/restart pas le capteur. Une vraie mise à jour doit terminer avec un nouveau `PREWARM_RESULT=READY`.

Depuis un checkout du dépôt :

```bash
./fingerprint/install.sh
```

L'installateur détecte Arch/CachyOS, Debian/Ubuntu, Fedora/RHEL, openSUSE et
Alpine. Arch/CachyOS délègue au paquet pacman natif. Avec systemd, le libfprint
local sous `/usr/local` n'est visible **que par fprintd** via un
`LD_LIBRARY_PATH` de service. Sans systemd, la même isolation passe par un
wrapper d'activation D-Bus prioritaire sous `/etc/dbus-1/system-services` :
aucun `ld.so.conf` global n'est modifié. Le `libdir` Meson réel est détecté
dynamiquement (multiarch Debian, `lib64`, `lib`) et l'ABI du fprintd de la
distribution est validée contre le candidat stagé avant toute modification
système.

Modes utiles :

```bash
./fingerprint/install.sh --build-only
./fingerprint/install.sh --no-install-deps
./fingerprint/install.sh --no-desktop-integration
```

Rollback :

```bash
sudo /var/lib/gxfp51a0-local-install/uninstall.sh
```

Arch/CachyOS peut appeler directement :

```bash
./fingerprint/install-arch.sh
```

L'installation ne supprime jamais les enrollments ni le cache PMK validé.
L'upgrade rel42 ne retire que les anciens entiers de timing non secrets.

Les paquets historiques Plasma 6.7.5 exigent l'option explicite
`--legacy-plasma-patches`. Le candidat KDE 6.8.3 natif est distinct et
n'est pas installé automatiquement avec le pilote. L'intégration fprintd/PAM
du bureau reste native et le QML v12 reste en quarantaine.

### Validation matcher optionnelle et respectueuse des données biométriques

Benjamin Allègre (Sigfrodr) publie tools/eval/fp_eval.py dans Sigfrodr/libfprint-goodixtls : un évaluateur local commun à la famille Milan-SPI avec séparation enrol/probe disjointe. Il ne sort que des agrégats EER, FAR/FRR, distributions de scores et d-prime ; captures et templates restent sur la machine du testeur. C'est utile pour une validation multi-utilisateur défendable en upstream de SIGFM face à des références neutres descriptor/géométriques et NBIS optionnel. Ce n'est pas une dépendance runtime et les builds release restent incapables de dumper les captures biométriques.

## Installation par distribution

Les anciennes expériences (rel24–rel61) restent accessibles dans l’[historique technique](docs/history/), le [journal de recherche](docs/research-log.md) et Git. Elles ne constituent pas la procédure actuelle. Les installateurs source sélectionnent QA63.2 ; rel71.30 et rel71.24 sont des références historiques de préversion et de retour arrière.

### Arch / CachyOS

Depuis la racine du dépôt :

```bash
./fingerprint/install-arch.sh
```

L'installateur vérifie la présence du `GXFP51A0`, compile le pilote QA63.2, installe `libfprint-goodix51a0` et utilise le service fprintd natif existant avec l'accès gpiochip nécessaire. Il n'ajoute aucun service de préchauffage, keepalive périodique ni hook de reprise S3. Les paquets et le QML du bureau restent inchangés par défaut ; les anciennes intégrations Plasma 6.7.5 exigent l'option explicite `--legacy-plasma-patches`.

Ensuite, l'interface native (KDE **Configuration du système → Utilisateurs**, GNOME **Paramètres → Utilisateurs**) reste le choix par défaut ; l'assistant guidé optionnel est une application **distincte**. L'installateur n'enrôle et ne vérifie jamais d'empreintes automatiquement.

Le pilote demande 20 poses. Déplace légèrement le doigt entre les poses afin de
couvrir plusieurs zones du doigt.

### Anciens templates de développement

Le format actuel est driver v4 / SIGFM v3. Si une empreinte enregistrée avec une ancienne version expérimentale n’est plus reconnue, supprimez **uniquement ce doigt** depuis les **paramètres graphiques natifs → Utilisateurs → Empreintes**, puis enregistrez-le de nouveau au même endroit. Ne supprimez pas toutes les empreintes et n’utilisez pas la ligne de commande pour les gérer. Cette étape n’est pas nécessaire pour une installation neuve.

### Debian / Ubuntu / Fedora / openSUSE / Alpine / autres Linux

L'installateur source portable reconstruit exactement le candidat libfprint
épinglé et garde le remplacement isolé sous `/usr/local` :

```bash
./fingerprint/install.sh
```

Il sait installer les dépendances sur Arch/CachyOS, Debian/Ubuntu, Fedora,
openSUSE et Alpine. Arch/CachyOS délègue au paquet pacman natif. Ailleurs, le
candidat est stagé, l'ABI du fprintd de la distribution est vérifiée, puis le
libfprint local est isolé à fprintd via un drop-in systemd ou un wrapper
d'activation D-Bus. Un manifeste de rollback est conservé.

Rollback :

```bash
sudo /var/lib/gxfp51a0-local-install/uninstall.sh
```

`./fingerprint/install.sh --build-only` permet de vérifier la compilation
sans rien installer.

## Matcher

Le chemin de production utilise :

- soustraction adaptative du fond ;
- normalisation percentile + unsharp ;
- FAST-9 multi-échelle à deux niveaux ;
- descripteurs BRIEF-256 non orientés ;
- cross-check mutuel + ratio test ;
- RANSAC rigide 200 itérations, tolérance 2 px ;
- raffinement rigide par moindres carrés ;
- meilleur score parmi 20 vues d'enrollment.

Le pilote ne baisse pas le seuil après un échec, n'additionne pas plusieurs
scores faibles et n'apprend pas depuis des vérifications échouées.

Le score pixel/ZNCC reste disponible uniquement avec le flag de recherche
explicite `GXFP_MATCH_DIAGNOSTICS`. Il ne participe pas à la décision
d'authentification.

## Validation contributeur

```bash
make -C fingerprint verify
```

La suite valide les tests déterministes/sécurité, le manifeste source, compile
libfprint `v1.94.100` et contrôle le binaire final.

Gates principales :

```text
SOURCE_MANIFEST=PASS
LIBFPRINT_BUILD=PASS
GOODIX51A0_OBJECT_COMPILED=YES
GOODIX51A0_FASTBRIEF_RANSAC_IN_LIBRARY=YES
GOODIX51A0_IDENTIFY_PATH_IN_LIBRARY=YES
RELEASE_BIOMETRIC_DUMP_HOOK=ABSENT
SOFTWARE_BUILD_READY=YES
ACTIVE_SENSOR_IO=NONE
GPIO_WRITES=NONE
MMIO_WRITES=NONE
FIRMWARE_ACTIONS=NONE
```

Les diagnostics locaux de maintenance sont optionnels ; un utilisateur normal
n'en a pas besoin.

## Sécurité et confidentialité

Ne jamais publier :

- captures ou templates d'empreinte ;
- PMK/PSK/clés ou fixtures propres à une unité ;
- binaires/firmwares Goodix ou Huawei propriétaires ;
- numéros de série ou identifiants privés.

Le cache PMK validé reste un état runtime protégé sous `/var/lib/fprint/`. rel43 ne persiste aucun timing adaptatif ; les anciens fichiers de timing non secrets sont supprimés à la migration.

Le template fprintd v4 local est une donnée biométrique et doit être protégé
comme tel.

Le pilote ne flashe pas le firmware du capteur.

## Périmètre

La cible prouvée est la combinaison exacte GXFP51A0 / GF3658 / ST411 ci-dessus.
Un autre appareil avec le même ACPI HID peut néanmoins avoir un câblage GPIO,
un firmware ou une intégration carte mère différents.

Ce logiciel biométrique reste reverse-engineered et expérimental. La validation
est aujourd'hui la plus forte sur l'unité de référence et sur des contrôles
négatifs inter-doigts du même utilisateur ; elle ne remplace pas une
certification biométrique sur un grand corpus inter-personnes. Ne considère pas
l'empreinte seule comme un facteur de sécurité haute assurance.

Voir [intégration desktop](docs/native-desktop-integration.md),
[provenance](PROVENANCE.md), [source](driver/goodix51a0/) et
[research log](docs/research-log.md) et [handoff actuel](docs/rel71.74-production-20261028.md).

Le sous-arbre de production du pilote est `LGPL-2.1-or-later`.

### Enrôlement guidé facultatif (toutes distributions Linux)

Le package intègre `gxfp51a0-enroll`, interface GTK3/PyGObject utilisant
le D-Bus standard de fprintd. Ouvrir l'application **Empreintes guidées GXFP51A0**
dans le menu KDE (ou lancer `gxfp51a0-enroll`). **Paramètres système KDE →
Utilisateurs → Empreintes** ouvre toujours l'interface KDE native,
**pas** cet assistant indépendant. Elle guide les 20 captures :
centre, haut/bas, gauche/droite, petites rotations et position naturelle.
La progression avance uniquement sur les captures acceptées par le pilote ;
les positions proposées ne sont pas mesurées par le capteur.

Aucune empreinte existante n'est supprimée. Pour refaire l'index droit,
vérifier d'abord le déverrouillage par mot de passe puis supprimer uniquement
cet index dans les paramètres système. Python3, GTK3 et PyGObject sont requis.
L'assistant ne propose que les doigts non enregistrés. Pour identifier un doigt,
le pilote compare **toutes les empreintes enregistrées**, pas seulement la
première, et conserve le meilleur score au-dessus du seuil fixe de 7.
Après inscription, vérifier le déverrouillage par plusieurs tentatives réelles.
Aucun service supplémentaire, correctif KDE ou affaiblissement du seuil biométrique.
