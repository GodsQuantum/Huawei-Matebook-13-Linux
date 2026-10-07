# Candidate checkpoint — GXFP51A0 rel71.23

**Date:** 2026-10-06
**Base:** rel71.22 / exact rel71.20 driver
**Status:** candidate

rel71.23 changes only the fprintd prestart recovery timing for deep Milan lock-ups.

Sequence:

```text
unbind spidev
→ 1 s fully unbound quiet
→ GXFP51A0 GPIO264 reset HIGH 300 ms / LOW 600 ms
→ 2 s detached quiet
→ rebind spidev
→ 1 s post-rebind quiet
→ start fprintd
→ package post-upgrade one-shot prewarm
```

The libfprint driver source is unchanged from rel71.20/71.22. Its executable `.text` and `.rodata` section hashes remain identical.

Package SHA256: `c048ce9260a98dc39420e898dbfe8a621572f66db3f33398945bdeb9149526fa`

Packaged lib SHA256: `0c6206841f6c1da417f8cd8290a6676f660edcc34991b97f7dead40eefb4a4b1`

Packaged prestart helper SHA256: `236d4596d0bf2b479e5f1af8976910d034eb2ac192c679a73fc4a50c6ff3a539`

Shared `.text` SHA256 with rel71.20/71.22: `624642e8f853ce2d6cc6720b834e4ccea1bace85f184223e6916673e53d56275`

Shared `.rodata` SHA256: `8c071f25fd3b3faf13f51b0ca4df3c679adf19eee082b63fce81e6eebdcc0297`

Targeted prestart/boot/quality-only retry tests: PASS. ABI relocation: PASS.

Live gate: install on the currently deep-locked sensor. Success means package-triggered prewarm reaches `PREWARM_RESULT=READY` without reboot.
