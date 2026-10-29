# rel71.75 lab — investigate delayed FDT-down second replies on 2020 MateBook 13

Default-OFF experimental QA source ONLY. Live Pegasus rel71.74 remains untouched.

## Independent real hardware evidence, 29 October 2026
szlukabence reported detailed independent data on the 2020 MateBook 13 (WRTB-WXX9, GXFP51A0 ST411 14115, Omarchy7.2.8-9), using our official rel71.74:
https://github.com/GodsQuantum/Huawei-Matebook-13-Linux/issues/6#issuecomment-6501280844
- Finger held at s2idle wake: first touch score22 and 4.0s wake→accept; native S4 first touch score16 at7.3s. s2idle is NOT genuine deep S3.
- Normal lock identify 7/9 first poses, 9/9 within2; wrong finger 0/6 accepted, all eleven TLS handshakes on first attempt. Native S4 better than rel71.30 with external restart (12.1s).
- Serious transport regression: 7/10 accepted sessions FDT-down gets only 1/2 replies, ACK=1. Valid positive score is retained by rel71.74 postauth handling, but next Claim spends3.5–6.5s rebuilding MCU/TLS versus normal FAST_READY51–85ms. One pre-image failure required second pose. Two GET_IMAGE no-ACK retries, 14 FDT retry probes, pacing100→300%. Rel71.30 ~50 sessions had no desync, but it lacked strict two-reply proof and could mask queued responses.

## Narrow experiment and risk boundary
0x32 FDT-down is expected to return an ACK followed by an independent FDT/touch data frame. Current strict proof allows 100ms after first reply. Maybe the second reply is 100–300ms late on 2020, OR it never arrives (device mode difference). Do not conclude before A/B.

Opt-in environment GXFP_FDT_DOWN_SECOND_REPLY_WINDOW_QA_MS permits exactly 200 or 300. Default unset and invalid values retain 100ms. Only opcode0x32 after first reply is affected. Required successful ACK+two replies, TLS-GCM, existing drain, full transport recovery after incomplete proof, matcher threshold7, image-quality gates, cancellation, PAM/password fallback remain unchanged. No retry of already acknowledged command, no extra prewarm service. FDT_DOWN_DRAIN logs counts, ACK, time from first→second reply or timeout, configured budget; NO biometric/PMK/PSK content.

If the second never arrives, the extra 100/200ms per failing command is a regression. Stop the experiment and inspect target FDT mode and reply contract instead of increasing timeout again. If 200ms receives a second reply, compare 20–30 repeated LOCK sessions and at least one natural s2idle/S4 cycle on 2020, exact first-pose rate, next Claim→READY delay, desync frequency, bad image/TLS errors and wrong-finger rejection. Preserve original rel71.74 rollback and don't touch Pegasus.

Other-sensor parity source https://github.com/berkekbgz/libfprint-goodix-spi/blob/main/re/PARITY.md warns ACK and subsequent data may share one IRQ-high window; use exact-length reads and do not skip replies. Not proof of this ST411 timeout cause.

Test executable C option helper 5 cases (100/200/300, invalid, protocol proof retained), full native make verify; source not production installed, draft PR only until hardware data.
