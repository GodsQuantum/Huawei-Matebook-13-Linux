# rel71.65 cleanup settling QA

Opt-in 30..90ms gap before FDT_UP cleanup GET_IMAGE, retains ACK/TLS/NAV/FDT_DOWN, match threshold 7, native fprintd. Hypothesis: reduce occasional 1000ms missing GET_IMAGE ACK observed after score20/7 on natural S3. No proven speedup; hardware A/B pending. No forced reboot or S3. Based on rel71.64 fresh pre-S3 ImageBase; previous failed experiments disabled. Five focused and native software tests PASS; Arch and GitHub CI still required.
