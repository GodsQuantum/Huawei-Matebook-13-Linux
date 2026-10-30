# QA46 — asynchronous FDT-down IRQ instead of synchronous two-reply drain

State: lab candidate, based on hardware QA42 commit 3c92fb0, not QA43-45.
NOT yet a certified fix. One real human LOCK is required, and reliability
requires multiple trials of LOCK, genuine S3 and S4.

## Hardware comparison

- QA42 01:44: contact during warm ImageBase, background DISCARDED; manual
  FDT then saw six zones mean231/drop122 and verified score13/7 in 1.181s.
- Other QA42/44 LOCK: accepted warm ImageBase, then idle FDT mean353-355,
  drop0-1 for about five seconds while user tried several finger placements.
- QA45 attempted to arm FDT-down, but required BOTH ACK and second report
  within 100ms under finger-OFF. Real ACK=1/data=0, sensor init failed.
- Upstream goodix-fp-dump goodix.py waits for the second 0x32 report
  with read(timeout=None) after command ACK. Its driver_51x0.py arms
  0x36 then 0x32 and waits for a finger. The szlukabence Goodix IMAGING
  protocol document corroborates this sequencing.

## QA46 opt-in

GXFP_FDT_ASYNC_EVENT_QA=1 is OFF by default, and is used ONLY following
a fresh warm ImageBase accepted after double finger-off proof:
- WakeupMCU, live FDT manual proof; if finger already landed, keep QA42
  manual detection rather than arming new finger-down;
- settle according to the existing capture gap and issue the same 0x32
  packet as the previously validated capture recipe; validate ACK only;
- WAIT_ON: existing 100ms GLib timeout checks IRQ, no 0x36 manual probe
  while a 0x32 event is outstanding, and parses later complete 20-byte
  FDT event with tag0x32/0x36, length and checksum before contact gate;
- only proven contact proceeds to the same TLS-GCM finger image and
  unchanged strict score7 matcher;
- cancellation, malformed IRQ, no-contact event and bounded overall
  timeout invalidate outstanding warm state (future Claim cold resets).
  No 0x32 replay and no unsupported immediate second-reply timeout.

No new services or timers, no changes to PAM, KDE, enrollment, biometric
security threshold, TLS decryption or required post-capture cleanup.

## Test limits

Synthetic FDT C packet tests, static source guard and full native
libfprint compile plus regression suite provide SOFTWARE validation only.
They do not emulate electrical GPIO48 signals. A real first-finger
hardware trial is necessary before any merge or production promotion.
Rollback package remains byte-identical QA42, with saved SHA256.
