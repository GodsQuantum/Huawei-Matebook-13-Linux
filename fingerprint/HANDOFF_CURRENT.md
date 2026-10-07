# Public handoff — GXFP51A0 rel71.24

Current public production checkpoint: **rel71.24**.

- target: Huawei MateBook 13 / GXFP51A0 / GF3658 ST411
- libfprint base: `v1.94.100`
- threshold: **7**
- template: v4, 20 views
- warm readiness: ~82–83 ms on the validated reference unit
- recovery: target-only spidev unbind/rebind + short active-HIGH GPIO264 pulse + complete GPIO request release
- deep-S3 transport: validated
- firmware flashing: none
- GPIO112: never touched
- PMK/templates: preserved
- score fusion: disabled / absent
- score-conditioned same-placement retry: absent

rel71.18 remains the immutable rollback reference.

See:
- `docs/validated-checkpoint-71.24.md`
- `docs/validated-checkpoint-71.18.md`
- `docs/recovery-architecture-2026-10-06.md`
- `README.md`

Do not publish fingerprint captures/templates, PMK/PSK material, serials, hostnames, usernames or private filesystem paths.