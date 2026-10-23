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
- Certification tally: reboot 1/3; cold boot 0/3; LOCK 0/5 under this
  candidate; supervised deep S3 0/3. Overall hardware verdict **PENDING**.

Measure only cycles starting after the installed package timestamp:
    python3 fingerprint/scripts/gxfp51a0-hardware-evidence.py --since 2026-10-23T11:44:05+02:00 --json

Rollback verification (read-only) on Pegasus:
    sudo /usr/local/sbin/gxfp51a0-qa-rollback --check
The fallback archives are root-owned at /var/lib/gxfp51a0-qa-rollback/
and independently SHA256-verified. Actual rollback has NOT been executed
on the live session. No reboot/suspend or login-manager restart in QA.

Release requirements: five consecutive 1st-touch LOCK successes,
three owner-supervised real deep-S3 runs (including in-progress verify),
three owner-supervised cold starts, three warm reboots,
wrong-finger rejection, password fallback, no sensor hang or
credential/bypass regression, bounded startup/wake and energy behavior.
Retain strict matcher threshold 7 and original enrollment templates.
Security FAR requires a separate population study. Do not mark this
repository or installed package as hardware-certified without evidence.
