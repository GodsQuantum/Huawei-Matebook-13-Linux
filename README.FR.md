# Huawei MateBook 13 sous Linux

> Onboarding Linux en une commande, avec priorité aux mécanismes natifs, pour la famille Huawei MateBook 13.
>
> **English: [README.md](README.md)** · **简体中文: [README.ZH-CN.md](README.ZH-CN.md)**

## Objectif

Un MateBook 13 pris en charge doit pouvoir repartir d'une installation Linux neuve ou changer de distribution et retrouver l'état matériel validé avec une seule commande :

```bash
git clone https://github.com/GodsQuantum/huawei-matebook-13-linux.git
cd huawei-matebook-13-linux
./install.sh
```

Règle du projet : **native first**. Si Linux gère déjà correctement un composant, le dépôt le vérifie et laisse la distribution, le kernel et le bureau l'administrer. Le dépôt n'installe du code que pour les vrais manques matériels spécifiques.

Voir [`docs/ONE_COMMAND_ONBOARDING.md`](docs/ONE_COMMAND_ONBOARDING.md).

## Matériel de référence

Profil validé : Huawei `WRTB-WXX9` / MateBook 13 avec :

- Intel UHD Comet Lake-U ;
- NVIDIA GeForce MX250 / GP108M (`10de:1d13`) ;
- Wi-Fi + Bluetooth Intel CNVi ;
- audio Intel HDA ;
- caméra UVC IMC Networks (`13d3:56c6`) ;
- touchpad / touch / stylet ELAN ;
- `huawei_wmi` mainline ;
- lecteur d'empreintes Goodix `GXFP51A0` / GF3658 ST411.

Les autres révisions MateBook 13 sont détectées par identifiants matériels et ne sont jamais supposées identiques.

## État

| Domaine | État | Politique du projet |
| --- | --- | --- |
| **Empreinte — GXFP51A0 / GF3658** | **Checkpoint 71.24 validé** | Pilote libfprint/fprintd natif géré par le repo ; seuil fixe 7 ; recovery spidev ciblé avant énumération ; 3 locks KDE consécutifs réussis à la 1re pose |
| **GPU — NVIDIA MX250** | **PRIME standard fonctionnel** | Branche legacy NVIDIA R580 de la distro + PRIME Render Offload ; ancien gestionnaire PCI-remove non installé par défaut |
| **Touches Huawei / Fn lock / seuils batterie** | **Kernel mainline** | Utiliser `huawei_wmi`, ne pas dupliquer le pilote |
| **Caméra / Wi-Fi / Bluetooth / touchpad / stylet / audio** | **Natifs sur la machine de référence** | Valider uniquement |
| **Profils d'énergie / suspend** | **Gérés par la distribution** | Ne pas empiler plusieurs gestionnaires d'énergie |

## Checkpoint fingerprint 71.24

71.24 est le checkpoint de production validé actuel. 71.18 reste le rollback immuable de référence.

Recovery :

```text
démarrage/restart fprintd
→ unbind uniquement spi-GXFP51A0:00 de spidev
→ reset GPIO264 actif-HIGH
→ settle
→ rebind uniquement spi-GXFP51A0:00
→ udev settle
→ libfprint/fprintd
```

Le recovery GPIO264 par pulse court a récupéré sans reboot ni power-cycle un capteur déjà bloqué par des erreurs TLS digest répétées. Les Claims warm restent à **82–83 ms FAST_READY** ; les locks KDE normaux et un cycle deep-S3 manuel ont été validés. Le seuil reste **7**, sans fusion de scores.

Documentation : [`fingerprint/docs/validated-checkpoint-71.24.md`](fingerprint/docs/validated-checkpoint-71.24.md).

Installation fingerprint seule :

```bash
./fingerprint/install.sh
```

## Politique GPU

Le MX250 de référence utilise le chemin PRIME standard de la distribution :

```text
bureau → Intel UHD
charge GPU → prime-run → NVIDIA MX250
repos → gestion runtime NVIDIA/PCIe standard
```

L'ancien gestionnaire [`gpu-power/`](gpu-power/) qui retirait le GPU du PCI est conservé pour l'historique/recherche uniquement.

## Doctor matériel

Validation en lecture seule :

```bash
./matebook13-doctor.sh
# ou
./install.sh --doctor-only
```

Le doctor vérifie Huawei WMI, PRIME/NVIDIA, GXFP51A0, caméra, hotkeys, touchpad, Wi-Fi, Bluetooth, vidéo et audio sans choisir de politique utilisateur.

## Sécurité et vie privée

L'onboarding par défaut ne flashe aucun firmware, ne change pas silencieusement les seuils de charge, ne pin/downgrade pas le kernel pour le fingerprint et n'installe pas plusieurs gestionnaires d'énergie concurrents.

Ne publiez pas de nom d'utilisateur, chemin personnel, hostname, numéro de série, UUID, credential, capture/template d'empreinte, PMK/PSK, firmware propriétaire ou binaire Windows.

## Contribuer

Voir [CONTRIBUTING.md](CONTRIBUTING.md) et [SECURITY.md](SECURITY.md).

## Licence

GPL-2.0-only à la racine ; les fichiers du pilote fingerprint conservent leurs licences SPDX.
