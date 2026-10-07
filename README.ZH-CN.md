# Huawei MateBook 13 Linux 支持

> 面向参考 Huawei MateBook 13 的“一条命令 + native first”Linux 硬件 onboarding。
>
> **English: [README.md](README.md)** · **Français: [README.FR.md](README.FR.md)**

## 一条命令

全新安装 Linux 或更换发行版后：

```bash
git clone https://github.com/GodsQuantum/huawei-matebook-13-linux.git
cd huawei-matebook-13-linux
./install.sh
```

请以普通桌面用户运行，不要直接使用 `sudo`。

仓库只补齐 Linux 尚未正确支持的硬件缺口；已经由 kernel / distro / desktop 正常支持的部分保持原生管理。

## 已验证参考硬件

Huawei MateBook 13 `WRTB-WXX9`：

- Intel Comet Lake-U UHD
- NVIDIA GeForce MX250 / GP108M `10de:1d13`
- Intel CNVi Wi-Fi + Bluetooth
- Intel HDA audio
- IMC Networks UVC camera `13d3:56c6`
- ELAN touchpad / touch / stylus
- 主线 `huawei_wmi`
- Goodix `GXFP51A0` / GF3658 ST411 指纹传感器

其它 Huawei 硬件版本不会被默认视为完全相同。

## 仓库管理范围

| 模块 | 状态 | 策略 |
| --- | --- | --- |
| **指纹** | **rel71.24 已验证** | 仓库原生 libfprint/fprintd；固定阈值 7；deep-S3 / TLS recovery 已验证 |
| **MX250 电源** | **GPU Manager v3.2** | Intel 默认；空闲时 MX250 从 PCI 移除；需要时使用 R580 + PRIME |
| **Huawei hotkeys / Fn-lock / 电池接口** | **Linux mainline** | 使用 `huawei_wmi`，不重复安装驱动 |
| **Intel GPU / Wi-Fi / Bluetooth / camera / touch / stylus / audio** | **原生支持** | 只验证 |
| **CPU/平台电源策略** | **发行版负责** | 只保留一个策略服务 |
| **Firmware / BIOS** | **发行版/厂商负责** | 绝不自动刷写 |

## 指纹

当前 checkpoint：**rel71.24**。

- 参考机 warm FAST_READY 约 **82–83 ms**
- GPIO264 active-HIGH 短脉冲 recovery
- 无需 reboot / power-cycle 即恢复真实 TLS deep lock
- 手动 deep-S3 transport 已验证
- 固定阈值 **7**
- 不融合分数
- template-v4 enrollment 保持兼容

技术记录：[fingerprint/docs/validated-checkpoint-71.24.md](fingerprint/docs/validated-checkpoint-71.24.md)

指纹安装器支持 **Arch/CachyOS、Debian/Ubuntu、Fedora/RHEL-family、openSUSE、Alpine**。

## 为什么 MX250 需要仓库管理

MX250 属于 **Pascal**。NVIDIA 文档中的 PCIe RTD3 需要 **Turing 或更新架构**，因此普通 PRIME 无法保证 MX250 真正断电。

GPU Manager v3.2 会：

1. 普通 DRI/Vulkan/GLX/EGL 应用默认使用 Intel；
2. 空闲时让 MX250 从 PCI 消失；
3. dGPU 应用启动时 rescan 并加载 NVIDIA R580；
4. 使用标准 PRIME Render Offload；
5. NVIDIA 仍被使用时拒绝关卡；
6. 应用结束后卸载 NVIDIA 并只移除 MX250。

已经声明 `PrefersNonDefaultGPU=true` 或 `X-KDE-RunOnDiscreteGpu=true` 的应用会自动导入。Steam 客户端保持 Intel，游戏单独管理。

## Huawei 原生功能

当前 kernel 的 `huawei_wmi` 已支持 hotkeys、Fn-lock、电池充电阈值和 mic-mute LED，因此仓库不会再安装一份重复的 Huawei platform driver，也不会偷偷设置电池阈值。

## Doctor

```bash
./matebook13-doctor.sh
# 或
./install.sh --doctor-only
```

会检查 Huawei WMI、R580/MX250、电源管理冲突、deep sleep、指纹、摄像头、输入设备、Wi-Fi/Bluetooth、audio 与 fwupd 可见性。

## 架构 / 审计

- [一条命令 onboarding](docs/ONE_COMMAND_ONBOARDING.md)
- [硬件支持审计矩阵](docs/HARDWARE_SUPPORT_MATRIX.md)
- [指纹](fingerprint/README.ZH-CN.md)
- [GPU / 电源](gpu-power/README.ZH-CN.md)

## 安全

默认流程不会自动刷写 firmware，不会静默修改电池阈值，不会为了指纹固定/降级 kernel，也不会自动叠加多个 CPU 电源管理器。

## 许可证

仓库根目录为 GPL-2.0-only；fingerprint 驱动文件保留各自 SPDX 许可证。
