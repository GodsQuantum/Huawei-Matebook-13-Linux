# KScreenLocker S3 fingerprint PAM recovery (Plasma 6.7.5)

Local package: 6.7.5-1.5.

The patch keeps KDE bug 481808 / MR !340 semantics: suspend is not treated as
a password authentication failure.

Runtime logs proved that the previous resume hook could log a rearm while
fprintd received zero requests. Upstream PamWorker ignores tryUnlock while an
old pam_authenticate is still active and latches unavailable after
PAM_AUTHINFO_UNAVAIL.

On resume this package leaves the interactive password worker untouched and
restarts only noninteractive authenticators. A stale fingerprint PAM
conversation is cancelled and allowed to unwind. Real authentication failures
still respect PAM fail-delay. A stale PAM_AUTHINFO_UNAVAIL result is treated as
a hardware-availability boundary instead: its unavailable latch and inherited
next-attempt delay are cleared before one fresh fingerprint authentication is
started. This removes the roughly four-second penalty observed on successful
resume recovery without weakening password or biometric failure throttling.

There is no polling heartbeat, periodic timer, external sleep hook, runtime-PM
manipulation, or Goodix matcher/enrollment change.

## Plasma 6.8.3 native readiness candidate

[The separate 6.8.3-1.2 recipe](native-readiness-6.8.3/PKGBUILD) targets the
6.8.3 worker/greeter protocol. The initial native lock request survives worker
startup. Cancellation discards the request, and concurrent theme attempts share
one PAM conversation. The greeter arms the workers once the lock is effective and a completed
view can receive the result. Removing the final usable view cancels the
conversation and discards outstanding confirmation. A restored view starts
a fresh conversation. Unlock confirmation remains with the theme.

In Regular mode, only the implicit fingerprint worker retries a slow
`PAM_AUTHINFO_UNAVAIL` while the lock still requests authentication. KDE's
existing worker threshold (250 ms) classifies fast unavailability, and the
existing failure counter disables a reader after repeated closely spaced
failures. Slow recurring hardware failures are indistinguishable from reader
timeouts through this PAM result and may keep retrying until the lock stops.
A normal password rejection or fingerprint mismatch keeps its existing policy.

The optional package builds from the signed KDE release and ships the three
Arch vendor PAM files. Its `check()` uses separate test service names, an
isolated D-Bus session, private configuration/state and the built worker under
pam_wrapper. These tests cover lifecycle behavior. Real lock and resume
performance requires physical fingerprint trials.

The 6.7.5 recipe above remains the historical installer target. The 6.8.3 recipe
must be selected explicitly for a matching installation.

### Run the native regressions

From `native-readiness-6.8.3/`, run `makepkg --check` in an Arch build
environment with the recipe's build and check dependencies. The recipe applies
the three patches to the signed 6.8.3 source. Its separate check build selects
`test_password`, `test_fingerprint` and `test_smartcard`. The test aborts if
the worker resolves outside the build tree or pam_wrapper is missing. The
production build uses the vendor service names.

The `ksmserver-pamReadinessTest` CTest entry exercises 14 cases:

| Behavior | Qt test cases |
| --- | --- |
| One start before worker readiness, cancellation before readiness | `earlyStartOnce`, `cancelBeforeReady` |
| Duplicate request at a prompt, cancellation and an explicit fresh start | `duplicateStartDuringPrompt`, `stopWhilePrompting` |
| Password succeeds with fingerprint waiting for input or timing out | `passwordWhileFingerprintPending`, `passwordSuccessDuringFingerprintTimeout` |
| Stop or grace during deferred worker creation | `deferredCreationAfterStop`, `graceBlocksDeferredStart` |
| Slow fingerprint timeout retries, rapid repeat failures trip the circuit breaker, explicit stop cancels the retry | `fingerprintTimeoutRearms`, `stopDuringFingerprintRetry` |
| Switching away from Regular suppresses the implicit retry, fast unavailability leaves password usable | `switchMethodDuringFingerprintTimeout`, `unavailableFingerprintStaysIdle` |
| Grace cancels a pending fingerprint after a rejected password left the manager idle | `graceAfterPasswordFailure` |
| A new QML view receives a fresh success after first/last-view transitions and removal of a prior confirmation | `viewLifecycle` |

The view test uses real Qt Quick views with offscreen rendering and a minimal
QML success handler. The PAM results come from private test services. Monitor
hotplug on a compositor, physical finger recognition, hardware timing and deep
suspend/resume still require separate trials.
