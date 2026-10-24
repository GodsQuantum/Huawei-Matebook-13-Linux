# Pegasus rel71.55 — current package boundary and hardware gates (23 Oct 2026)

STATUS: SOFTWARE PASS; HARDWARE NOT CERTIFIED; DO NOT MERGE INTO MAIN.

Current observed installed package: libfprint-goodix51a0
1.94.100.goodix51a0-71.55 (installed 2026-10-23 11:44:05 +02:00).
Kernel before reboot: 7.3.0-2-cachyos; after reboot: 7.3.0-3-cachyos.
Plasma Login Manager QA 6.8.1-1
is running and password and fingerprint PAM are separate.

Five actual deep S3 cycles on the *same boot* were analyzed; ALL occurred
before the current package installation and do not validate rel71.55.
Earlier branches had warm/cancellable issues and sometimes needed repeated
touches. Never silently carry their results into the current candidate.
The post-install-only read-only report returned 0 S3 records.
Before the 13:07 reboot on 23 October, no reboot with this build had been
physically tested. The first supervised warm reboot is now recorded below.
No cold power-off/on has been physically tested with this package.
A "first touch" assertion requires a human report; logs alone do not prove it.

## Real hardware evidence — warm reboot trial 1/3 (PASS)

- Date: 2026-10-23, Europe/Paris.
- Human observation: owner explicitly reports **REBOOT: first physical finger placement OK**.
- Actual new kernel boot: 13:07:12 +02:00, kernel 7.3.0-3-cachyos.
- Driver after reboot: libfprint-goodix51a0 1.94.100.goodix51a0-71.55.
- fprintd started 13:07:20 and PLM started 13:07:28. PLM active executable
  no longer points to a deleted (pre-package) image.
- Automatic PLM fingerprint authentication began 13:07:32;
  native FAST_READY took approximately 83 ms. Until the human touch the
  greeter performed 30-second auth windows and re-armed after timeouts,
  retaining password fallback. These timeouts are **not** evidence of
  physical fingers rejected.
- Physical fingerprint contact detected at 13:12:03 (touch 0x38; 3 zones);
  strict enrolled match score 10 against threshold 7, 13:12:04;
  successful terminal and PLM fingerprint authentication at 13:12:05.
  The user session then started successfully.
- Attribution: the human report supports **first-placement success**;
  fprintd and PLM logs independently confirm the successful fingerprint
  and session. This is a warm **reboot**, not a cold power-off boot.
- Reboot 1/3; cold boot 0/3; supervised deep S3 0/3. LOCK outcomes
  are separately recorded below. Hardware verdict **PENDING**.

## LOCK first-touch validation and performance (2026-10-23)

- LOCK #1: **PASS**, human-reported unlock on first physical touch,
  approximately 13:17 Europe/Paris; rel71.55, post-reboot kernel 7.3.0-3.
  fprintd journal: retained context FAST_READY ~83 ms. Full latency must
  not be inferred from that single fast-ready measurement.
- LOCK #2: **FAIL first-touch** (unlock SUCCESS on second touch), owner
  reports this explicitly. Journal confirms at 13:42:50.845+02:00:
  FAST_READY 84 ms with touch already sensed (0x0f) before auth READY.
  Background reference aged 1,852,274 ms (30m52s), versus the 20-second
  freshness limit. Refresh was safely deferred due to finger on sensor,
  not replaced with finger-contaminated background.
  - First placement DETECTED 13:42:51.156, usable scored image
    13:42:52.192 **6 < threshold 7**, completed at 13:42:52.952.
    Finger RELEASED 13:42:54.779.
  - Retry rebase requested 13:42:54.780; background capture discarded
    13:42:55.987 because the next finger landed during calibration.
  - Second placement DETECTED 13:42:56.188, match scored at
    13:42:57.170 **10 >= threshold 7**; terminal auth success
    13:42:58.014.
  - KDE kscreenlocker logged "Worker did not terminate in time, killing
    it" at 13:42:58.023 AFTER terminal biometric success. Audit this
    worker exit timing; do not claim it caused the initial rejection.
  - KDE greeter started 13:42:49.150; fingerprint FAST_READY occurred
    ~1.695 seconds later. The first pose was already present at open.
- **Reliability tally: LOCK attempts 2, first-touch PASS 1 / FAIL 1.
  Consecutive first-touch streak 0/5; reboot PASS 1/3, cold boot 0/3,
  true deep S3 **1/3** after the real Oct 23 cycle below.
  Certification NOT complete.**
- Performance constraints: source explicitly permits multiple frames only
  for frames failing *pre-match quality checks*, and forbids doing a
  score-conditioned best-of-N after a usable subthreshold image. Keep
  this anti-false-accept boundary and matcher threshold 7 intact.
  Candidate engineering work: instrument screen-lock-to-PAM-ready time,
  look at an earlier native KDE fingerprint service activation and a
  genuinely finger-off calibration window; compare 1st-touch reliability,
  not just latency. No risky live change should be made without QA.

## Deep S3 #1 — natural suspend, verified (PASS first physical pose)

- 2026-10-23 14:05:02.096+02:00: Pegasus entered genuine kernel
  `PM: suspend entry (deep)`, not s2idle. Wake/exit 14:11:54.797;
  ~6m53s spent asleep, same rel71.55 install and kernel 7.3.0-3.
- Owner explicitly reports first physical finger placement succeeded after
  the natural sleep; journal independently shows 1/3 DETECTED_HOLD and
  biometric success with score 14 against unchanged threshold 7.
- Idle pre-suspend packet ACK=true and cached clean RAM-only ImageBase
  saved 14:04:57.141. Native post-S3 prewarm started 14:11:54.823,
  used detached GPIO264 short reset 14:11:55.824, reached READY
  14:11:59.077 (4.254s prewarm). Cached full-six-zone finger reference
  accepted only after live FDT verification, then real fingerprint
  matching still required.
- Physical contact detected 14:11:59.675, score 14 at 14:12:00.736,
  terminal auth 14:12:01.748. This does NOT mean subsecond login.
- Independent read-only post-install evidence parser:
  records_examined=1, sleep_packet_ack=true, clean_reference_saved=true,
  native_resume_called=true, prewarm_fallback=false,
  idle_cancellable_assertions=0, matched_score=14.
- Counts as real deep-S3 hardware test 1/3 and first-touch user evidence.
  Need two more, including active Verify suspend. Performance still OPEN.
- Follow-up experimental rel71.56 fast prewarm is a DIFFERENT, opt-in
  branch/PR #17; remains uninstalled and hardware-UNVERIFIED on Pegasus.

Measure only cycles starting after the installed package timestamp:
    python3 fingerprint/scripts/gxfp51a0-hardware-evidence.py --since 2026-10-23T11:44:05+02:00 --json

Rollback verification (read-only) on Pegasus:
    sudo /usr/local/sbin/gxfp51a0-qa-rollback --check
The fallback archives are root-owned at /var/lib/gxfp51a0-qa-rollback/
and independently SHA256-verified. Actual rollback has NOT been executed
on the live session. No reboot/suspend or login-manager restart was initiated remotely in QA;
the reboot above was performed by the owner.

Release requirements: five consecutive 1st-touch LOCK successes,
three owner-supervised real deep-S3 runs (including in-progress verify),
three owner-supervised cold starts, three warm reboots,
wrong-finger rejection, password fallback, no sensor hang or
credential/bypass regression, bounded startup/wake and energy behavior.
Retain strict matcher threshold 7 and original enrollment templates.
Security FAR requires a separate population study. Do not mark this
repository or installed package as hardware-certified without evidence.
## 2026-10-24 post-rollback LOCK #1 — first-touch PASS (rel71.55)

- After the experimental rel71.56 S3 performance regression, root-owned package
  rollback and removal of the QA_FAST override were confirmed. At
  15:21:48+02:00, installed `libfprint-goodix51a0` was rel71.55, fprintd
  active, `Environment=` empty.
- Owner explicitly reports unlock using ONE physical finger placement.
- KDE lock greeter PAM worker initializing at 15:21:08.955; READY later.
  fprintd `FAST_READY` 72ms at 15:21:10.241 (full KDE-to-ready is
  ~1.4 seconds; this 72ms is only fast sensor validation).
- Finger already on sensor at Claim; previous clean reference aged 335690ms
  and refresh deferred to avoid a finger-contaminated background.
- First physical pose detected 15:21:10.733 (FDT 6/6, 0x3f);
  actual image scored **15 against threshold 7** at 15:21:11.633;
  verified terminal success 15:21:12.443. Approx finger-detect to terminal
  1.71 seconds; greeter-PAM initialization to terminal ~3.49 seconds.
- Success confirms usable LOCK authentication and no obvious regression
  after the rollback, not completion of repeated S3/boot certification.
  No kernel changes, no PAM bypass, no lowered score threshold.
## 2026-10-24 post-rollback LOCK #2 — first-touch PASS, perceived latency SLOW

Owner confirmed exactly one physical touch but experienced a delay and
suspected that lifting the finger was required. Verified rel71.55 from
Pegasus journal; no experimental S3 flag.

- KDE worker init 15:24:59.950; fprintd FAST_READY=72ms at
  15:25:01.270. Normal greeter-to-FDT delay ~1.32 seconds.
- Clean ImageBase aged 566709ms (>20s policy). Warm validation
  attempted a new background and discarded it when finger arrived
  at 15:25:02.359, ~1.06 s after refresh began. Old clean
  ImageBase retained without finger contamination.
- First physical press detected 15:25:02.751 with all six FDT zones;
  biometric score **7/7** at 15:25:03.680 (0.929s later).
- Mandatory target cleanup completed at 15:25:04.383, ~0.703s after
  match scored. Positive terminal at 15:25:04.505, 0.122s later.
  Finger detect -> terminal ~1.754s, KDE PAM init -> terminal ~4.555s.
- User **does not need to remove the finger** for a positive verdict.
  `LIFT_NOW` and `verified positive terminal without release wait`
  are logged within microseconds, at 15:25:04.505. The
  cleanup before this remains essential for a synchronized SPI/TLS
  and cannot be casually skipped.
- User first-touch hardware LOCK after rollback: **2 of 2**.
  The S3/cold-boot certification gates remain pending.
- To identify the slow command precisely without risking another
  release regression, next QA branch rel71.57 / PR #18 adds
  opt-in read-only per-step cleanup timings. It is NOT deployed.
