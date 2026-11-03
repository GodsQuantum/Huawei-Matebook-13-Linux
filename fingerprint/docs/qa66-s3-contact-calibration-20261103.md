# QA66 — reuse previously verified clean S3 reference during calibration

Exact baseline QA63 main 309978d7c521faf016adab068042de84514880ff.
QA65 300ms reset change intentionally not carried forward: QA63 timing remains.
Field root cause QA65 S3 second test at 23:49:45: full touch 0x3f while
calibration awaited finger-off; 8s lost before discarding touched background
and restoring saved clean RAM image at 23:49:53.

QA66 opt-in GXFP_S3_CALIB_CONTACT_REUSE_QA=1 applies solely to two native
S3 finger-off waits. With saved valid clean RAM reference and full 6/6
finger detected, re-prove the original age-bounded S3 reference with a
SECOND independent live FDT before leaving calibration early. Failed proof,
partial contact, absent or stale saved reference all retain strict clean
dwell and contaminated-image rejection unchanged; never accept touched
ImageBase. TLS/PMK and encrypted image+quality+matcher7 unchanged. No
GPIO edge mode, firmware, re-enrollment or external services.

Native tests and real Pegasus trials must pass before promotion to main.
QA63 package rollback SHA256
553c9fc2611d86f345eca1ad96f6ada501e9a4ea98371267a9ce7f1042f41b3c.
