# rel71.56 proposal: faster idle deep-S3 preflight (QA ONLY)

Status: experimental branch; no Pegasus installation or hardware certification.

## Verified rel71.55 reference measurements

Pegasus kernel 7.3.0-3-cachyos on 2026-10-23:
- Real automatic **deep S3** entered 14:05:02.096+02, exited
  14:11:54.796+02 (~6m53s asleep).
- Native idle resume begins 14:11:54.823+02.
- Detached GPIO264 HIGH10/LOW150 short reset logged 14:11:55.824+02.
- RAM-only pre-S3 clean background verified with full six-zone early finger
  at 14:11:59.077+02; prewarm READY at the same time.
- Total prewarm ~4.254 s; first physical touch detected 14:11:59.675,
  score 14 at 14:12:00.736 against unchanged matcher threshold 7;
  first-touch success at 14:12:01.748.
- Two unconditional one-second sleeps in idle resume prewarm consume a
  combined two seconds, even if MCU restart and TLS are already responsive.

## QA optimization

With explicit environment GXFP_S3_FAST_PREFLIGHT=1, the first detached
S3 preflight uses 200 ms + 200 ms settle windows. Theoretical fixed-wait
saving: 1.6 s; **this is not a measured hardware improvement**. Normal
installation without the flag remains byte-for-byte equivalent in sleep
duration and retry behavior.

If fast preparation fails, close transport, abandon provisional TLS and
background, then retry once using the original 1 s + reviewed full GPIO264
300/600 ms reset + 1 s before allowing the standard Claim fallback.
Never fast-reset with open SPI/IRQ, reuse failed ImageBase, bypass PAM,
relax the match score 7, or replay accepted GET_IMAGE under a degraded TLS
session. This still needs in-vivo S3/first-touch regression tests.

## Certification gate

Build and source-safety tests must pass first. On Pegasus, only after
preserving the known-good rel71.55 and KDE stock rollback, run supervised
S3 repeats including active verification, wake with finger already down,
finger-off cold background, 3 LOCKs, reboot, negative-finger and password
fallback; log actual resume latency and compare with rel71.55.
Do not merge or label production ready without those tests.

Independent KDE lockscreen measurement: the existing KScreenLocker 6.8
greeter takes ~1.7-1.8 s to request the fingerprint reader; reducing this
requires a separate review of upstream PAM worker readiness and should
not use a blanket QML autostart across suspend (battery regression risk).
