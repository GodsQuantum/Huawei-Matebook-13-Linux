#!/usr/bin/env python3
"""Prompt-free KScreenLocker patcher: safe transform and rollback gates."""
import hashlib
from pathlib import Path
import runpy

root = Path(__file__).resolve().parents[2]
path = root / "integration/kde-lockscreen/gxfp51a0-kde-lock-reapply"
ns = runpy.run_path(str(path))
stock = "\n".join(old for old, _ in ns["PATCHES"]) + "\n" + ns["ORIGINAL"] + "\n"
candidate = ns["build"](stock)
assert ns["verify"](candidate) == hashlib.sha256(stock.encode()).hexdigest()
assert "parent.uiVisible" not in candidate
assert "running: !authenticator.unlocked && !lockScreenUi.biometricSuspendPending" in candidate
assert "function onResumingFromSuspend()" in candidate
assert "authenticator.stopAuthenticating();" in candidate
assert candidate.count("authenticator.startAuthenticating();") == 3
assert candidate.count("Component.onCompleted: {") == 1
assert "lockScreenRoot.uiVisible = true" not in candidate
assert "pam_fprintd" not in candidate and "loginctl" not in candidate
for broken in (candidate, stock.replace("onAboutToSuspend()", "unexpectedSuspend()"),
               stock.replace("parent.uiVisible", "unexpectedVisible")):
    try:
        ns["build"](broken)
    except ValueError:
        pass
    else:
        raise AssertionError("unsafe/unknown vendor QML not rejected")
hook = (root / "integration/kde-lockscreen/90-gxfp51a0-promptfree.hook").read_text()
assert "Target = plasma-desktop" in hook
assert "When = PostTransaction" in hook
assert "gxfp51a0-kde-lock-reapply --apply" in hook
print("KDE_PROMPT_FREE_LIFECYCLE_SOURCE_TEST=PASS")
