# GXFP51A0 rel71.69: Hall of Fame synthesis and new 50us CS-setup idea (2026-10-27)

## Actual hardware ranking
- Fastest measured genuine 1-pose S3: rel71.55, wake-to-match **6.951s**, prewarm4.254s, but only **2/3 S3 first-touch successes**.
- Fastest measured 1-pose LOCK: rel71.55, KDE-to-success **3.488s**, finger-to-success **1.711s**.
- Most repeated historical normal LOCK: rel71.30 **18/20 first-touch**, 20/20 by second, warm FAST_READY82-84ms, **S3/BOOT not validated**.
- Best historical targeted hard SPI recovery: rel71.24 (rebind before device enumeration; S3 mixed, not a universal fix).
- Current live rel71.67: two first-touch LOCKs 5.523/4.757s, single natural S3 success wake+13.129s, user ~2 presses with attribution uncertain. 
- Rejected rel71.68: LOCK one 2-press event (first 4/7 reject then 12/7), one 1-press 7/7 PIXEL_RESCUE; natural S3 no unlock despite many presses, repeated TLS/digest failures and GET_IMAGE ACK with **zero IRQs in 200 polls**, never PREWARM_READY, gave up after 39.569s. Rollback to checksum-pinned rel71.67 and PR#30 closed. All scores retain threshold7.
No single release is certified across LOCK/S3/BOOT/REBOOT; do not combine isolated records into a fake release win.

## Recent upstream source comparisons
- **OctoDevStudios/MateBookFP**, same GXFP51A0 macOS port: https://github.com/OctoDevStudios/MateBookFP/blob/main/docs/FINGERPRINT.md . Reports CS assertion then **50 microseconds before first SPI clock** eliminated TLS digest and IRQ misses, and a separate macOS LPSS D0 wake treatment. Evidence is on macOS, NOT measured on Pegasus Linux. Never copy its password-storing lockscreen workaround to KDE.
- **Sigfrodr/libfprint-goodixtls issue#6**: https://github.com/Sigfrodr/libfprint-goodixtls/issues/6 . On sibling 5187 / 51A7 sensor, repeat TLS handshakes eventually wedge MCU; SPI controller sends full transfers but receives all-zero headers; reset pulses do not revive sensor. **Reactive** spidev unbind/rebind is effective, **proactive** per-auth rebind worsens reliability. Progressive shortened TLS receive-window tweak was retracted after real-world use; do not copy it. Similar symptoms on 51A0 are a HYPOTHESIS until SPI/MISO trace proves them.
- **Sigfrodr matcher**: https://github.com/Sigfrodr/libfprint-goodixtls . Max-of-multiple scored captures from a held finger admitted an impostor. Do not lower threshold7 or retry scored frames; acquire new enrollment views only with safe higher-confidence gates.
- **szlukabence** reference: https://github.com/szlukabence/goodix-fingerprint-spi-linux . Preserve ST411/ChicagoHS SPI mode0, SPI_CS_HIGH, PSK GCM, FDT and full 0x20 capture sequence.
- **matifema/gxfp51a0-linux**: https://github.com/matifema/gxfp51a0-linux is an installer/diagnostic around our native driver, not another independent core.
- **PeshalaDilshan/OpenGoodixSPI**: https://github.com/PeshalaDilshan/OpenGoodixSPI is a kernel probe skeleton lacking working 51A0 image/biometric integration, not a replacement.
- Linux SPI UAPI https://github.com/torvalds/linux/blob/master/include/uapi/linux/spi/spidev.h . spi_ioc_transfer.delay_usecs applies AFTER each transfer, not before first clock; kernel has spi_device.cs_setup but spidev does not expose direct setup ioctl.

## Experimental rel71.69 implementation (code only)
- Base is rel71.67, not failed rel71.68. Default SPI uses original SPI_IOC_MESSAGE(1) with same payload, no behavior change. Opt-in GXFP_SPI_CS_SETUP_QA=1 uses SPI_IOC_MESSAGE(2) with a ZERO-LENGTH first transfer, delay_usecs=50, cs_change=0, followed by original unchanged payload. Helper is shared for runtime SPI calls; no custom service, no kernel/PMK/GPU/PAM change.
- Synthetic native C test wraps ioctl (NEVER touches actual /dev/spidev) verifying default 1-transfer, opt-in 2-transfer, descriptor delay/CS flag and TX/RX, malformed parameters; test RED before, GREEN after. Full libfprint make verify passes. No biometric captures or key logs.
- CRITICAL UNKNOWN: pxa2xx controller may ignore a zero-length transfer or toggle CS; without ftrace/logic analyzer, this cannot be certified to create a real 50us CS-to-clock setup. Never enable QA on production before validating the actual controller behavior with a bounded hardware test and rollback. No score threshold, TLS checks, enrollment or other biometric security relaxed.
- Future only-when-wedged spidev recovery needs detailed idle/worker, sysfs privilege and reenumeration review. Never rebind on every LOCK or use the retracted recv-window patch.

## Acceptance
Five consecutive first-touch LOCK, three true S3 (including mid-verify), three cold BOOT, three REBOOT, unknown finger rejected, password works, and no get-image stall. Every claim sourced from journal and human finger count, not CI alone. Live Pegasus remains rel71.67 with original enrolled right index.

## 2026-10-27 00:48 CET — rel71.69 SOFTWARE GATES COMPLETE; HARDWARE ELECTRICAL GATE OPEN
Source commit 2ff110c GitHub PR#31 has 12 of 12 completed PASS CI: native libfprint driver/source regressions, baseline, KDE dual PAM and native Arch integration, Alpine, Arch, Debian, Fedora, openSUSE, Ubuntu24.04/26.04, public privacy scan. No kernel or sensor access in these CI jobs. Independent clean Arch base-devel container built libfprint-goodix51a0 1.94.100.goodix51a0-71.69 package SHA256 **9cf6ed809329e30ae356b90da65b764a61df127c137f131861b751a3e4145dad**, copy pinned solely in LXC700 DEV temporary package archive, staging directory removed. **Not transferred or installed on Pegasus.** Native shared SPI helper's mock ioctl lab test passes including default off and opt-in TX/RX, but real pxa2xx hardware can ignore zero-length xfers. Strict experiment remains disabled and unproven; cannot claim 50us CS setup occurred electrically. Keep rel71.67 production-reference baseline on Pegasus and two pinned rollback packages. PR#31 remains DRAFT and merge blocked pending CS edge measurement and genuine user-attested hardware tests.
