# Maintainer instructions — Huawei MateBook 13 Linux

Before changing, building or installing any GXFP51A0 candidate, **read**
`fingerprint/docs/HALL_OF_FAME.md`, `fingerprint/docs/HARDWARE_TRIALS.csv`,
`fingerprint/HANDOFF_CURRENT.md` and the active Cloud9 workspace handoff.
Use the measured winner's *code* as a new experimental base, with separately
validated KDE/desktop corrections; also benchmark the currently installed
version. Never promote an acceleration in READY→DETECT when first-touch
matching regresses. Keep both failed trials and the rollback package.
A record is not a reliability certification; compare the same end-to-end
metric under real user-attested LOCK, S3 and S4 trials.


This is the public source repository. Read README.md, docs/PROJECT_MAP.md, and fingerprint/HANDOFF_CURRENT.md before modifying driver or installers. The current reference-machine release is rel71.74 (matebook13-rel71.74); repeated S4 is NOT certified. Historical rel71.69 and older branches, trial documents and old README sections are NOT current installation instructions.

## Changes
- Preserve native fprintd/PAM/password fallback, enrolled templates, matcher threshold 7 and authenticated TLS-GCM. No speculative firmware erase/UPFW, GPIO112 manipulation or bypassing PAM/global sudo policy.
- Work within an isolated development environment; preserve source and library ABI. Test fingerprint/research offline and the entire fingerprint verify suite. Any reliability/speed claim requires actual user-attested LOCK / real S3 evidence; unit CI alone does not certify hardware.
- Keep version, package config, README languages, Hall of Fame trials, docs and HANDOFF_CURRENT consistent. Respect CONTRIBUTING.md and SECURITY.md; never commit biometric images/templates, PMK/PSK, Windows proprietary blobs, personal paths/IPs or credentials.
- Prefer native driver/library/platform features over services, polling, prestart GPIO hacks or extra sleep hooks. Rejected rel71.65/66/68 reset/timing experiments are historical.
- Use the current published release tag matebook13-rel71.74 and its SHA256 evidence as installation references (older verified packages remain rollback candidates), not arbitrary worktree files. Test and document changes before merging. Keep public documentation accessible to humans and agents with links, not accumulated stale checkpoint headlines.
