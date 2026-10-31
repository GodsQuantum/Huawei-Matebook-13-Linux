# KDE Plasma 6.8: fingerprint authentication with a hidden lock prompt

Pegasus hardware: GXFP51A0 SPI, libfprint QA42.
Status: targeted KDE integration; physical no-mouse LOCK test required.

## Evidence, 31 October 2026

During the physical LOCK at 23:43 the operator reported several failed finger
placements, then success after moving the mouse to reveal the prompt.
fprintd first logged FAST_READY at 23:43:37.966, accepted a 1234ms
ImageBase refresh, VERIFY READY at 23:43:39.241, DETECTED_HOLD 197ms
later, and positive score16/7 at 23:43:41.196. The driver was not
running biometric verification continuously before this client activation.

The installed KDE 6.8.2 LockScreenUi.qml initializes uiVisible=false and
calls authenticator.startAuthenticating() from onUiVisibleChanged. Moving
the mouse triggers this path. This is a KDE-side activation problem,
not a matching-score problem. KDE Bug 527454 and 528023 describe related
Plasma 6.8 UI/biometric issues; this exact cause is visible in the QML.

## Reuse the native KScreenLocker backend

The existing Pegasus pacman hook and pinned prompt-free QML asset from
October 22 were not applied to the October 30 updated QML: the helper's
vendor_sha was for the earlier KDE file and deliberately refused to
overwrite unknown versions. Do NOT copy the old QML onto Plasma 6.8.2.

The new version-aware helper lives at
fingerprint/integration/kde-lockscreen/gxfp51a0-kde-lock-reapply
and reuses the already-existing system hook, without creating a service,
sleep worker, extra watchdog or altering the driver.

It modifies only the selected authenticating UI lifecycle:
- start the stock authenticator at QML component completion even when the
  prompt is hidden;
- use KDE's existing 1-second heartbeat while the screen remains locked,
  including when the prompt has faded away;
- suspend the heartbeat and stop authenticating when KWorkspace
  SessionManagement emits aboutToSuspend, restart on resumingFromSuspend;
- leave password fallback, PAM, KDE selection, TLS, enrollment and
  fingerprint matching policy unchanged.

The upstream KDE SessionManagement API officially exposes both signals.
An existing September 2026 KDE fingerprint workaround verified prompt
activation but warned that keeping the sensor awake during s2idle could
prevent S0ix sleep and drain a battery. Never bypass the suspend guard.
Pegasus reports deep S3 supported, but native S3 behavior must still be
physically checked before declaring production-ready.

The helper applies only to precisely recognized QML text anchors, lints
the transformed QML with qmllint, makes a SHA256-verified vendor backup
for rollback and atomically replaces the installed file. Unknown future
Plasma layout -> fail safely without touching the vendor QML. The
native pacman hook targets the actual QML owner, plasma-desktop, so a
compatible update can reapply the patch.

No physical effectiveness is established by syntax or source tests alone.
First verify one LOCK without moving the mouse, then natural S3 and LOCK.

On Pegasus, pacman -Qo verified plasma-desktop 6.8.2-1.1 owns the QML;
the existing /etc/pacman.d/hooks/90-gxfp51a0-kde-lock.hook already targets
plasma-desktop and invokes the same helper path. Do not create another hook.
pacman -Qkk plasma-desktop is expected to report precisely ONE altered QML
while the opt-in integration is active; plasma-workspace remains pristine.
