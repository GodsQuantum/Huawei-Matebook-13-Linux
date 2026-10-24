# rel71.58 S3 stale ImageBase rebase (experimental, opt-in only)

QA `GXFP_S3_STALE_REBASE_QA=1` requires verified clean finger-OFF
ImageBase before rearming a new scored verification after a low-score
press with a stale (>20s) background. Prevents the 2026-10-24
19:06:12–29 repeat 2/7,2/7,2/7 after early finger on S3 wake.

Kept completely separate from rejected rel71.56 S3 fast preflight.
No change to matcher threshold=7, security, password fallback or the
default rel71.55 retry. New code uses existing cancellation-aware
FDT clear-stable checks, 650ms dwell, bounded 4000ms wait per pass,
and at most two full clean-background captures; no GPIO resets.
Any contamination requires a clean retry; no fresh reference => no
another scored Verify using stale background. A transport error
retains the established native session-recovery pathway. New runtime
behavior MUST be certified on Pegasus hardware with safe rel71.55
rollback available. This is not production-ready.
## Pegasus rel71.58 staged QA on 2026-10-24

Built with CT700 native verification PASS and Arch Linux Docker package,
SHA256 b50e73cb7b54a4c750616b27ceb14ccbcf561351f1acac984eddd920fb55de15.
All 12 PR #19 CI checks passed. Native pacman -U on Pegasus with
GXFP_S3_STALE_REBASE_QA=1 drop-in, Snapper 1846/1847. No reboot, kernel,
KDE, PAM or enrolled prints changed. 32 installed files verified 0 altered.
fprintd active; first post-package native prewarm had 2 TLS failures
then startup fallback. One controlled native daemon restart at 19:28:43
completed by 19:28:58, no terminal prewarm error. A TLS handshake retry
still appeared, so hardware confirmation is pending. fprintd-list at
19:29:24 finds 1 sensor and enrolled right-index finger.
Pinned rel71.55 SHA256 f1432414c19fd13bf5abbde9b8b713b3a606e18af89dcdb85d0c6334e5a14deb
remains at /var/lib/gxfp51a0-qa-rollback/rel71.55.pkg.tar.zst.
Rollback: remove /etc/systemd/system/fprintd.service.d/80-gxfp51a0-s3-stale-rebase-qa.conf,
systemctl daemon-reload, native pacman -U pinned rel71.55; no reboot.
No first-touch or S3 cert granted yet. Next test: LOCK one physical press.

## LOCK 2026-10-24 19:33:31 CEST — PASS first physical pose

Owner-reported one placement, subjective speed "moyen". Actual Pegasus
libfprint rel71.58, fprintd active with GXFP_S3_STALE_REBASE_QA=1.
KDE greeter PAM worker 19:33:29.345391; FAST_READY 19:33:30.957964
(293ms internal; not greeter end-to-end). ImageBase aged 272,585ms;
refresh deferred because finger already present. First physical press
DETECTED_HOLD 19:33:31.450355, six FDT zones. One scored frame 11/7
at 19:33:32.462908; cleanup finished 19:33:33.222172;
positive terminal without release wait at 19:33:33.352171.
Measurements: finger->terminal 1902ms; KDE greeter->terminal 4007ms;
score->terminal 889ms. No authentication regression observed, but
latency is NOT faster: baseline rel71.55 post-rollback median was
1739ms over 3 successful first-touch LOCKs versus 1902ms here
(+9.4% based on a single trial, inconclusive).
Real deep S3 and cold boot with rel71.58 remain untested.

Canonical multi-version ledger and records: [HALL_OF_FAME.md](HALL_OF_FAME.md)
and [HARDWARE_TRIALS.csv](HARDWARE_TRIALS.csv). Run the local
gxfp51a0-hall-of-fame.py script with --write whenever evidence changes,
then --check and make verify, and push the CSV, page and primary evidence
together. The script never substitutes journal logs for human placement.
