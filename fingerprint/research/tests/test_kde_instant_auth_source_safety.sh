#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/../.." && pwd)"
h="$root/integration/kde-lockscreen/gxfp51a0-kde-lockscreen-integrate"
hook="$root/integration/kde-lockscreen/90-gxfp51a0-kde-lockscreen.hook"

grep -Fq 'GXFP51A0 bounded-lockscreen-auth-retry fingerprint integration v11' "$h"
grep -Fq 'rewrite_v9_to_v10' "$h"
grep -Fq 'apply_v11' "$h"
grep -Fq 'rewrite_v10_to_v11' "$h"
grep -Fq 'Target = plasma-desktop' "$hook"
grep -Fq -- '--apply' "$hook"

tmp="$(mktemp)"
stock="$(mktemp)"
trap 'rm -f "$tmp" "$stock"' EXIT

cat >"$stock" <<'QML'
MouseArea {
        id: lockScreenRoot
        property bool uiVisible: false
        onUiVisibleChanged: {
            if (uiVisible) {
                Window.window.requestActivate();
            }
            authenticator.startAuthenticating();
        }
        onBlockUIChanged: {
        }
        Component.onCompleted: launchAnimation.start();
}
QML
cp "$stock" "$tmp"

# Stock -> v11: QML contains a bounded retry for KDE PAM worker readiness.
GXFP51A0_KDE_LOCKSCREEN_QML="$tmp" bash "$h" --apply
GXFP51A0_KDE_LOCKSCREEN_QML="$tmp" bash "$h" --check
grep -Fq 'GXFP51A0 bounded-lockscreen-auth-retry fingerprint integration v11' "$tmp"
grep -Fq 'authenticator.unlocked || attempts >= 16' "$tmp"
grep -Fq 'interval: 500' "$tmp"
grep -Fq 'id: gxfp51a0StartupAuthTimer' "$tmp"
grep -Fq 'if (lockScreenRoot.Window.window && !authenticator.unlocked)' "$tmp"
grep -A4 -F 'GXFP51A0 bounded-lockscreen-auth-retry fingerprint integration v11' "$tmp" |
  grep -Fq 'gxfp51a0StartupAuthTimer.start();'
! grep -A4 -F 'GXFP51A0 bounded-lockscreen-auth-retry fingerprint integration v11' "$tmp" |
  grep -Fq 'authenticator.startAuthenticating();'
! grep -Fq 'GXFP51A0 resume rearm BEGIN' "$tmp"
! grep -Fq 'function onResumingFromSuspend()' "$tmp"
! grep -Fq 'gxfp51a0ResumeRearmPending' "$tmp"
! grep -Fq 'gxfp51a0ResumeRearmTimer' "$tmp"
! grep -Fq 'onLoginFailedDelayStarted' "$tmp"

GXFP51A0_KDE_LOCKSCREEN_QML="$tmp" bash "$h" --remove
cmp -s "$tmp" "$stock"

# Existing v10 deployment -> v11 migration and idempotence.
cp "$stock" "$tmp"
GXFP51A0_KDE_LOCKSCREEN_QML="$tmp" bash "$h" --apply
python3 - "$tmp" <<'PYMOD'
from pathlib import Path
import sys
p=Path(sys.argv[1])
s=p.read_text().replace("GXFP51A0 bounded-lockscreen-auth-retry fingerprint integration v11", "GXFP51A0 cpp-lifecycle-auth fingerprint integration v10", 1)
start=s.index("        // GXFP51A0 window-ready startup timer BEGIN")
end=s.index("        // GXFP51A0 window-ready startup timer END",start)
old="""        // GXFP51A0 window-ready startup timer BEGIN
        Timer {
            id: gxfp51a0StartupAuthTimer
            interval: 25
            repeat: true
            triggeredOnStart: true
            property int attempts: 0
            onTriggered: {
                attempts++;
                if (lockScreenRoot.Window.window) {
                    stop();
                    lockScreenRoot.uiVisible = true;
                } else if (attempts >= 80) {
                    stop();
                    console.warn("GXFP51A0: lockscreen Window not ready for fingerprint startup");
                }
            }
            onRunningChanged: {
                if (running) {
                    attempts = 0;
                }
            }
        }
"""
s=s[:start]+old+s[end:]
p.write_text(s)
PYMOD
GXFP51A0_KDE_LOCKSCREEN_QML="$tmp" bash "$h" --apply
GXFP51A0_KDE_LOCKSCREEN_QML="$tmp" bash "$h" --check
migrated_sha=$(sha256sum "$tmp" | cut -d' ' -f1)
GXFP51A0_KDE_LOCKSCREEN_QML="$tmp" bash "$h" --apply
test "$migrated_sha" = "$(sha256sum "$tmp" | cut -d' ' -f1)"
GXFP51A0_KDE_LOCKSCREEN_QML="$tmp" bash "$h" --remove
cmp -s "$tmp" "$stock"

# v9 -> v11: remove only the QML resume block after the automatic upgrade.
cp "$stock" "$tmp"
GXFP51A0_KDE_LOCKSCREEN_QML="$tmp" bash "$h" --apply
python3 - "$tmp" <<'PY'
from pathlib import Path
import sys
p=Path(sys.argv[1])
s=p.read_text()
s=s.replace("GXFP51A0 bounded-lockscreen-auth-retry fingerprint integration v11",
            "GXFP51A0 native-S3-continuity fingerprint integration v9", 1)
needle="        // GXFP51A0 window-ready startup timer END\n"
resume="""        // GXFP51A0 resume rearm BEGIN
        Connections {
            target: sessionManagement
            function onResumingFromSuspend() {
                gxfp51a0StartupAuthTimer.restart();
                authenticator.startAuthenticating();
            }
        }
        // GXFP51A0 resume rearm END
"""
s=s.replace(needle, needle+resume, 1)
p.write_text(s)
PY
GXFP51A0_KDE_LOCKSCREEN_QML="$tmp" bash "$h" --apply
GXFP51A0_KDE_LOCKSCREEN_QML="$tmp" bash "$h" --check
! grep -Fq 'GXFP51A0 resume rearm BEGIN' "$tmp"

echo 'test_kde_instant_auth_source_safety: OK (KDE startup auth retries bounded; native S3 lifecycle unchanged)'
