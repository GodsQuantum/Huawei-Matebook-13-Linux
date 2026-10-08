[Reading 167 lines from start (total: 167 lines, 0 remaining)]

# Goodix GXFP51A0 / GF3658 ST411 Linux 驱动

面向 Huawei MateBook 13 2021 系列中 SPI Goodix GXFP51A0 的实验性原生 libfprint 驱动。

> **English: [README.md](README.md)** · **Français : [README.FR.md](README.FR.md)**

## 当前状态 — 2026-10-08

### 推荐预览版 rel71.30；已完整验证的回退版本 rel71.24

默认安装 rel71.30。在 MateBook 13 参考机通过 KDE 图形界面完成指纹录入后，8/8 次正确指纹验证达到阈值 7。之后的人工负面测试中，未注册手指连续获得 2/3/4 分并被拒绝，已注册右手食指获得 16 分并成功解锁。此结果并非对所有设备的安全性统计证明。独立机器、Ubuntu/GNOME、冷启动和 deep-S3 测试尚未完成。rel71.24 是最近完整验证过的 cold-boot/deep-S3 回退版本；rel71.18 保留为更早的不可变回退基线。

关键 recovery 在 libfprint 枚举之前执行：

```text
restart fprintd
→ 只 unbind spi-GXFP51A0:00
→ GPIO264 active-HIGH ~10 ms 短脉冲
→ LOW + 完全 release GPIO request
→ detached settle
→ 只 rebind spi-GXFP51A0:00
→ udev settle
→ resident fprintd
```

已经连续出现 TLS digest failure 的传感器无需 reboot / power-cycle 即恢复。warm Claim 仍保持 **82–83 ms FAST_READY**；普通 KDE 锁屏和一次手动 deep-S3 suspend/resume 均已验证。

认证策略保持固定阈值 **7**、20-view enrollment、template v4；不融合分数，也不根据低分在同一次按压内重试。一次可用物理按压只评分一次，低于 7 必须抬起后重新放置。

详见 [`docs/validated-checkpoint-71.24.md`](docs/validated-checkpoint-71.24.md) 与 [`docs/recovery-architecture-2026-10-06.md`](docs/recovery-architecture-2026-10-06.md)。

## 安装

重复运行是幂等的：已精确安装的 release 不会重新 build/restart 传感器；真正的驱动更新必须得到新的 `PREWARM_RESULT=READY`。

推荐从源码 checkout 执行：

```bash
./fingerprint/install.sh
```

安装器自动识别 Arch/CachyOS、Debian/Ubuntu、Fedora/RHEL-family、
openSUSE 与 Alpine。Arch/CachyOS 使用原生 pacman 包；systemd 系统通过
service-local `LD_LIBRARY_PATH` **只让 fprintd 使用** `/usr/local`
libfprint。非 systemd 系统通过 `/etc/dbus-1/system-services` 中更高优先级
的 D-Bus activation wrapper 实现同样隔离，不修改全局 `ld.so.conf`。
Meson `libdir` 会动态检测（Debian multiarch、`lib64`、普通 `lib`），且在
修改系统文件之前会用发行版自己的 fprintd 验证 staged candidate 的 ABI。

常用模式：

```bash
./fingerprint/install.sh --build-only
./fingerprint/install.sh --no-install-deps
./fingerprint/install.sh --no-desktop-integration
```

回滚：

```bash
sudo /var/lib/gxfp51a0-local-install/uninstall.sh
```

Arch/CachyOS 也可以直接执行：

```bash
./fingerprint/install-arch.sh
```

安装器不会删除 enrollment，也不会删除已经验证的 PMK cache。rel42 升级
只清理 rel24–rel40 遗留的非敏感 timing 整数。

对于 Plasma Login Manager 6.7.5，本仓库还提供已验证的密码/指纹分离认证
兼容包：输入密码不会再等待指纹 timeout。其他桌面继续使用各自原生
fprintd/PAM 集成。

### 可选的隐私保护 matcher 验证

Benjamin Allègre（Sigfrodr）在 Sigfrodr/libfprint-goodixtls 中发布了 tools/eval/fp_eval.py，作为 Milan-SPI 系列的本地统一评估工具。它使用互不重叠的 enrol/probe 划分，只输出 EER、FAR/FRR、分数分布和 d-prime 等聚合统计；原始指纹图像和模板始终留在测试者自己的机器上。该工具适合为 SIGFM 提供可用于 upstream 的多用户验证，但不是驱动的运行时依赖；release build 仍不包含生物特征 capture dump 功能。

## 各发行版安装说明

rel24–rel61 的旧测试记录保存在[技术历史](docs/history/)、[研究日志](docs/research-log.md)以及 Git 历史中，它们不是当前的安装步骤。推荐使用 rel71.30 预览版；rel71.24 是最近通过完整冷启动和 deep-S3 验证的回退版本。

### Arch / CachyOS

从仓库根目录执行：

    ./fingerprint/install-arch.sh

安装器会检查 GXFP51A0、构建并安装审核过的 libfprint/fprintd、仅增加驱动所需的 gpiochip 权限，并安装一次性的 boot prime；不会安装周期 keepalive，也不会安装外部 S3 resume hook。仅在 Plasma 6.7.5 上，它还会应用 package-managed、幂等且可回滚的 KDE/Plasma Login Manager 兼容集成；其他桌面继续使用自己的原生 fprintd/PAM 集成。之后可使用桌面标准设置，或：

    # 请通过 KDE/GNOME 原生图形界面添加、删除和验证指纹。
    # 安装程序不启动命令行录入或验证。

驱动要求 20 次 enrollment 按压。每次轻微移动手指，让 80×64 小传感器覆盖不同区域。

### 旧开发模板

当前磁盘格式为 driver template v4 / SIGFM v3。如果之前安装过本仓库的早期开发版本，可能需要一次性删除旧模板并重新 enrollment：

    # 如需删除旧指纹，请使用桌面图形界面，不要批量删除。

全新安装不需要此步骤。

### Debian / Ubuntu / Fedora / openSUSE / Alpine / 其他 Linux

可移植源码安装器会重建精确固定的 libfprint candidate，并将替换隔离在 `/usr/local`：

    ./fingerprint/install.sh

支持 Arch/CachyOS、Debian/Ubuntu、Fedora、openSUSE、Alpine 的构建依赖。Arch/CachyOS 委托给原生 pacman 包；其他系统先 stage candidate 并验证发行版 fprintd ABI，再通过 systemd drop-in 或 D-Bus activation wrapper 只让 fprintd 使用本地 libfprint，同时保存 rollback manifest。

回滚：

    sudo /var/lib/gxfp51a0-local-install/uninstall.sh

只验证构建而不安装：

    ./fingerprint/install.sh --build-only

## Matcher

生产 matcher 使用：

- 自适应背景减除；
- percentile normalization + unsharp；
- 两级 multi-scale FAST-9 keypoints；
- 非定向 BRIEF-256 descriptors；
- mutual-best cross-check + Lowe ratio filter；
- 200 次 rigid RANSAC，2 px inlier tolerance；
- 最小二乘 rigid refinement；
- 在 20 个 enrollment view 中取最佳分数。

驱动不会在失败后降低阈值、累加多个弱分数，也不会从失败/低置信验证中学习。`GXFP_MATCH_DIAGNOSTICS` 下的 pixel/ZNCC scorer 仅供研究，不参与认证决策。

## 贡献者验证

    make -C fingerprint verify

主要 release gate 包括：

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

验证 target 不执行主动传感器传输、GPIO/MMIO 写入或固件操作。普通用户不需要 `fingerprint/tools/` 下的维护诊断。

## 安全与隐私

请勿提交或发布：

- 指纹采集图像或 enrollment 模板；
- PMK/PSK/密钥材料或设备专用 fixture；
- Goodix/Huawei 专有二进制或固件；
- 序列号或私有机器标识。

已验证的 PMK cache 仍作为受保护的 runtime 状态保存在 `/var/lib/fprint/`。rel43 不持久化任何自适应 timing；升级时只删除旧版留下的非敏感 timing 整数。v4 fprintd 模板属于生物特征数据，应按敏感认证数据保护。驱动不会刷写传感器固件。

## 支持范围

已证明的目标是上述 GXFP51A0 / GF3658 / ST411 精确组合。即使 ACPI HID 相同，其他机器仍可能有不同的 GPIO 接线、固件或主板集成。

这是 reverse-engineered 的实验性生物识别软件。当前验证主要来自参考设备和同一用户的跨手指负样本，不等同于大规模跨人群生物识别认证。不要把指纹单独视为高保证级安全因素。

更多信息：

- [桌面原生集成](docs/native-desktop-integration.md)
- [provenance](PROVENANCE.md)
- [驱动源码](driver/goodix51a0/)
- [research log](docs/research-log.md)
- [当前 handoff](HANDOFF_CURRENT.md)

生产驱动子树使用 `LGPL-2.1-or-later`，具体以各文件 SPDX 标记和 [PROVENANCE.md](PROVENANCE.md) 为准。
