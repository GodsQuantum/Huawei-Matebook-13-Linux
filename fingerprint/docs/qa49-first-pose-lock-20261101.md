# QA49 first natural LOCK, one physical pose — 2026-11-01 03:10 CET

User confirmed: **one physical placement**. Source: actual fprintd unit journal on Pegasus,
2026-11-01 03:09:59–03:10:01; no biometric raw pixels or templates stored.

Installed package QA49-1, opt-in GXFP_FIRST_POLL_HELD_QA=1, KDE prompt-free
correction active and fprintd healthy. At 03:09:59.266644 **FAST_READY
52ms**, FDT **touch=0x00** (finger absent). ImageBase was **1704255ms**
old. During attempted warm background capture a finger landed; the
background was correctly **discarded**, FDT mean208 at03:10:00.100770.
VERIFY READY 03:10:00.106255. DETECTED_HOLD 03:10:00.468928
(READY→DETECT **362.673ms**, 6 zones). FIRST_HELD_POLL_QA message is
**ABSENT**: guard did not arm because touch was absent at FAST_READY.
This case does not test the intended fast-path.

Verified strict 0x32 typed FDT replies 61ms and76ms. First and only
usable image: frame591ms, sift6ms, match140ms (total739ms). Primary
SIGFM score4/7; already-shipped strict PIXEL_RESCUE (view15,
inliers3, overlap2465, ZNCC789/1000, agreement873/1000) produced
effective 7/7 without a second image. Cleanup587ms OK (normal
commands0x34/0x20/0x50/0x32). Positive terminal at
03:10:01.796703, DETECT→terminal **1327.775ms**.

**A/B vs QA42+KDE47 02:04** (different background age and FDT timing,
not controlled): READY→DETECT 361.303ms, frame574ms, match122ms,
cleanup566ms, DETECT→terminal1269.654ms. QA49 first-finger
PASS but **no latency improvement**: READY→DETECT +1.370ms,
frame +17ms, match +18ms, cleanup +21ms, DETECT→positive +58.121ms.
No evidence that QA49 caused the differences. It did not trigger.
QA42 record LOCK first-touch DETECT→terminal1181ms still stands.

Possible next optimization (NOT implemented in QA49): the warm
ImageBase refresh already detected a live finger after claim, at
03:10:00.100770 before VERIFY READY. A guarded one-shot hint can be
set from that verified touch too, to catch the usual user gesture.
Do not change threshold7, TLS security, accepted clean reference,
or repeat 1ms polling. Only deploy after reviewing prior QA46
event-loop failure and with hardware A/B.
