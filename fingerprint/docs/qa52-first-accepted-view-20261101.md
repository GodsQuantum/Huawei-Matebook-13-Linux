# QA52 - first accepted view shortcut (lab only)

- Source: hardware-record QA42 branch qa/gxfp71.74-fdt-typed32-20261030, not QA49 or rejected QA50.
- Only when GXFP_VERIFY_FIRST_ACCEPT_QA=1: stop traversing remaining views of the single Verify template after the unaltered per-view SIGFM gives score >=7.
- On every negative, near miss, Identify/multi-print, diagnostics or missing opt-in: perform full existing scoring, strict pixel rescue and recovery unchanged.
- No threshold relaxation, score pooling/fusion, score-dependent recapture, change to TLS, FDT, cleanup, enrollment, S3/S4, KDE or PAM.
- Potential CPU benefit only on Verify where an early enrolled view already matches. It cannot improve failures caused by stale ImageBase (3/7 etc.) or the ~0.55s mandatory cleanup.
- Hardware and real-time benchmark: not yet validated. Do not deploy or promote from software PASS alone; use rollback QA49 and compare to QA42 baseline.

## CT700 evidence (1 November 2026)

- Complete make -C fingerprint verify: PASS, native libfprint v1.94.100 build PASS, source manifest PASS, no sensor I/O.
- 4 software invariance tests PASS, 30,000 synthetic score arrays and two defensive source assertions.
- Deterministic synthetic SIGFM test (24 identical views, first one already qualifying, 20 iterations): full traversal 191.118 ms/Verify, shortcut 8.094 ms/Verify, **183.024 ms best-case CPU reduction**.
- This is a *synthetic* optimistic bound, not an expected/guaranteed improvement to Pegasus or a first-pose result. On an impostor/near miss or a genuine score rescued by pixel overlap from a late view, full scan remains unchanged.
- Security/acceptance decision unchanged for every test vector; score magnitude returned by Verify can differ when it already exceeds threshold. Identify and diagnostic/full-scan are unaffected.
- Deploy only as separate opt-in QA with verified rollback, and measure actual LOCK and score, plus no-regression S3/S4, before promotion.

## Upstream cross-check - sources consulted 2026-11-01

- Goodix sibling GDIX51C0 has a **native ChicagoHS matcher**, 12-stage position-aware enrolment, 50-entry adaptive templateStudy, quality/coverage checks, and no-finger ImageBase preserved only after FDT-up. However, it uses a different MCU/integration and is tested on a MateBook 16s, **not** a drop-in replacement for GXFP51A0: https://github.com/berkekbgz/libfprint-goodix-spi/blob/main/re/PARITY.md
- The GXFP51A0 STM32 ST411 first-contact/protocol reverse engineering explicitly distinguishes its MCU from sibling ChicagoHS models: https://github.com/szlukabence/goodix-fingerprint-spi-linux
- The independent GXFP5187 matcher reports inflated impostor scores with richer multi-view fusion, so it must not be borrowed without genuine impostor tests: https://github.com/Sigfrodr/libfprint-goodixtls
- GLib timeouts are not precise; do not claim g_timeout_add(1) guarantees 1-ms detection: https://docs.gtk.org/glib/func.timeout_add.html
- NIST NFIQ2 specifically benchmarks 500-PPI plain fingerprint impressions. Do not treat its score or models as validated for a 64x80 tiny area sensor: https://www.nist.gov/services-resources/software/nfiq-2

Priority after QA52: the genuine **3/7 to 13/7 quality variance** and pre-READY ImageBase refresh dominate reliability. Compare safe FDT-up background refresh in an isolated experiment and add quality/coverage measurement without storing images or changing threshold or automatically writing enrolled templates. No code transplant or adaptive template persistence without secure rollback and genuine/impostor trials.
