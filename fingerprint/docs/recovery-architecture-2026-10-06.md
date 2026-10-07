# GXFP51A0 recovery architecture — 2026-10-06

## Product rule

The driver must remain a normal Linux userspace fingerprint stack:

`GXFP51A0 -> libfprint -> fprintd -> PAM -> desktop`

No kernel pinning, controller-specific kernel patch, periodic keepalive, or persistent
recovery sidecar is part of the product architecture.

## Evidence used

### Sigfrodr/libfprint-goodixtls

The current GXFP5187 recovery helper documents a measured deep-lock state in which
short commands still work but long transfers/TLS do not. It states that neither a
GPIO reset nor reopening the spidev node clears that state by itself; resetting the
spidev kernel binding is the decisive recovery boundary. Its recovery sequence stops
fprintd, unbinds the target SPI child, performs a short target reset, waits, rebinds
spidev, then starts fprintd.

Relevant upstream:
- https://github.com/Sigfrodr/libfprint-goodixtls/blob/dda67c8affef6c2ba3fc45145539db2768eb96b2/gx-recover.sh
- https://github.com/Sigfrodr/libfprint-goodixtls/commit/03959b6b8cb16ea88663b2523b5a62867e7ae096

### szlukabence/goodix-fingerprint-spi-linux

This is directly GXFP51A0/ST411 research. Its recovery probe explicitly exercises:
- spidev unbind/rebind only;
- reset only;
- reset while unbound;
- combined reset + rebind.

The same contributor tested our driver in GodsQuantum/huawei-matebook-13-linux #6
and established the distinction between a genuine matcher verdict and transport
desynchronisation. rel24 transport recovery eliminated the normal-use degradation on
the 2020 machine; deep S3 remained a real cold/power boundary.

Relevant upstream:
- https://github.com/szlukabence/goodix-fingerprint-spi-linux/blob/7b8284898696e26a2fd5cb9a05b8605012439a7d/tools/recover_probe.py
- https://github.com/GodsQuantum/huawei-matebook-13-linux/issues/5
- https://github.com/GodsQuantum/huawei-matebook-13-linux/issues/6

### berkekbgz/libfprint-goodix-spi

The GDIX51C0 implementation treats a hardware-open boundary separately from a warm
session and documents hardware validation for daemon restart, post-restart match and
suspend/resume cold-open refresh. Its GPIO reset request is short-lived rather than
kept as a persistent output handle.

Relevant upstream:
- https://github.com/berkekbgz/libfprint-goodix-spi/blob/010a665f54089a1632b1ab7be588b316ace934e2/docs/SUPPORTED_DEVICES.md
- https://github.com/berkekbgz/libfprint-goodix-spi/blob/010a665f54089a1632b1ab7be588b316ace934e2/drivers/gdix51c0/gdix51c0-proto.c

## reference MateBook 13 evidence

rel71.17 proved that all of the following can still leave ST411 TLS unusable:
- GPIO264 reset with SPI open;
- closing /dev/spidev1.0;
- GPIO264 reset with the SPI fd closed;
- reopening /dev/spidev1.0;
- fresh staging fallback.

The remaining proven recovery boundary is therefore the spidev kernel binding itself.

## rel71.18 design

Recovery is performed before libfprint enumerates the reader, through an
`ExecStartPre` of the existing fprintd service:

1. verify exact target `spi-GXFP51A0:00`;
2. refuse to detach any non-spidev driver;
3. unbind only `spi-GXFP51A0:00` from spidev;
4. execute the same reviewed GPIO264 HIGH 300 ms -> LOW 600 ms reset implementation
   used by the driver;
5. leave one second of quiet settle time;
6. restore `driver_override=spidev`;
7. bind only `spi-GXFP51A0:00`;
8. wait for the binding, then udev settle;
9. start ordinary resident fprintd.

This happens before an FpContext/FpDevice exists, avoiding a hot-remove underneath a
live libfprint object.

### Explicit non-goals

The recovery never:
- unbinds `pxa2xx-spi.4` or any parent controller;
- touches GPIO112;
- changes the kernel;
- changes firmware;
- changes PMK/templates/enrollments;
- installs a recovery daemon/service/timer;
- patches KDE/Plasma.

The helper is a short-lived fprintd preflight and fails visibly on required recovery
errors.
