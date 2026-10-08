[Reading 208 lines from start (total: 208 lines, 0 remaining)]

# Goodix GXFP51A0 / GF3658 ST411 sous Linux

Pilote libfprint natif expérimental pour le Goodix SPI GXFP51A0 présent dans la
famille Huawei MateBook 13 2021.

> English: [README.md](README.md) · 简体中文: [README.ZH-CN.md](README.ZH-CN.md)

## État actuel — 8 octobre 2026

### Préversion recommandée rel71.30

rel71.30 est désormais installée par défaut. Après réenrôlement dans KDE, les 8 captures de vérification relevées sur le MateBook 13 de référence ont toutes atteint le seuil 7. rel71.24 reste le dernier rollback validé en cold-boot et deep-S3 ; Un premier contrôle négatif a été effectué : majeur droit non enregistré refusé (scores 2/3/4), puis index droit accepté immédiatement (score 16, seuil 7). Cela ne constitue pas une mesure statistique du taux de fausses acceptations. Des essais sur d'autres machines restent nécessaires. rel71.18 reste le rollback historique.

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

Pour Plasma Login Manager 6.7.5, le dépôt contient aussi le paquet de
compatibilité validé qui sépare l'authentification fingerprint et mot de passe :
saisir le mot de passe n'attend plus l'expiration d'une tentative empreinte.
Les autres bureaux conservent leur intégration fprintd/PAM native.

### Validation matcher optionnelle et respectueuse des données biométriques

Benjamin Allègre (Sigfrodr) publie tools/eval/fp_eval.py dans Sigfrodr/libfprint-goodixtls : un évaluateur local commun à la famille Milan-SPI avec séparation enrol/probe disjointe. Il ne sort que des agrégats EER, FAR/FRR, distributions de scores et d-prime ; captures et templates restent sur la machine du testeur. C'est utile pour une validation multi-utilisateur défendable en upstream de SIGFM face à des références neutres descriptor/géométriques et NBIS optionnel. Ce n'est pas une dépendance runtime et les builds release restent incapables de dumper les captures biométriques.

## Installation par distribution

Les anciennes expériences (rel24–rel61) restent accessibles dans l’[historique technique](docs/history/), le [journal de recherche](docs/research-log.md) et Git. Elles ne constituent pas la procédure actuelle. rel71.30 est la préversion recommandée ; rel71.24 reste le dernier rollback entièrement testé après démarrage à froid et deep-S3.

### Arch / CachyOS

Depuis la racine du dépôt :

```bash
./fingerprint/install-arch.sh
```

L'installateur vérifie la présence du `GXFP51A0`, compile le patch libfprint, installe `libfprint-goodix51a0` et `fprintd`, ajoute uniquement l'accès gpiochip nécessaire et installe un prime boot one-shot. Il n'installe ni keepalive périodique ni hook externe de reprise S3. Sur Plasma 6.7.5 uniquement, il applique aussi les intégrations KDE/Plasma Login Manager package-managed, idempotentes et réversibles ; les autres bureaux gardent leur intégration fprintd/PAM native.

Ensuite utilise uniquement l'interface graphique native (KDE **Configuration du système → Utilisateurs**, GNOME **Paramètres → Utilisateurs**). Le programme d'installation n'exécute pas de commande d'enrôlement ou de vérification biométrique.

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
[research log](docs/research-log.md) et [handoff actuel](HANDOFF_CURRENT.md).

Le sous-arbre de production du pilote est `LGPL-2.1-or-later`.
