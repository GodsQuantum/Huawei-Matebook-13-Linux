# QA60: IRQ level evidence for missing FDT data (disabled by default)

Pegasus 2026-11-03 10:27 true deep S3 had correct native TLS/ImageBase prewarm but genuine biometric scores3/7,4/7,5/7,3/7 (threshold7). Three 0x32 FDT commands returned ACK without separate typed data: 2 postcapture, 1 precapture. QA58 extra 50ms postNAV did not prevent failures; QA56 200ms ACK->report window had not recovered missing reports. Independent 2020 ST411 report found distinct 0x32 data after ACK when present (typical gap61–88ms).

QA60 diagnostic variable GXFP_FDT_IRQ_DIAG_QA=1 adds ONE read-only GPIO48 IRQ level snapshot ONLY on a 0x32 ACK-only event at the preexisting timeout, reporting current IRQ level, count of observed GPIO-high wakeups since ACK, silence polls and milliseconds since ACK. Default off. No SPI request, retry, extra delay, packet data, fingerprint frame, template or secrets. Required two valid typed frames, strict TLS and matcher score7 stay unchanged.

Interpretation: level0 and no observed IRQ cannot prove MCU never produced the frame, but suggests no pending IRQ when checked. Level1 or an observed IRQ without typed data suggests investigate read/framing. This is measurement, NOT a fix nor evidence of stable S3/S4/negative-finger acceptance. Needs actual deep S3 hardware A/B before promotion.
