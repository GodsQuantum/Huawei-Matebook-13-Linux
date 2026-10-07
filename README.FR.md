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
| **Fingerprint** | **rel71.24 validée** | Pilote libfprint/fprintd natif du dépôt ; seuil 7 ; deep-S3 et recovery TLS validés |
| **Énergie MX250** | **GPU Manager v3.2** | Session Intel par défaut ; MX250 retirée du PCI au repos ; R580 + PRIME pour les applis dGPU |
| **Hotkeys / Fn-lock / batterie Huawei** | **Linux mainline** | Utiliser `huawei_wmi`, ne pas le dupliquer |
| **Intel GPU / Wi-Fi / Bluetooth / caméra / touch / stylet / audio** | **Natifs** | Vérifier uniquement |
| **Profils CPU/plateforme** | **Distribution** | Un seul fournisseur, pas d'empilement |
| **Firmware / BIOS** | **Distribution/fabricant** | Jamais flashé automatiquement |

## Fingerprint

Checkpoint actuel : **rel71.24**.

- FAST_READY warm ~**82–83 ms** sur la machine de référence ;
- recovery GPIO264 par pulse court actif-HIGH ;
- récupération d'un vrai deep lock TLS sans reboot/power-cycle ;
- deep-S3 manuel validé côté transport ;
- seuil fixe **7** ;
- aucune fusion de scores ;
- enrollments template-v4 conservés.

Documentation : [fingerprint/docs/validated-checkpoint-71.24.md](fingerprint/docs/validated-checkpoint-71.24.md)

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

## Licence

GPL-2.0-only à la racine ; les fichiers du pilote fingerprint conservent leurs licences SPDX.
