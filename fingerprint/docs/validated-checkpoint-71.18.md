# Validated checkpoint — GXFP51A0 rel71.18

**Date:** 2026-10-06
**Target:** Huawei MateBook 13, ACPI `GXFP51A0`, Goodix GF3658 / ST411, firmware `GF_ST411SEC_APP_14115`
**libfprint base:** `v1.94.100`
**Checkpoint:** `71.18`

This document freezes the first rel71.x state that simultaneously proved:

- recovery from a previously TLS-degraded reader **without reboot or power-cycle**;
- fast warm Claims through the native libfprint/fprintd lifecycle;
- three consecutive ordinary KDE lock/unlock authentications succeeding on the **first physical finger placement**;
- the original SIGFM acceptance policy remaining unchanged at threshold **7**;
- no fingerprint capture dumps, periodic keepalive, external suspend hook, firmware flashing, or enrollment/PMK reset.

## Why rel71.18 works

### 1. The real cold boundary is the `spidev` binding

On this ST411 target a deep Milan/TLS desynchronisation can survive all of the following:

- GPIO264 reset while SPI is open;
- closing and reopening `/dev/spidev1.0`;
- GPIO264 reset while the SPI file descriptor is closed;
- a new TLS attempt using the cached PMK;
- the bounded fresh-staging fallback.

rel71.17 proved this directly: the MCU continued to answer short commands, but TLS still failed with digest errors.

rel71.18 therefore adopts the recovery boundary independently reached by current Goodix SPI work from Sigfrodr and szlukabence: tear down the **target child `spidev` binding itself** before libfprint enumerates the reader.

The fprintd pre-start sequence is:

```text
fprintd start/restart
        ↓
unbind only spi-GXFP51A0:00 from spidev
        ↓
GPIO264 active-HIGH reset: HIGH 300 ms → LOW 600 ms
        ↓
1 second quiet settle
        ↓
driver_override=spidev
        ↓
rebind only spi-GXFP51A0:00
        ↓
udev settle
        ↓
ordinary resident fprintd --no-timeout
```

The helper is an `ExecStartPre` of the existing fprintd service. It is **not** a persistent daemon, timer, keepalive service, or sleep hook.

The parent Intel/PXA2xx SPI controller is never unbound and GPIO112 is never touched.

### 2. Recovery runs before libfprint creates the reader object

A hot unbind underneath a live `FpDevice` would look like a udev device removal and make lifecycle handling fragile.

rel71.18 performs the target rebind **before** fprintd creates its libfprint `FpContext`. Once the daemon is running, normal Claims use the native driver state machine.

### 3. Cold preparation and warm authentication are separate

After a successful cold preparation the driver retains the prepared TLS/background/FDT context in the resident fprintd process.

An ordinary Claim therefore performs an FDT-only `FAST_READY` probe instead of repeating the full TLS/device initialization.

Validated warm readiness remains approximately **82–83 ms** on the reference MateBook 13.

Host SPI/GPIO descriptors are closed when idle; there is no periodic polling or keepalive traffic.

### 4. ImageBase has a much shorter freshness lifetime than TLS

A TLS session can remain valid for hours while the raw background plane used for image subtraction becomes stale much sooner.

Before biometric capture, rel71.18 refreshes ImageBase/background+FDT when its age exceeds 20 seconds. This keeps authentication independent from the much longer warm TLS lifetime.

The refresh is native driver work and does not alter enrollment data.

### 5. Transport retry semantics follow the Windows protocol boundary

For `GET_IMAGE`, the driver allows the Windows-parity **1000 ms ACK window**.

A command is replayed only when there is no ACK/TLS evidence for that entire window. If `GET_IMAGE` was accepted and the TLS image later times out, the accepted command is **not replayed**; the session is recovered instead.

The FDT-down path drains both the ACK and the following FDT data response before `GET_IMAGE`, preventing stale response data from contaminating the next transaction.

### 6. The biometric policy is deliberately conservative

The active matcher remains the established FAST/BRIEF/SIGFM path:

- FAST-9, two-level pyramid;
- BRIEF-256;
- Lowe ratio test;
- reciprocal cross-check;
- rigid RANSAC + least-squares refinement;
- best score against the 20 enrolled views;
- fixed acceptance threshold **7**.

Templates remain version 4 and existing 20-view enrollments are preserved.

A diagnostic multi-view union was tested in rel71.16–71.18. It is **not safe as an authentication score**: on the first rel71.18 human lock the correct candidate was baseline `8` / fused `23`, while other enrolled fingers reached baseline `2` / fused `8` and baseline `2` / fused `19`.

Therefore `MATCH_FUSION` remains diagnostic-only in the frozen 71.18 checkpoint and must never replace the baseline decision or reuse threshold 7. It should be removed from the next production cleanup unless redesigned and validated with a real impostor corpus.

## Live validation evidence

The reader was deliberately tested from a TLS-degraded state left by rel71.17.

### Recovery gate

fprintd restart produced:

```text
PRESTART_RECOVERY step=unbind-spidev
PRESTART_RECOVERY step=gpio264-reset-300-600
PRESTART_RECOVERY step=rebind-spidev
PRESTART_RECOVERY=READY unbind+GPIO264+rebind
```

No reboot, power-cycle, or manual sysfs operation was used.

The first non-biometric Claim recovered to `production_ready=1`. A second Claim completed in ~107 ms wall time with `FAST_READY in 83 ms`.

### Human lock gates

Three consecutive ordinary KDE lock/unlock tests then succeeded on the first physical placement:

| Lock | Active baseline score | Threshold | Result |
| --- | ---: | ---: | --- |
| 1 | 8 | 7 | first pose / first image PASS |
| 2 | 11 | 7 | first pose / first image PASS |
| 3 | 12 | 7 | first pose / first image PASS |

No systematic `GET_IMAGE retrying attempt=2/2` and no accepted-command TLS-image timeout appeared in those lock traces.

## Package identity

Arch/CachyOS checkpoint package:

```text
libfprint-goodix51a0-1.94.100.goodix51a0-71.18-x86_64.pkg.tar.zst
SHA256 32496ed97ec4ec93085d943408ffd454fdec0ab7d3f2d2a9931baa41446d8c58
```

Packaged libfprint:

```text
SHA256 283c76c603ab05711405e3a4bc6fa1a556f1435f7f507ced864b93243b2d007b
```

Pre-start recovery helper:

```text
SHA256 ed08bf62951cf64fb5f962f01764338be97faeac2d3dcdbd4013cd2b4fe4fe80
```

## Portability contract

The product boundary is intentionally standard Linux userspace:

```text
GXFP51A0 → spidev → libfprint → fprintd → desktop fingerprint PAM / CLI
```

The core package does not patch KDE/Plasma, GDM, or a global PAM password stack. Desktop-specific compatibility code, when needed for an old desktop release, is opt-in rather than a driver dependency.

The portable installer builds the same driver/recovery path for the supported distro families and validates the local fprintd ABI before installation.

## Safety invariants

rel71.18 must not regress these rules:

- never lower the acceptance threshold below 7;
- never silently delete/re-enroll fingerprints;
- never delete the validated PMK cache as a generic recovery action;
- never dump raw biometric captures/templates in release builds;
- never flash sensor firmware;
- never touch GPIO112;
- never unbind the parent SPI controller;
- never add periodic keepalive/polling;
- never add a persistent recovery sidecar when a native lifecycle boundary can do the job;
- never make a desktop-specific QML patch part of the core driver package.

## Upstream/research influences

The rel71.18 recovery design was cross-checked against:

- Sigfrodr/libfprint-goodixtls — deep-lock recovery by target spidev rebind;
- szlukabence/goodix-fingerprint-spi-linux — GXFP51A0 reset/rebind experiments and direct testing on the 2020 MateBook 13;
- berkekbgz/libfprint-goodix-spi — explicit cold hardware lifecycle and restart/suspend regression gates.

See [recovery-architecture-2026-10-06.md](recovery-architecture-2026-10-06.md) for the detailed rationale.

## Next gates

This checkpoint is deliberately frozen before further optimization.

The next work should be done **after** preserving this state:

1. remove the rejected diagnostic fusion from the production path;
2. re-run normal-lock gates;
3. validate one additional fprintd restart/recovery cycle;
4. validate deep S3;
5. only then consider further matcher/image-quality optimization.
