# QA42 — ordinary LOCK failure before mouse reveals KDE prompt, 31 October

Operator: multiple physical finger placements did not unlock the screen;
authentication worked after the mouse was moved and the KDE prompt appeared.
Exact number of physical placements and their individual timestamps unknown.
Count this as a first-touch failure, not as a successful single-press LOCK.

The fprintd log only contains a Verify session starting after the operator
made the prompt visible:

- 23:43:37.966120 FAST_READY 83ms, FDT touch0x00, idle351/floor327
- 23:43:38.001134 old ImageBase age1546397ms vs required20000ms; refresh
- 23:43:39.235518 WARM_REBASE accepted fresh ImageBase in 1234ms
- 23:43:39.241345 VERIFY READY
- 23:43:39.438447 DETECTED_HOLD, touch0x3f zones6 mean208 drop144
- 23:43:40.435241 capture image1 score16/7
- 23:43:41.196795 positive terminal, 1758ms from DETECTED_HOLD.

KDE Plasma6.8.2 installed QML at the time had uiVisible=false by default
and only started the authenticator from onUiVisibleChanged, which is
triggered by mouse/keyboard activity. A previously installed version-guarded
pacman helper for prompt-free authentication refused the new 6.8.2 vendor
QML, so the stock file was active. This explains why moving the mouse
started fprintd, without implicating the matcher. The 1.234s ImageBase
refresh was additional delay after auth was started.

The source fix is separate draft PR #47, targeting the KDE UI rather than
changing QA42 or bypassing TLS, ImageBase freshness, threshold7, or PAM.
The fix was applied later, after this failed trial; no physical post-fix
LOCK result is yet available. Do not label this trial a successful
first-touch authentication or conflate 1758ms with first physical contact.
