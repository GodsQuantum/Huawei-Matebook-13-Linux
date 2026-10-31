# QA42 + KDE PR47: LOCK with hidden password prompt, 1 November 2026

First hardware acceptance only, not reliability certification. User explicitly
reported LOCK : 1ere pose after the requested test without moving the mouse
or keyboard, while password prompt was hidden. The no-mouse condition is
the operator's report (not independently video-recorded); sensor evidence
is in the driver journals.

## Confirmed fprintd trace (Europe/Paris)

- 00:00:14.691896 FAST_READY 83ms; finger already ON first probe,
  touch0x3f mean240 floor329.
- 00:00:14.712104 ImageBase age995476ms >20000ms. Since finger was already
  present, 00:00:14.796238 WARM_REBASE safely DEFERRED, retaining old
  accepted reference (live mean231 floor340).
- 00:00:14.801739 VERIFY READY, 00:00:14.998207 DETECTED_HOLD touch0x3f
  zones6 mean223 drop130.
- 00:00:16.038694 image1 score14/7, image pipeline1040ms;
  00:00:16.797565 cleanup758ms succeeded; terminal positive
  00:00:16.797743 = 1799.536ms DETECT-to-success. From first FAST_READY
  to terminal positive is 2105.847ms. Neither equals time from first
  finger placement (timestamp not independently measured).
- fprintd active, QA42-1 installed unchanged, pacman-Qkk32/32,
  KDE6.8.2 QML SHA256 be39538eef37212f1e5295b4e6785b737bd7284aaf37623763d9dabac79b4295.

## Comparison and limits

Five previous pre-KDE-patch ordinary QA42 LOCK reports passed 1/5 on
first human finger placement. New patch launches KDE native Authenticator
when the invisible lock UI is created, does not require mouse activity;
keeps KDE's existing heartbeat active during hidden UI, and gates
authentication with native pre-suspend and post-resume events.

The HOF row uses a separate version name rel71.74-qa42+kde47 so the
pre-KDE-patch 1/5 and post-KDE-patch 1/1 remain separate cohorts.
S3/S4, wake power consumption, and long-term first-touch reliability
are NOT yet certified. Do not merge PR47 just on this one success.
