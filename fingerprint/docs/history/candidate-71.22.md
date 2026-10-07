# Candidate checkpoint — GXFP51A0 rel71.22

**Date:** 2026-10-06
**Base:** rel71.20 driver, unchanged
**Status:** candidate

## Why rel71.21 was rejected

rel71.21 incorrectly generalized a transport recovery rule to protocol-defined resets. In particular, it changed the validated post-PMK reset and TLS retry resets into close→reset→reopen boundaries. Live cold-prewarm testing then produced repeated TLS `digest check failed` errors and failed before any biometric score.

The comparison with the successful rel71.20 cold-prep showed the key distinction:

- **protocol reset**: part of the validated Windows/Goodix PMK/TLS sequence; transport continuity is intentional;
- **recovery reset**: used after transport/TLS desynchronization; detached close→GPIO264→reopen is appropriate.

rel71.21 is therefore **rejected** and must not be promoted.

## What rel71.22 changes

rel71.22 restores the **exact rel71.20 driver source** and changes only Arch package upgrade orchestration:

```text
pacman upgrade
→ synchronous restart fprintd
→ fprintd ExecStartPre performs target-only spidev unbind/reset/rebind
→ immediately start one-shot gxfp51a0-boot-prewarm
→ pacman returns only after prewarm has completed
```

This prevents the user's first later lock from becoming the first cold Claim.

## Driver identity vs rel71.20

- `fingerprint/driver/goodix51a0/`: source diff vs rel71.20 = NONE
- `.text` SHA256 is identical between rel71.20 and rel71.22
- `.rodata` SHA256 is identical between rel71.20 and rel71.22
- exported dynamic symbol set is identical
- ABI relocation check passes

The whole ELF SHA differs due to non-executable build metadata; executable and read-only program sections are byte-identical.

rel71.20 / rel71.22 shared `.text` SHA256:
`624642e8f853ce2d6cc6720b834e4ccea1bace85f184223e6916673e53d56275`

rel71.20 / rel71.22 shared `.rodata` SHA256:
`8c071f25fd3b3faf13f51b0ca4df3c679adf19eee082b63fce81e6eebdcc0297`

## Preserved biometric policy

- threshold 7
- first usable image per physical pose scored exactly once
- contrast/keypoint failures may recapture before scoring
- score below 7 requires a new physical pose
- no `MATCH_FUSION`
- existing Template v4 / enrollments preserved

## Package

`libfprint-goodix51a0-1.94.100.goodix51a0-71.22-x86_64.pkg.tar.zst`

SHA256:
`670bdde05ba31f17b1b0cae947142f8b5e6f4426e23554a9759317ec95edc04e`

Packaged lib SHA256:
`909b593708005e6fda17cb8ee19ad0f6972ed2938c77470822bb8df57dfd1522`

Prestart helper SHA256:
`ed08bf62951cf64fb5f962f01764338be97faeac2d3dcdbd4013cd2b4fe4fe80`

## Validation

- driver source identical to rel71.20: PASS
- targeted boot/prewarm/quality-only retry tests: PASS
- package clean build: PASS
- ABI relocation: PASS
- package upgrade restart→prewarm order: PASS
- forbidden keepalive/sleep/KDE artifacts: ABSENT
- rel71.21 detached-fallback strings: ABSENT
- old score-conditioned retry markers: ABSENT

## Required live gate

1. install rel71.22;
2. verify post-upgrade prewarm result;
3. confirm the prewarm uses the rel71.20 cold-prep behavior;
4. then run ordinary KDE locks;
5. only after normal locks pass, run manual deep S3.
