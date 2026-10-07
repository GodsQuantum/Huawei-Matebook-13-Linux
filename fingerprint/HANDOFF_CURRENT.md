# Fingerprint current boundary

**Current validated checkpoint:** rel71.18 — 2026-10-06

This public handoff intentionally contains only the reproducible product boundary. Internal session logs and machine-specific evidence are not part of the public repository.

## Proven runtime state

- Target: Huawei MateBook 13 with ACPI `GXFP51A0`, Goodix GF3658 / ST411, firmware `GF_ST411SEC_APP_14115`.
- libfprint base: `v1.94.100`.
- Active matcher threshold: **7**.
- Template format: v4, 20 enrolled views retained.
- Warm FDT `FAST_READY`: approximately **82–83 ms** on the validated reference unit.
- rel71.18 recovered a previously TLS-degraded reader without reboot or power-cycle.
- Three consecutive ordinary KDE lock tests passed on the first physical placement with baseline scores **8/7, 11/7 and 12/7**.

## Recovery boundary

Before libfprint enumerates the reader, the existing fprintd service runs a short pre-start helper:

```text
unbind only spi-GXFP51A0:00 from spidev
→ GPIO264 HIGH 300 ms → LOW 600 ms
→ 1 s settle
→ driver_override=spidev
→ rebind only spi-GXFP51A0:00
→ udev settle
→ ordinary resident fprintd
```

The helper never unbinds the parent SPI controller, never touches GPIO112 and is not a persistent service/timer.

## Biometric policy

The production decision remains the baseline FAST/BRIEF/SIGFM matcher with fixed threshold 7 and best single enrolled view. The diagnostic multi-view union present in the frozen rel71.18 checkpoint is non-authoritative and showed unsafe score inflation on wrong-finger candidates; do not promote it to the authentication decision.

## Required references

- [`docs/validated-checkpoint-71.18.md`](docs/validated-checkpoint-71.18.md)
- [`docs/recovery-architecture-2026-10-06.md`](docs/recovery-architecture-2026-10-06.md)
- [`README.md`](README.md)
- [`docs/safety.md`](docs/safety.md)

## Next gates

1. preserve this checkpoint before any optimization;
2. remove the rejected fusion diagnostic from the production cleanup;
3. repeat normal-lock and fprintd-restart gates;
4. validate deep S3;
5. only then continue image-quality/matcher work.
