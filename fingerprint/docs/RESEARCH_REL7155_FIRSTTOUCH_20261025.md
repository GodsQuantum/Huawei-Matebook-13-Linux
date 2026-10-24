# GXFP51A0 rel71.55 — optimisation research, 2026-10-25

STATUS: research-only branch based on the certified-worktree rel71.55 source. No driver code changed, no installation on Pegasus.

## Sources verified

- 2026-10-22 direct GXFP51A0 SIGFM / near-miss / gallery benchmark: https://github.com/Sigfrodr/libfprint-goodixtls/issues/5#issuecomment-6344049769
- Goodix sibling GXFP5187 benchmark and explicit biometric security hazards: https://github.com/Sigfrodr/libfprint-goodixtls
- Exact ST411 imaging protocol and adaptive DC: https://github.com/szlukabence/goodix-fingerprint-spi-linux/blob/main/docs/IMAGING.md
- Own GitHub issues #9 (2020 cold TLS Claim) and #10 (in-flight Verify suspend): https://github.com/GodsQuantum/Huawei-Matebook-13-Linux/issues
- Canonical full Hall of Fame including rel71.60 NO-GO: https://github.com/GodsQuantum/Huawei-Matebook-13-Linux/blob/66d26ff/fingerprint/docs/HALL_OF_FAME.md

## Current real-hardware baseline

- rel71.55 historical fastest accepted first-touch LOCK: 1.711s DETECT -> terminal, FDT FAST_READY 72-83ms. Typical accepted first-touch full callback 1.7-2.0s.
- On one 2026-10-24 23:28 rel71.55 success: finger DETECT -> score around 1.03s, score -> terminal around 0.90s, including mandatory capture cleanup. The FDT probe is NOT the main bottleneck.
- Full recent Hall of Fame: rel71.55 5/6 first-touch LOCK overall, 2/3 first-touch genuine deep S3. 71.58-71.60 experimental versions have real hardware regressions. Installed Pegasus driver remains rel71.55, fprintd active with no experimental flags.

## Existing implementation — DO NOT REINVENT

- Native SPI/udev -> libfprint -> fprintd D-Bus -> KDE/Polkit PAM; warm FDT-only readiness, GPIO264 recovery, authenticated SPI/TLS.
- Background subtraction + adaptive 54th-percentile DC centering, 8-bit stretch/unsharp, single independent captured frame per scored physical placement. No image averaging.
- FAST/BRIEF/RANSAC SIGFM v3, fixed threshold 7, fixed 20-view connected enrollment and strict per-view photometric rescue on a marginal primary match. No score fusion or best-of-multiple scored draws on the same finger placement.
- Multi-view RootSIFT/fusion was tested and rejected in docs/candidate-71.30.md and other archived evidence; raw sensor transfer/guidance improvements are present already.
- Exact matcher evaluation ABI was previously implemented at fingerprint/research/eval/gq_sigfm_fpeval.c, suitable for independent test harness evaluation.

## Very useful NEW direct-GXFP51A0 result dated 2026-10-22

The external controlled one-user, four-finger experiment measured single capture false-reject rate with real shipped SIGFM + per-view rescue at threshold 7:

| Enrolled distinct views | 10 | 15 | 20 | 25 |
|---|---:|---:|---:|---:|
| Observed false reject | 22.0% | 12.1% | 7.8% | 5.7% |
| Observed impostor accepted | 0/1815 | 0/1815 | 0/1815 | 0/1815 |

The genuine sample size shrinks for larger galleries and all impostors are from ONE person. ZERO observed false accepts does not imply true FAR=0. The observed impostor score max=6, only one inlier below threshold 7. Another published on-this-sensor EER comparison: shipped SIGFM ~4.73%, RootSIFT ~11.66%, NBIS ~50%. DO NOT lower threshold, switch matcher or use multiple scored captures per press based on these data alone.

## Ranked experiments for a new candidate based on rel71.55

1. FIRST TOUCH COVERAGE: Inspect existing 20-view template consistency and orientation diversity offline. Compare the exact SIGFM pipeline with 20 vs 25 genuinely distinct enrollment views (not duplicates), SAME held-out genuine and different-person impostor captures, same scoring threshold. Keep active fprintd template unchanged; do not reenroll Pegasus without independent rollback and user approval. No adaptive template writes in default path.
2. CAPTURE QUALITY: For failed first touches log only aggregate image-quality metadata (clipped percent, contrast/ridge energy, FDT means/zones, feature count, transport and cleanup times). No raw frames, biometric descriptors, templates, PMK or keys in public logs or Git. A capture rejected BEFORE scoring may be retried for quality, but a scored 4/7 is a negative for that placement. Compare alternate quality gates in a private offline benchmark.
3. CRITICAL PATH LATENCY: Instrument DETECT, GET_IMAGE/ACK/TLS, preprocessing, matcher, mandatory capture cleanup and final PAM callback separately. Roughly 0.8-0.9s elapses after a good scored image before final callback on the validated host. Investigate safe overlap/pipelining only if cleanup ordering and TLS/session lifecycle are preserved. No early positive reports that could race with next use. Do not naively shorten 30ms SPI gaps: sibling GXFP5187 showed 19/19 failures at 10ms.
4. S3 INDEPENDENTLY: Rather than repeat long finger-off waits (rel71.59 failure), examine real native 0x60 park ACK, detached resume, TLS/prewarm, first held-finger FDT, saved background quality, and first-image rejection. A pre-sleep image refresh alone is unproven; rel71.60 failed normal first-touch LOCK and must not be redeployed. Check active-Verify suspend deadlock reported by user #10 separately.

## Gate / privacy / deployment

- Develop in CT700 only, source branch based on rel71.55; no PAM/GPU/PMK/kernel/GPIO112/firmware changes; no reboots or host sleeps performed remotely.
- Use the existing fp_eval real-matcher plugin with held-out genuine and multiple unrelated impostor fingers; record FAR/FRR, raw counts and confidence intervals. Avoid leaking any real biometric data into the public repository.
- Per candidate require native build and cross-distro CI, intact rollback, then human-confirmed 5 LOCK / 3 deep S3 / 3 cold boots / 3 reboots / wrong-finger rejection and password fallback. Those are minimum functional gates, not evidence of a population-level FAR.
- Promote only if first-touch acceptance improves WITHOUT a reduction in security and p50/p95 DETECT -> terminal improve on comparable workload. All tests enter canonical HARDWARE_TRIALS.csv, Hall of Fame and handoff; push all documentation. No production claim before physical results.

NO runtime change is made by this document.
