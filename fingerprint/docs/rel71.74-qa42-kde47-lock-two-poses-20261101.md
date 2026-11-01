# QA42 + KDE47: 2 physical poses on ordinary LOCK, 2026-11-01 01:48 CET

Operator: first physical finger placement failed, second succeeded.
Biometric trace under *verified* record QA42 binary SHA256
8adcaa2d5c18b55045967d0574ca6920fb28687cff96b58d2494c9f041d33131.
Pacman 32/32, fprintd active, KDE prompt-free --check PASS.
QA48 already removed and its 66-dropin absent.

FIRST PRESS: FAST_READY at 01:48:01.135816 in 240ms, touch=0x3f
and mean215. ImageBase age 685268ms against freshness limit20000ms;
WARM_REBASE deferred because finger already down, so previous clean
background retained, never polluted. VERIFY READY 01:48:01.222261.
DETECTED_HOLD 01:48:01.584950, 6/6 FDT zones; strict 0x32 ACK and
data reported in 62/61ms. Image frame571ms, score5/7 at
01:48:02.296875. Cleanup560ms OK. THIS WAS A REAL BIOMETRIC
NO-MATCH, NOT A MISSED FINGER DETECTION OR KDE UI WAKE-UP.

SECOND PRESS: finger lifted by 01:48:05.521527. ImageBase refresh
was attempted, but the finger arrived during capture and the new
background was safely discarded at 01:48:06.354155.
VERIFY READY 01:48:06.359644, DETECTED_HOLD 01:48:06.732904,
0x32 replies validated in62/61ms. Image frame571ms, original
score5, strict PIXEL_RESCUE with inliers4, overlap2953,
ZNCC820 and agreement883 allowed score7/7 at01:48:07.468686.
Cleanup566ms OK; success01:48:08.035358 =1302.454ms measured
from second DETECTED_HOLD to positive. NOT A FIRST-POSE RECORD.

Underlying cause remains to be determined from further controlled
evidence. Stale ImageBase is a plausible cause of degraded matching
but finger placement differences and ambient conditions confound it.
Do not reduce threshold7, turn one physical press into best-of-N
score-conditioned captures, store biometric images, or claim 100%
first-touch reliability. New KDE47 cohort now includes one earlier
first-touch success and one failure.
