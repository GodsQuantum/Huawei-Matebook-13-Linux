# QA44 — warm ImageBase refresh stage telemetry

Lab-only draft, based on QA42 commit 3c92fb0. This experiment does not include
the failed QA43 shortcut that reused a stale ImageBase.

## Evidence

The first measured QA42 LOCK (2026-10-30) had FAST_READY 52ms but spent
approximately 836ms between FAST_READY and VERIFY_READY. A warm ImageBase
refresh raced with the user's finger placement, so the acquired background
was correctly discarded and the original background used. The result was
one pose and score13/7. QA43 skipped the warm refresh on a 122-second-old
reference; score fell to pixel-rescued7/7, with an unrelated/undetermined
GET_IMAGE cleanup retry of 1000ms. It was reverted and closed.

## Change

Optional GXFP_WARM_REBASE_STAGE_QA=1 outputs a single metadata-only line at
the end of each gx_warm_validate run, separating FDT-before, image capture
and FDT-after durations, with a finite outcome: before_fdt_fail,
finger_before, warm_image_fail, after_fdt_fail, finger_during, updated.
-1 means a stage did not finish. refresh_accepted is 1 only after the
original double finger-off evidence and memcpy of the new background.

No extra sensor command, change in command order, timeouts, retries,
authentication, TLS-GCM, matcher, quality, enrollment, GPIO, PAM or service.
The flag is off by default. No biometric/secret bytes are emitted.

## Safety and next gate

CT700 full make verify plus metadata safety test and cross-platform CI.
There is no performance claim or hardware certification. Keep QA42 installed
on Pegasus until sufficient human first-touch LOCK measurements establish
the proportion of wasted warm refreshes and distinguish FDT retry costs from
postauth GET_IMAGE cleanup delays. Do not blindly shorten the ACK timeout or
reintroduce the failed QA43 stale-background shortcut.
