# Maintainer navigation — Huawei MateBook 13 Linux

**Current stable reference for GXFP51A0/ST411: rel71.69** (27 October 2026), not a multi-machine or repeated-S3 certification.

## Entry points
| Location | What to read or change |
|---|---|
| README.md / README.FR.md / README.ZH-CN.md | Public overview, supported hardware, release link |
| AGENTS.md and CONTRIBUTING.md | Rules, tests and submission requirements |
| install.sh | Root HUAWEI Whiptail onboarding, selects modules; preserves existing settings |
| matebook13-doctor.sh | Read-only hardware audit, native fprintd check |
| gpu-power/ | MX250 / power management, **separate** from fingerprint |
| fingerprint/README.md and fingerprint/HANDOFF_CURRENT.md | Driver status and clear recovery path |
| fingerprint/driver/goodix51a0/ | Actual GXFP51A0 native libfprint code and SPI/TLS/FDT implementation |
| fingerprint/packaging/arch/ | Native Arch/CachyOS package, fprintd runtime unit; rel71.69 enables CS/FDT/S3 flags |
| fingerprint/install.sh, install-arch.sh, install-linux.sh | Driver-only installers for supported distributions |
| fingerprint/integration/ | Native desktop/PAM or other integration files; password remains available |
| fingerprint/research/ and fingerprint/scripts/ | Offline tests, compatibility tooling and HOF generation |
| fingerprint/docs/rel71.69-hardware-final-20261027.md | Operator-confirmed first-touch LOCK, S4 and deep-S3 metrics |
| fingerprint/docs/HALL_OF_FAME.md | Actual historical trial table; rejected candidates remain visible |
| fingerprint/docs/rel71.69-cs-setup-and-upstream-audit-20261027.md | Physical hypothesis, SPI kernel trace, upstream comparisons |
| fingerprint/docs/rel71.69-release-20261027.md | Packaged release, checksums and constraints |
| fingerprint/docs/HANDOFF_FULL_UNTIL_20261027_PRE_HYGIENE.md | Archived long development handoff; **historical only** |

## Current product versus historical research
- **Source of truth:** git main, published tag matebook13-rel71.69, package and driver-only source archive on GitHub releases. No dependency on private operator experiments.
- **Native driver:** fprintd and libfprint, threshold 7, encrypted TLS image, FDT proof, SPI-CS setup 50us. Operator's three first-touch successes do not prove cold-boot or multiple-S3 success on other hardware.
- **Not an active issue by default:** older rel71.30 preview instructions, abandoned long-reset attempts 71.65/71.66/71.68, private developer worktrees and old release packages. They are historical baselines; do not delete Git history.
- **Rollback:** upgrade/downgrade only via a verified package, preserving enrollment and password. No global PAM/sudo edits, forced suspend/reboot, GPIO112 or firmware changes during doc refactoring.
- **Privacy:** vendor binary/firmware research and human biometric artifacts are never committed. Use sanitized timings and aggregate scores in GitHub discussions.

## Routine review
Before change: git status, scope, hardware/model revision, release source, benchmark evidence and rollback. After change: code/tests/CI, README EN/FR/ZH consistency, HOF/handoff update and confirmation on real hardware if the claim depends on it.
