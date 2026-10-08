# Licenses and third-party attribution

This repository contains different components, each of which retains its **own upstream licensing terms**:

| Component | License / attribution |
| --- | --- |
| MateBook onboarding scripts, GPU management and original integration/research material without a more specific license header | **GPL-2.0-only** — see [LICENSE](LICENSE) |
| libfprint-based GXFP51A0 driver, matching, protocol and library integration code | **LGPL-2.1-or-later** — see [driver COPYING](fingerprint/driver/goodix51a0/COPYING) and individual SPDX headers |
| Linux kernel-style research/test components with explicit SPDX | Follow their file-level SPDX notices, including GPL-2.0-only where declared |
| Imported/upstream patches and third-party components | Keep each original copyright, license, SPDX and attribution notice |

The root GPL-2.0-only license **does not relicense** LGPL components or any third-party code. The driver is derived from upstream libfprint work and may also include explicitly credited third-party contributions. Please preserve all relevant upstream notices when modifying or redistributing it.

No proprietary Huawei/Goodix Windows binaries, raw fingerprint templates, per-machine PMKs, firmware blobs or personal device identifiers are included in the release assets. See [security policy](SECURITY.md) for safe reports.

---

## Français

Le dépôt contient plusieurs composants **sous leurs licences d’origine** : scripts et intégrations propres au projet sous **GPL-2.0-only**, pilote dérivé de libfprint et composants associés sous **LGPL-2.1-or-later**, et éléments tiers conservant leurs mentions de copyright et de licence. La licence GPL du dépôt **ne remplace pas** les licences LGPL ou tierces.

## 简体中文

仓库包含采用**不同许可证**的组件：项目自身的安装脚本与集成代码采用 **GPL-2.0-only**，基于 libfprint 的指纹驱动及相关库代码采用 **LGPL-2.1-or-later**，第三方内容保留原版权及许可证声明。仓库根目录的 GPL 许可**不会覆盖**驱动的 LGPL 或其他第三方许可证。
