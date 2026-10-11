# 2026-10-11: Plasma Login Manager 3.10 deployed, native cold QA on Pegasus

Machine: Pegasus Huawei MateBook 13, GXFP51A0/ST411, CachyOS 7.2.9-2.
Installed libfprint-goodix51a0 rel71.36; stock fprintd active; updated
Plasma Login Manager 6.7.5-3.10 with passive initialization status.
The new greeter displays "Initialisation du lecteur d'empreintes..."
while the independent fingerprint PAM worker is still opening the
hardware. Only the real PAM prompt "Placez votre index droit..." means
that fingerprint VerifyStart is ready. The password worker is ALWAYS
available without waiting for sensor setup. 0 altered installed files
under pacman -Qkk on PLM, libfprint, fprintd. Existing enrollment, PMK,
Bitwarden PIN, Polkit and kernel/mount architecture are preserved.

Three native cold Claims under kernel 7.2.9-2 after deliberately restarting
ONLY stock fprintd; own-user D-Bus Claim (NO fingerprint capture/VerifyStart):
1. 12:43:21-12:43:29: success (), 7.47s, production_ready=1.
2. 12:43:49-12:44:04: success (), 16.20s, accepted GET_IMAGE timeout
   at 12:44:00; driver completed existing detached transport recovery
   and eventually production_ready=1.
3. 12:44:17-12:44:25: success (), 7.63s, production_ready=1.
All 3 successful, but cold prep timing still variable. These only test
device initialization, not biometric authentication. No services, timers,
sleep hooks or standalone recovery helpers added. No computer reboot
or user logout initiated by assistant.

Remaining HARDWARE acceptance gate: user manually chooses KDE Deconnexion,
NOT Super+L, so a fresh greeter starts with patched QML. Confirm passive
initializing message while hardware not ready, then real PAM fingerprint
placement cue; verify enrolled RIGHT INDEX logs in, report poses.
Retain password path. After successful login, test normal Super+L then
real deep S3 again if user chooses. Examine actual scores and timing.
Do not merge/promote GitHub stable main until all physical gates pass.
Known safe driver/package and original templates kept for rollback.
