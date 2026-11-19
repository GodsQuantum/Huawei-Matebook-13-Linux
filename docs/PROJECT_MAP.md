# Maintainer navigation — Huawei MateBook 13 Linux

**Current source installer: QA63.2** (19 November 2026, Arch package QA63-3). Latest published release: rel71.74 (28 October).

**Pegasus KDE status (19 November 2026).** The optional v12 QML is quarantined following a post-resume lockout. Pegasus runs stock Plasma 6.8.3 QML and remembers `Regular`. Driver QA63-3 and native KScreenLocker 6.8.3-1.2 were installed at 15:33 CET; native Claim/Release and file integrity checks passed. Physical unlock with this changed stack is still unverified. `Fingerprint=false` hides the explicit fingerprint method while the parallel worker can still run. The operator previously confirmed one password unlock after kernel-confirmed deep S3 using the preceding stock greeter. The two v12 biometric recoveries from 18 November are historical and required repositioning. Repeatable password fallback, first-touch S3, suspend power, cold boot, repeated S4 and other hardware remain unverified. [Incident and hardware evidence](../fingerprint/docs/real-hardware-certification.md).

## Entry points
| Location | What to read or change |
|---|---|
| README.md / README.FR.md / README.ZH-CN.md | Public overview, supported hardware, release link |
| AGENTS.md and CONTRIBUTING.md | Rules, tests and submission requirements |
| install.sh | Root HUAWEI Whiptail onboarding, selects modules; preserves existing settings |
| matebook13-doctor.sh | Read-only hardware audit, native fprintd check |
| gpu-power/ | MX250 / power management, **separate** from fingerprint |
| fingerprint/README.md, README.FR.md, README.ZH-CN.md | Current public driver status, guided enrollment and recovery |
| fingerprint/driver/goodix51a0/ | Actual GXFP51A0 native libfprint code and SPI/TLS/FDT implementation |
| fingerprint/packaging/arch/ | Current QA63.2 Arch/CachyOS source package and fprintd runtime |
| fingerprint/install.sh, install-arch.sh, install-linux.sh | Driver-only installers for supported distributions |
| fingerprint/integration/ | Native desktop/PAM and lockscreen integration files |
| fingerprint/research/ and fingerprint/scripts/ | Offline tests, compatibility tooling and HOF generation |
| fingerprint/docs/rel71.69-hardware-final-20261027.md | Operator-confirmed first-touch LOCK, S4 and deep-S3 metrics |
| fingerprint/docs/HALL_OF_FAME.md | Actual historical trial table; rejected candidates remain visible |
| fingerprint/docs/rel71.69-cs-setup-and-upstream-audit-20261027.md | Physical hypothesis, SPI kernel trace, upstream comparisons |
| fingerprint/docs/rel71.69-release-20261027.md | Packaged release, checksums and constraints |
| fingerprint/docs/HANDOFF_FULL_UNTIL_20261027_PRE_HYGIENE.md | Archived long development handoff; **historical only** |

## Current product versus historical research
- **Source of truth:** Git `main` for its installed driver source, `fingerprint/STABLE_RELEASE` for the latest published rel71.74 tag, and a separately tested feature branch for proposed changes. The private canonical handoff is stored **outside** the public repository.
- **Native driver:** fprintd and libfprint, threshold 7, encrypted TLS image, FDT proof, SPI-CS setup 50us. Reported one-pose LOCK/S3 does not prove repeatable genuine S4, cold boot or other hardware.
- **Not an active issue by default:** older rel71.30 preview instructions, abandoned long-reset attempts 71.65/71.66/71.68, archived worktrees and old release packages. They are historical baselines; do not delete Git history.
- **Rollback:** upgrade/downgrade only via a verified package, preserving enrollment and password. No global PAM/sudo edits, forced suspend/reboot, GPIO112 or firmware changes during doc refactoring.
- **Privacy:** vendor binary/firmware research and human biometric artifacts are never committed. Use sanitized timings and aggregate scores in GitHub discussions.

## Routine review
Before change: git status, scope, hardware/model revision, release source, benchmark evidence and rollback. After change: code/tests/CI, README EN/FR/ZH consistency, HOF/handoff update and confirmation on real hardware if the claim depends on it.
