# Huawei MateBook 13 sous Linux

> Onboarding Linux en une commande, avec priorité aux mécanismes natifs, pour le Huawei MateBook 13 de référence.
>
> **English: [README.md](README.md)** · **简体中文: [README.ZH-CN.md](README.ZH-CN.md)**

## Une seule commande

Après une réinstallation ou un changement de distribution :

```bash
git clone https://github.com/GodsQuantum/huawei-matebook-13-linux.git
cd huawei-matebook-13-linux
./install.sh
```

À lancer avec l'utilisateur normal du bureau, **pas avec sudo**.

Sans argument dans un terminal interactif, `./install.sh` commence par un audit matériel en lecture seule puis ouvre un menu **Whiptail**. L’interface s’adapte automatiquement à la taille du terminal ; `Esc`/Annuler quitte sans modification. **RECOMMENDED** vérifie le baseline natif et n’applique que les composants du dépôt manquants/obsolètes, tandis que **FINGERPRINT** et **GPU** restent strictement limités au composant choisi.

Après une installation/mise à jour réussie, le checkout installe le raccourci utilisateur :

```bash
HUAWEI
```

`HUAWEI` ouvre le même menu audité. Les flags explicites comme `HUAWEI --doctor-only` restent disponibles pour les scripts.

La réexécution de la release fingerprint exactement installée est sûre : l’installeur conserve la session fprintd/TLS active au lieu de rebuild/restart le capteur. Une vraie mise à jour du pilote doit obtenir un nouveau `PREWARM_RESULT=READY` avant que l'installation soit déclarée réussie.

Le dépôt ne corrige que les manques matériels que Linux ne gère pas correctement ; tout ce qui est déjà natif reste géré par le kernel, la distribution et le bureau.

## Matériel de référence validé

Huawei MateBook 13 `WRTB-WXX9` :

- Intel UHD Comet Lake-U
- NVIDIA GeForce MX250 / GP108M `10de:1d13`
- Wi-Fi + Bluetooth Intel CNVi
- audio Intel HDA
- caméra UVC IMC Networks `13d3:56c6`
- touchpad / touch / stylet ELAN
- `huawei_wmi` mainline
- capteur Goodix `GXFP51A0` / GF3658 ST411

Les autres révisions Huawei ne sont jamais supposées identiques.

## Ce que gère le dépôt

| Domaine | État | Politique |
| --- | --- | --- |
| **Fingerprint** | **rel71.24 validée · rel71.30 candidate** | libfprint/fprintd natif ; seuil 7 ; rel71.30 ajoute un enrollment connecté et un rescue conservateur par vue |
| **Énergie MX250** | **GPU Manager v3.2** | Session Intel par défaut ; MX250 retirée du PCI au repos ; R580 + PRIME pour les applis dGPU |
| **Hotkeys / Fn-lock / batterie Huawei** | **Linux mainline** | Utiliser `huawei_wmi`, ne pas le dupliquer |
| **Intel GPU / Wi-Fi / Bluetooth / caméra / touch / stylet / audio** | **Natifs** | Vérifier uniquement |
| **Profils CPU/plateforme** | **Distribution** | Un seul fournisseur, pas d'empilement |
| **Firmware / BIOS** | **Distribution/fabricant** | Jamais flashé automatiquement |

## Fingerprint

Checkpoint validé : **rel71.24**. Candidate de développement active : **rel71.30**.

- FAST_READY warm ~**82–83 ms** sur la machine de référence ;
- recovery GPIO264 par pulse court actif-HIGH ;
- récupération d'un vrai deep lock TLS sans reboot/power-cycle ;
- deep-S3 manuel validé côté transport ;
- seuil fixe **7** ;
- aucune fusion de scores ;
- enrollments template-v4 conservés.

Documentation : [rel71.24 validée](fingerprint/docs/validated-checkpoint-71.24.md) · [rel71.30 candidate](fingerprint/docs/candidate-71.30.md)

Installation seule :

```bash
./fingerprint/install.sh
```

Distributions prises en charge par l'installateur fingerprint : **Arch/CachyOS, Debian/Ubuntu, Fedora/RHEL-family, openSUSE et Alpine**.

## Pourquoi la MX250 nécessite un gestionnaire

La MX250 est **Pascal**. Le RTD3 PCIe documenté par NVIDIA exige **Turing ou plus récent** ; PRIME seul ne garantit donc pas une vraie extinction de cette carte.

GPU Manager v3.2 :

1. garde les applications DRI/Vulkan/GLX/EGL ordinaires sur Intel ;
2. retire la MX250 du PCI lorsqu'elle est inactive ;
3. la rescane et charge NVIDIA R580 pour une application dGPU ;
4. utilise PRIME Render Offload ;
5. refuse toute extinction tant qu'un processus utilise NVIDIA ;
6. décharge NVIDIA puis retire uniquement la MX250.

Les applications déclarant déjà `PrefersNonDefaultGPU=true` ou `X-KDE-RunOnDiscreteGpu=true` sont importées automatiquement. Le client Steam reste sur Intel ; les jeux se gèrent séparément.

Pour une application dont les métadonnées sont incorrectes :

```bash
GPU-control add
GPU-control list
GPU-control doctor
```

## Fonctions Huawei natives

Le `huawei_wmi` actuel du kernel couvre déjà hotkeys, Fn-lock, contrôle de charge batterie et LED mic-mute. Le dépôt ne fournit donc aucun driver Huawei redondant et ne choisit jamais silencieusement les seuils de charge.

## Doctor matériel

```bash
./matebook13-doctor.sh
# ou
./install.sh --doctor-only
```

Il vérifie modèle, Huawei WMI, R580/MX250, conflits de gestionnaires d'énergie, deep sleep, fingerprint, caméra, entrées, Wi-Fi/Bluetooth, audio et visibilité de fwupd.

## Architecture / audit

- [Onboarding en une commande](docs/ONE_COMMAND_ONBOARDING.md)
- [Matrice de support auditée](docs/HARDWARE_SUPPORT_MATRIX.md)
- [Fingerprint](fingerprint/README.FR.md)
- [GPU / alimentation](gpu-power/README.FR.md)

## Sécurité

Aucun flash automatique, aucun seuil batterie imposé, aucun kernel pin/downgrade pour le fingerprint, aucun empilement automatique de TLP/tuned/auto-cpufreq/power-profiles-daemon.

## Raccourcis de commande

Les mêmes commandes sont accessibles directement dans le menu Whiptail `HUAWEI`, rubrique **SHORTCUTS / AIDE**.

| Commande | Utilité |
|---|---|
| `HUAWEI` | Ouvre le centre de contrôle MateBook avec audit préalable. |
| `HUAWEI --doctor-only` | Lance uniquement l’audit matériel/logiciel en lecture seule. |
| `HUAWEI --menu` | Force l’ouverture du menu Whiptail interactif. |
| `HUAWEI --no-fingerprint` | Lance l’onboarding sans modifier le fingerprint. |
| `HUAWEI --no-gpu` | Lance l’onboarding sans modifier GPU Manager. |
| `HUAWEI --gpu-driver-preinstalled` | Exige un pilote NVIDIA R580 fourni par la distro et n’en installe jamais. |
| `GPU-control` | Affiche la vue d’ensemble du GPU Manager. |
| `GPU-control menu` | Ouvre le menu interactif propre à GPU Manager. |
| `GPU-control overview` | Alias explicite de la vue d’ensemble. |
| `GPU-control status` | Affiche l’état détaillé MX250/NVIDIA/PCI/leases. |
| `GPU-control list` | Liste les applications actuellement gérées pour NVIDIA. |
| `GPU-control run -- COMMANDE` | Utilise la MX250 pour ce lancement uniquement, puis la libère. Exemple : `GPU-control run -- handy`. |
| `GPU-control add App.desktop` | Force une application graphique donnée à utiliser la MX250 à chaque lancement normal. Exemple : `GPU-control add Handy.desktop`. |
| `GPU-control remove App.desktop` | Retire ce wrapper NVIDIA permanent et restaure le lancement normal/Intel. |
| `GPU-control add` | Permet de choisir interactivement une application graphique. |
| `GPU-control steam-add APPID` | Active NVIDIA à la demande pour un jeu Steam. |
| `GPU-control steam-remove APPID` | Retire NVIDIA à la demande pour ce jeu Steam. |
| `GPU-control steam-all-on` | Active NVIDIA à la demande pour tous les jeux Steam détectés. |
| `GPU-control steam-all-off` | Désactive cette politique globale Steam. |
| `GPU-control apply` | Réapplique toutes les règles desktop/Steam sauvegardées. |
| `GPU-control install / repair / upgrade` | Installe ou répare/met à jour transactionnellement GPU Manager. |
| `GPU-control doctor` | Audit en lecture seule de l’intégrité/configuration GPU Manager. |
| `GPU-control test` | Smoke-test réversible : réveil MX250 → NVIDIA → test → extinction. |
| `GPU-control uninstall` | Désinstalle l’intégration GPU Manager et restaure les états desktop sauvegardés. |
| `GPU-control --lang en|fr|zh COMMANDE` | Choisit la langue CLI/menu de GPU Manager. |

GPU Manager ne cherche volontairement pas à « deviner » quelles applications seraient gourmandes. À l’installation/réparation, il peut importer les fichiers `.desktop` qui demandent explicitement le GPU discret avec `PrefersNonDefaultGPU=true` ou `X-KDE-RunOnDiscreteGpu=true`. Les autres applications restent sur Intel par défaut. Utilisez `GPU-control run -- app` pour un usage NVIDIA ponctuel, ou `GPU-control add App.desktop` pour rendre ce choix permanent pour cette application.

Pour Handy, la configuration recommandée est de laisser l’autostart sur Intel et d’utiliser `GPU-control run -- handy` uniquement lorsque la MX250 est réellement souhaitée.

## Licence

GPL-2.0-only à la racine ; les fichiers du pilote fingerprint conservent leurs licences SPDX.
