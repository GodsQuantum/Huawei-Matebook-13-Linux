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

在交互式终端中不带参数运行 `./install.sh` 时，脚本会先执行只读硬件审计，然后打开 **Whiptail** 菜单。界面会根据当前终端自动调整大小；按 `Esc`/Cancel 可直接退出且不做任何修改。**RECOMMENDED** 会检查原生平台 baseline，并只应用缺失/过期的仓库组件；**FINGERPRINT** 和 **GPU** 则严格只处理所选组件。

成功安装或更新后，会创建用户命令：

```bash
HUAWEI
```

`HUAWEI` 会打开同一个审计菜单；`HUAWEI --doctor-only` 等显式参数仍可用于脚本和自动化。

重复运行当前已精确安装的 fingerprint release 是安全的：安装器会保留当前 fprintd/TLS 会话，不会重新 build 或 restart 传感器。真正的驱动升级必须通过新的 `PREWARM_RESULT=READY` 语义 gate 后才报告成功。

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
| **指纹** | **rel71.30 推荐预览版 · rel71.24 S3 验证回退** | 仓库原生 libfprint/fprintd；阈值 7；rel71.30 增加连通式录入与保守的单视图 near-miss rescue |
| **MX250 电源** | **GPU Manager v3.2** | Intel 默认；空闲时 MX250 从 PCI 移除；需要时使用 R580 + PRIME |
| **Huawei hotkeys / Fn-lock / 电池接口** | **Linux mainline** | 使用 `huawei_wmi`，不重复安装驱动 |
| **Intel GPU / Wi-Fi / Bluetooth / camera / touch / stylus / audio** | **原生支持** | 只验证 |
| **CPU/平台电源策略** | **发行版负责** | 只保留一个策略服务 |
| **Firmware / BIOS** | **发行版/厂商负责** | 绝不自动刷写 |

## 指纹

**默认指纹安装版本：rel71.30**。在 MateBook 13 参考机通过 KDE 原生界面重新录入后，8/8 次校验捕获达到固定阈值 7。rel71.24 仍是最近通过冷启动/deep-S3 全面验证的回退版本；其他硬件与错误手指测试仍未完成。

- 参考机 warm FAST_READY 约 **82–83 ms**
- GPIO264 active-HIGH 短脉冲 recovery
- 无需 reboot / power-cycle 即恢复真实 TLS deep lock
- 手动 deep-S3 transport 已验证
- 固定阈值 **7**
- 不融合分数
- template-v4 enrollment 保持兼容

技术记录：[已验证 rel71.24](fingerprint/docs/validated-checkpoint-71.24.md) · [推荐预览版 rel71.30](fingerprint/docs/candidate-71.30.md)

在参考 MateBook 13 上又完成了负面测试：未录入手指的 2/3/4 分均被拒绝，已录入右手食指获得 16 分并成功解锁（阈值保持 7）。Ubuntu 可选择仅安装指纹驱动 `./fingerprint/install.sh`，或完整 HUAWEI/GPU 安装 `./install.sh`；必须匹配 `GXFP51A0` 与显卡 `10de:1d13`，Ubuntu 24.04/26.04 LTS 的源码编译 CI 全部通过（7/7 发行版矩阵），但 Ubuntu 真实指纹硬件、GPU 和 GUI/PAM 运行仍未验证。

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

## 命令快捷方式

同样的说明也可以在 `HUAWEI` Whiptail 菜单中的 **SHORTCUTS / HELP** 页面查看。

| 命令 | 作用 |
|---|---|
| `HUAWEI` | 打开先审计再操作的 MateBook 控制中心。 |
| `HUAWEI --doctor-only` | 仅执行只读硬件/软件审计。 |
| `HUAWEI --menu` | 强制打开交互式 Whiptail 菜单。 |
| `HUAWEI --no-fingerprint` | 执行 onboarding，但跳过指纹改动。 |
| `HUAWEI --no-gpu` | 执行 onboarding，但跳过 GPU Manager 改动。 |
| `HUAWEI --gpu-driver-preinstalled` | 要求发行版已提供 NVIDIA R580，脚本绝不安装驱动。 |
| `GPU-control` | 显示 GPU Manager 总览。 |
| `GPU-control menu` | 打开 GPU Manager 自己的交互式菜单。 |
| `GPU-control overview` | 显式显示同一总览/仪表盘。 |
| `GPU-control status` | 显示 MX250/NVIDIA/PCI/lease 的详细状态。 |
| `GPU-control list` | 列出当前由 NVIDIA 策略管理的应用。 |
| `GPU-control run -- COMMAND` | 仅本次启动使用 MX250，程序退出后自动释放。示例：`GPU-control run -- handy`。 |
| `GPU-control add App.desktop` | 让某个桌面应用今后正常启动时始终使用 MX250。示例：`GPU-control add Handy.desktop`。 |
| `GPU-control remove App.desktop` | 移除该永久 NVIDIA 包装，恢复默认/Intel 启动。 |
| `GPU-control add` | 交互式选择一个桌面应用进行管理。 |
| `GPU-control steam-add APPID` | 为一个 Steam 游戏启用按需 NVIDIA。 |
| `GPU-control steam-remove APPID` | 移除该 Steam 游戏的按需 NVIDIA。 |
| `GPU-control steam-all-on` | 为检测到的所有 Steam 游戏启用按需 NVIDIA。 |
| `GPU-control steam-all-off` | 关闭 Steam 全局策略。 |
| `GPU-control apply` | 重新应用已保存的 desktop/Steam 路由规则。 |
| `GPU-control install / repair / upgrade` | 安装或事务式修复/升级 GPU Manager。 |
| `GPU-control doctor` | 对 GPU Manager 做只读完整性审计。 |
| `GPU-control test` | 可逆 MX250 测试：唤醒 → 加载 NVIDIA → 测试 → 释放。 |
| `GPU-control uninstall` | 删除 GPU Manager 集成并恢复保存的 desktop 状态。 |
| `GPU-control --lang en|fr|zh COMMAND` | 选择 GPU Manager CLI/菜单语言。 |

GPU Manager 不会猜测哪些应用“看起来很吃 GPU”。安装/修复时，它只会自动导入明确通过 `PrefersNonDefaultGPU=true` 或 `X-KDE-RunOnDiscreteGpu=true` 请求独显的 `.desktop`。其他应用默认继续使用 Intel。偶尔需要 NVIDIA 时使用 `GPU-control run -- app`；如果希望某应用以后每次正常启动都使用 NVIDIA，则使用 `GPU-control add App.desktop`。

对于 Handy，建议让自启动继续使用 Intel；只有明确需要 MX250 时再运行 `GPU-control run -- handy`。

## 许可证

仓库根目录为 GPL-2.0-only；fingerprint 驱动文件保留各自 SPDX 许可证。
