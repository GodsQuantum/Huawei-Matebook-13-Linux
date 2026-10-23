# Pegasus rel71.55 — current package boundary and hardware gates (23 Oct 2026)

STATUS: SOFTWARE PASS; HARDWARE NOT CERTIFIED; DO NOT MERGE INTO MAIN.

Current observed installed package: libfprint-goodix51a0
1.94.100.goodix51a0-71.55 (installed 2026-10-23 11:44:05 +02:00).
Kernel observed: 7.3.0-2-cachyos. Plasma Login Manager QA 6.8.1-1
is running and password and fingerprint PAM are separate.

Five actual deep S3 cycles on the *same boot* were analyzed; ALL occurred
before the current package installation and do not validate rel71.55.
Earlier branches had warm/cancellable issues and sometimes needed repeated
touches. Never silently carry their results into the current candidate.
The post-install-only read-only report returned 0 S3 records.
No cold boot or reboot with this package has yet been physically tested.
No "first touch" assertion can be derived from logs alone.

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
