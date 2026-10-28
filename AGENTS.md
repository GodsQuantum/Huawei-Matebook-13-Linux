# Maintainer instructions — Huawei MateBook 13 Linux

This is the public source repository. Read README.md, docs/PROJECT_MAP.md, and fingerprint/HANDOFF_CURRENT.md before modifying driver or installers. The current reference-machine release is rel71.74 (matebook13-rel71.74); repeated S4 is NOT certified. Historical rel71.69 and older branches, trial documents and old README sections are NOT current installation instructions.

## Changes
- Preserve native fprintd/PAM/password fallback, enrolled templates, matcher threshold 7 and authenticated TLS-GCM. No speculative firmware erase/UPFW, GPIO112 manipulation or bypassing PAM/global sudo policy.
- Work within an isolated development environment; preserve source and library ABI. Test fingerprint/research offline and the entire fingerprint verify suite. Any reliability/speed claim requires actual user-attested LOCK / real S3 evidence; unit CI alone does not certify hardware.
- Keep version, package config, README languages, Hall of Fame trials, docs and HANDOFF_CURRENT consistent. Respect CONTRIBUTING.md and SECURITY.md; never commit biometric images/templates, PMK/PSK, Windows proprietary blobs, personal paths/IPs or credentials.
- Prefer native driver/library/platform features over services, polling, prestart GPIO hacks or extra sleep hooks. Rejected rel71.65/66/68 reset/timing experiments are historical.
- Use the current published release tag matebook13-rel71.74 and its SHA256 evidence as installation references (older verified packages remain rollback candidates), not arbitrary worktree files. Test and document changes before merging. Keep public documentation accessible to humans and agents with links, not accumulated stale checkpoint headlines.
