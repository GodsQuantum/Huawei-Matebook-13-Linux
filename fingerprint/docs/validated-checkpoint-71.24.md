# Validated checkpoint 71.24 — GXFP51A0 / GF3658 ST411

Date: 2026-10-07

## Scope

This checkpoint targets the Huawei MateBook 13 GXFP51A0 / Goodix GF3658 ST411 profile documented by this repository. It is a native libfprint/fprintd implementation pinned to libfprint `v1.94.100`.

rel71.18 remains the immutable rollback checkpoint. rel71.24 keeps the rel71.20 production driver logic and changes the pre-enumeration recovery helper.

## Authentication invariants

- fixed SIGFM threshold: **7**
- template v4 / SIGFM v3, 20 enrolled views
- best single enrolled view; no score addition or fusion
- no `MATCH_FUSION`
- first usable image of a physical placement receives exactly one score
- same-placement recapture is allowed only before scoring for the contrast gate or fewer than 25 keypoints
- a score below 7 requires lift/reposition/new physical placement
- at most three independent physical placements per verification cycle
- no biometric capture dump in release builds

## Recovery boundary

Before fprintd/libfprint enumerates the target:

```text
unbind spi-GXFP51A0:00 from spidev
→ 1 s detached quiet
→ request GPIO264 inactive LOW
→ active-HIGH reset pulse ≈10 ms
→ deassert LOW
→ release the GPIO request completely
→ ≈150 ms settle
→ keep the target detached for the bounded post-reset quiet period
→ rebind spi-GXFP51A0:00
→ post-rebind settle
→ start resident fprintd
→ one-shot prewarm
```

The helper never touches GPIO112 and never unbinds the parent SPI controller.

## Hardware validation

The short-pulse path recovered a reader already stuck in a repeated TLS/digest-failure state without reboot or power-cycle.

Validated runtime properties include:

- `PREWARM_RESULT=READY`
- warm `FAST_READY` around 82–83 ms
- repeated normal KDE lock/unlock operation
- manual deep-S3 suspend/resume with TLS transport remaining operational
- existing PMK, firmware and template-v4 enrollments preserved
- multiple enrolled fingers tested locally with cross-finger negative controls remaining below threshold

Single-placement genuine scores can still vary on the 80×64 sensor. This is handled by independent lift/reposition attempts, not by lowering the threshold or combining weak scores.

## Matcher evidence and limitation

The production matcher remains FAST-9 + BRIEF-256 + reciprocal matching + rigid RANSAC. A privacy-preserving upstream held-out evaluation on the same GXFP51A0/ST411 class reported the real matcher materially outperforming generic ORB, RootSIFT and NBIS alternatives, while also showing that genuine single-press false rejects remain possible.

The observed impostor margin is not sufficient to justify lowering threshold 7. This checkpoint therefore prioritizes conservative authentication behavior over single-placement acceptance rate.

## Package / source

Arch package version:

```text
libfprint-goodix51a0 1.94.100.goodix51a0-71.24
```

Public Arch package SHA256: `909eec96f45fd46d581b5083beeac57f4f2831260a086b6e54a91a70ecc6c94a`

Packaged lib SHA256: `260a2307a5bb00f53d0fcc7ffd5eb5b3a99c4b4c10211eb0201c5942b8fa95e7`

Runtime helpers:

- prestart recovery: `074b3828138f986948a874814f869daf2724a0a764d399fb495bff935e13d77d`
- boot prewarm: `6907d3d95445da1da61a4b2f081b8717c6ba7976ed89f069821f8596bceab611`

The public package's executable/data sections were verified byte-identical to the rel71.24 instance used for hardware validation:

- `.text`: `624642e8f853ce2d6cc6720b834e4ccea1bace85f184223e6916673e53d56275`
- `.rodata`: `8c071f25fd3b3faf13f51b0ca4df3c679adf19eee082b63fce81e6eebdcc0297`
- `.data`: `609923ea4c693a90dd012eb07a080a21cb638163a65b0b1541f5baac0205d55e`