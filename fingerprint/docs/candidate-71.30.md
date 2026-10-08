# Candidate rel71.30 — connected enrollment + conservative near-miss rescue

**Date:** 2026-10-08
**Status:** candidate; not yet promoted to a validated release
**Validated rollback:** rel71.24 (rel71.18 remains the older immutable rollback reference)

## Why this candidate exists

The transport/lifecycle path can be fully ready while a genuine placement still scores only 2–4 against a threshold of 7. A fresh 20-view enrollment also showed weak internal topology: many same-finger view pairs barely overlapped, while the measured inter-finger corpus remained well separated.

A current aggregate screen over three 20-view templates reported:

- 570 same-finger view pairs;
- only 16 / 570 same-finger pairs scored >= 7;
- 1,200 inter-finger pairs;
- inter-finger primary-score maximum: 4;
- strict photometric Gate A rescues: 6 same-finger pairs, 0 / 1,200 inter-finger pairs.

Those numbers are local engineering evidence, not a population FAR claim.

## What was rejected

### Lowering the global threshold

Rejected. The authentication threshold stays fixed at **7**.

### Multi-view score union / MATCH_FUSION

Rejected. Earlier live testing showed cross-view union could inflate wrong-finger candidates far above their primary single-view score. The production decision therefore remains based on one enrolled view at a time.

A 2026 ST411/GXFP5187 driver uses a similar union of probe keypoints explained by multiple template views. That approach was reviewed but is not imported here because the reference MateBook 13 already has contrary wrong-finger evidence.

### Replacing FAST/BRIEF with the reviewed RootSIFT matcher

Rejected for this release. Offline leave-one-out evaluation on the current 60 enrolled views did not improve the zero-observed-FAR operating point. RootSIFT multi-view fusion also increased impostor scores materially.

## rel71.30 authentication policy

Primary authentication is unchanged:

1. one physical placement;
2. first usable image is the only scored biometric draw for that placement;
3. score each enrolled view independently;
4. best single-view SIGFM score;
5. fixed acceptance threshold **7**.

A secondary **per-view** photometric rescue may promote a near miss only when all of these are true:

- primary best score >= 4 and < 7;
- the same enrolled view has SIGFM score 3..6;
- overlap >= 1500 pixels;
- ZNCC >= 0.750;
- pixel agreement >= 0.845.

No scores are summed across views. The rescue consumes no extra biometric draw.

## rel71.30 enrollment policy

The previous enrollment path accepted every capture with enough keypoints. On a partial 80x64 sensor this can spend fixed enrollment slots on views that are almost disconnected from the useful core of the template.

rel71.30 keeps **20 accepted views**, but a rejected view no longer advances the enrollment counter.

### Anchor: accepted views 1–5

- first valid view is accepted;
- each following anchor view must score at least **3** against an already accepted view.

This deliberately creates redundancy around the user's natural placement.

### Connected expansion: accepted views 6–20

A candidate view is accepted when either:

- it scores at least **3** against one accepted view; or
- it scores at least **2** against at least **two** accepted views.

Otherwise the driver asks for another placement without advancing the stage.

These low enrollment-connectivity scores never authenticate a login and do not alter the threshold of 7.

## Build and safety gates

rel71.30 was built and verified away from the target sensor:

- full research/source test suite: PASS;
- reproducible libfprint v1.94.100 build: PASS;
- Arch package build in an ephemeral Arch build environment: PASS;
- release biometric dump hook: ABSENT;
- active sensor I/O during build: NONE;
- GPIO writes during build: NONE;
- MMIO writes during build: NONE;
- firmware actions during build: NONE.

Reference Arch package build (isolated development LXC, ephemeral `archlinux:base-devel`, GCC 16.2.1, pinned Meson 1.12.0 / Ninja 1.13.2):

Package SHA-256:

```text
4d8065f9e55dafe7e8dd38d7687b33c8b14199498d59acd99f99523389037aef
```

Packaged `/usr/lib/libfprint-2.so.2.0.0` SHA-256:

```text
efa5d5bda4e15c22336ab76c0e69294bf6da303e5d42277128fb808397b60297
```

The package installs as:

```text
libfprint-goodix51a0 1.94.100.goodix51a0-71.30
```

## Validation still required

Before tagging rel71.30 as validated:

1. re-enroll at least the main login finger with rel71.30 so the connected-enrollment policy is actually used;
2. validate repeated ordinary lock/unlock attempts, emphasizing first-placement success;
3. run cross-finger negative controls;
4. validate one cold boot and one deep-S3 resume;
5. confirm no regression in prewarm / TLS recovery.

Until those gates pass, rel71.24 remains the public validated checkpoint.
