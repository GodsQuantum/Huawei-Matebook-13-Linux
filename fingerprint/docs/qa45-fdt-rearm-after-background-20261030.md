# QA45 — FDT finger-down rearm after proven warm ImageBase

Status: laboratory candidate. Based on QA42 (PR42 commit 3c92fb0).
Previous QA43 stale-baseline shortcut is excluded; QA44 only measured stages,
and also failed a human LOCK. Keep QA42 on the reference machine pending
separate hardware validation.

## Hardware evidence

Repeated actual user LOCK failures under QA44 and QA42 on 2026-10-30:
- the sensor answers FDT manual probes and refreshed ImageBase successfully
  in 798–807ms; verified baseline idle mean 353–355, threshold drop >30;
- waiting FDT probes for five seconds report mean 353–355 / drop 0–1 despite
  user stating multiple physical finger contacts, never DETECTED_HOLD;
- verification client cancels after 4–5 seconds, independently of the internal
  30-second driver poll budget. Authentication never reached the matcher.
- earlier QA42 with a finger landing during background refresh discarded the
  frame, still scored 13/7 on one physical placement.

## Source comparison (2026-10-30)

- szlukabence/goodix-fingerprint-spi-linux docs/IMAGING.md explicitly
  prescribes finger mode 0x36 then down 0x32 after an empty background,
  and before a finger GET_IMAGE.
- goodix-fp-dump's driver_51x0.py uses the same order on the family.
- Current gx51_capture_recipe.c warm background ends in GET_IMAGE; the
  cleanup recipe runs only after a *finger* frame. gx_warm_validate does FDT
  manual 0x36 before/after the empty image, but after success does not
  restore FDT-down 0x32. gx_session_start only issues raw WakeupMCU.

This is a testable hypothesis, not proof of causality. Recent relevant
GitHub issues include GodsQuantum/Huawei-Matebook-13-Linux #6 (2020 SPI
retries/transport), #32 (S3/S4 capture), and #10 (suspend race); none
confirms this precise warm-FDT loss.

## Narrow QA45 change

- Existing strict checks prove fresh background finger-off before AND
  after warm image; only successful refresh sets warm_bg_refreshed.
- After WakeupMCU and before Verify READY, opt-in
  GXFP_FDT_REARM_AFTER_BG_QA=1 sends exactly mode0x36 -> down0x32 with
  the same FDT_FINGER_MODE bytes as the existing finger-capture recipe.
- Uses existing gx_send_plain_drain with ACK/data proof and timing; a
  failure aborts the operation and flags capture recovery. No bypass.
- Unchanged: double finger-off background proof, 20-second freshness,
  typed FDT-down validation, TLS-GCM image, matcher threshold7,
  enrollment, required cleanup, PAM/KDE and password fallback.
- Disabled by default. Zero timers, background services or polling.
- No reliance on the failed QA43 aged ImageBase bypass.

## Acceptance

C unit exercise of exact 0x36/0x32 pair and metadata/source-safety test,
full make -C fingerprint verify with native libfprint build, GitHub CI
12 checks, reproducible Arch package with SHA256 and QA42 rollback. A
first real LOCK is required to measure finger-detection and first-touch
success before merge/promotion. If first physical test fails, rollback QA42,
record all FDT values and do not label it stable.
