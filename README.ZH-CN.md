# Huawei MateBook 13 Linux 支持

> 面向 Huawei MateBook 13 系列的“一条命令 + native first”Linux 硬件 onboarding。
>
> **English: [README.md](README.md)** · **Français: [README.FR.md](README.FR.md)**

## 目标

受支持的 MateBook 13 在全新安装 Linux 或更换发行版后，应能用一条命令恢复到已验证的硬件状态：

```bash
git clone https://github.com/GodsQuantum/huawei-matebook-13-linux.git
cd huawei-matebook-13-linux
./install.sh
```

项目原则：**native first**。Linux 已经正确支持的组件只做检测和验证；只有真正缺失的硬件支持才由本仓库补充。

详见 [`docs/ONE_COMMAND_ONBOARDING.md`](docs/ONE_COMMAND_ONBOARDING.md)。

## 参考硬件

当前已验证的参考机型为 Huawei `WRTB-WXX9` / MateBook 13：

- Intel Comet Lake-U UHD；
- NVIDIA GeForce MX250 / GP108M (`10de:1d13`)；
- Intel CNVi Wi-Fi + Bluetooth；
- Intel HDA audio；
- IMC Networks UVC camera (`13d3:56c6`)；
- ELAN touchpad / touch / stylus；
- 主线内核 `huawei_wmi`；
- Goodix `GXFP51A0` / GF3658 ST411 指纹传感器。

其它 MateBook 13 硬件版本按设备 ID 检测，不默认视为完全相同。

## 状态

| 模块 | 状态 | 项目策略 |
| --- | --- | --- |
| **指纹 — GXFP51A0 / GF3658** | **已验证 71.18 checkpoint** | 本仓库维护原生 libfprint/fprintd 驱动；固定阈值 7；枚举前目标 spidev recovery；连续 3 次 KDE 锁屏首按成功 |
| **GPU — NVIDIA MX250** | **标准 PRIME 正常工作** | 使用发行版 NVIDIA R580 legacy + PRIME Render Offload；默认不安装旧 PCI-remove 管理器 |
| **Huawei hotkeys / Fn lock / 电池阈值** | **主线内核支持** | 使用 `huawei_wmi`，不重复安装驱动 |
| **摄像头 / Wi-Fi / Bluetooth / touchpad / stylus / audio** | **参考机原生支持** | 只验证 |
| **电源配置 / suspend** | **发行版管理** | 不叠加多个 power manager |

## 指纹 checkpoint 71.18

71.18 在进一步优化前冻结为已知良好版本。

核心 recovery：

```text
fprintd 启动/重启
→ 只 unbind spi-GXFP51A0:00
→ GPIO264 active-HIGH reset
→ settle
→ 只 rebind spi-GXFP51A0:00
→ udev settle
→ libfprint/fprintd
```

一个故意保留在 TLS degraded 状态的传感器无需 reboot 或 power-cycle 即恢复。warm Claim 回到约 **82–83 ms FAST_READY**，之后连续三次 KDE 锁屏都在第一次物理按压成功，baseline 分数为 **8/7、11/7、12/7**。

技术记录：[`fingerprint/docs/validated-checkpoint-71.18.md`](fingerprint/docs/validated-checkpoint-71.18.md)。

仅安装 fingerprint：

```bash
./fingerprint/install.sh
```

## GPU 策略

参考 MX250 使用发行版标准 PRIME：桌面运行在 Intel UHD，需要独显的程序通过 `prime-run` 使用 MX250，空闲状态交给 NVIDIA/PCIe runtime PM。

历史 [`gpu-power/`](gpu-power/) PCI-remove 管理器只保留用于研究，不进入默认 onboarding。

## 硬件 doctor

```bash
./matebook13-doctor.sh
# 或
./install.sh --doctor-only
```

doctor 只读检查 Huawei WMI、PRIME/NVIDIA、GXFP51A0、摄像头、hotkeys、touchpad、Wi-Fi、Bluetooth、video 和 audio。

## 安全与隐私

默认 onboarding 不刷写 firmware，不静默修改电池充电阈值，不为了 fingerprint 固定/降级 kernel，也不安装多个互相竞争的电源管理器。

不要公开用户名、home 路径、hostname、序列号、UUID、凭据、指纹图像/模板、PMK/PSK、专有 firmware 或 Windows 二进制。

## 许可证

仓库根目录为 GPL-2.0-only；fingerprint 驱动文件保留各自 SPDX 许可证。
