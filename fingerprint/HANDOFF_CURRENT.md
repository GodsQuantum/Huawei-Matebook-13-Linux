## 2026-10-27 ~15:15 CET — FINALIZATION UNDERWAY: reference release rel71.69
User explicitly accepted rel71.69 after one first-touch native LOCK, first-touch after real S4 hibernation, and one first-touch genuine S3. Logs show LOCK 02:04 score7/7 DETECTED→terminal1374ms, post-S4 LOCK14:57 score11/7 DETECTED→terminal1826ms, S3 wake14:58:45.886 PREWARM_READY+5153ms score15/7 DETECTED→terminal2054ms wake→positive8218ms, no image TLS failure. No invented rapid hibernation total (KDE was waiting many minutes for user's finger). User asks freeze, publish main GitHub, final repo installer, sync local/Cloud9/Pegasus, clean, existing apps native auth. Release is REFERENCE for this machine, not blanket multi-hardware/cold-boot full certification. Fingerprint core NEVER changed after successful human trials; only packaging systemd config and portable env now reproduce 3 existing flags S3_FRESH_BG=1 FDT_DOWN_PROOF=1 SPI_CS_SETUP=1, portable release marker71.69, README EN/FR/ZH and trial HOF. Native make verify PASS; CI and reproducible release package build, install on Pegasus, remove obsolete /etc/80 QA override only after package default proven, final installer idempotence and app PAM inventory, public main/release sync and cleanup remain. Keep root password fallback, existing KDE dedicated fingerprint PAM and Polkit custom; do not add sudo/global system-auth PAM without noninteractive/SSH regression proof. No reboot/forced S3. Snapshot/rollback rel71.67+71.64 pinned SHA checks. Canonical Cloud9 repo workspace has mode-only uncommitted modifications on five scripts; preserve scripts and reconcile executable modes before fast-forward, do not overwrite active data.

## CURRENT 2026-10-27 — QA rel71.69 CS50 installed on Pegasus, native kernel SPI trace and sensor startup improved
Real hardware kernel SPI trace confirms CS→data first SCLK proxy6us default vs76us with new zero-len first 50us delay, a single CS assert/deassert, full 4-byte native WakeupMCU executed. No physical pin logic analyzer, but kernel-level pxa2xx trace valid. Same sensor rel71.67 normal fprintd init 7.971s with 10 no-IRQ target ACK misses and pacing300%; rel71.69 after opt-in restart 6.184s with zero target ACK misses and pacing150%, initial no TLS digest failures, one FDT ACK retry. User fingerprint S3/LOCK still UNTESTED on 71.69, do NOT certify performance beyond startup. Pacman rel71.69 installed, Snapper1936/1937, native fprintd PID355020 ACTIVE; flags only S3_FRESH_BG_QA FDT_DOWN_PROOF_QA SPI_CS_SETUP_QA all=1; package Qkk 32/32 intact, right-index enrolled. Package checksum 9cf6ed809329e30ae356b90da65b764a61df127c137f131861b751a3e4145dad, pinned 71.67 08681c32... and 71.64 c3df31e8... rollback verified. CI source12/12 PASS; run trace + temporary transfer files cleaned; ftrace all flags returned 0. No kernel/PMK/PAM/GPU/GPIO112 edits or forced reboot/S3. User next physically tests normal LOCK then genuine natural deep S3 reporting actual poses and speed, if regress revert fast. Source report fingerprint/docs/rel71.69-cs-setup-and-upstream-audit-20261027.md, GitHub PR#31 draft.

## 2026-10-27 00:48 CET — rel71.69 SOFTWARE GATES COMPLETE; HARDWARE ELECTRICAL GATE OPEN
Source commit 2ff110c GitHub PR#31 has 12 of 12 completed PASS CI: native libfprint driver/source regressions, baseline, KDE dual PAM and native Arch integration, Alpine, Arch, Debian, Fedora, openSUSE, Ubuntu24.04/26.04, public privacy scan. No kernel or sensor access in these CI jobs. Independent clean Arch base-devel container built libfprint-goodix51a0 1.94.100.goodix51a0-71.69 package SHA256 **9cf6ed809329e30ae356b90da65b764a61df127c137f131861b751a3e4145dad**, copy pinned solely in LXC700 DEV temporary package archive, staging directory removed. **Not transferred or installed on Pegasus.** Native shared SPI helper's mock ioctl lab test passes including default off and opt-in TX/RX, but real pxa2xx hardware can ignore zero-length xfers. Strict experiment remains disabled and unproven; cannot claim 50us CS setup occurred electrically. Keep rel71.67 production-reference baseline on Pegasus and two pinned rollback packages. PR#31 remains DRAFT and merge blocked pending CS edge measurement and genuine user-attested hardware tests.
## CURRENT 2026-10-27 — rel71.69 SPI CS 50us source-only hypothesis; PEGASUS LIVE rel71.67
Real 71.68 LOCK1 2 human poses score4 rejected then12 positive, LOCK2 1 pose PIXEL_RESCUE score7/7. Real S3 many physical placements FAIL; multiple TLS/digest handshake failures then GET_IMAGE accepted without ANY IRQ (0/200) or TLS image, PREWARM_FALLBACK after39.569s. rel71.68 PR#30 closed; Pegasus checksum-pinned rollback rel71.67 successful, fprintd ACTIVE PID331542, exact flags FRESH_BG_QA+FDT_DOWN_PROOF_QA, no S3_LONG_FIRST, 32/32 intact; rel71.64 secondary fallback.
HOF best rel71.55 1pose S3 wake-to-match6.951s but 2/3 S3 reliability, best LOCK3.488s; rel71.30 strongest repeated LOCK18/20 but no proven S3/boot; rel71.24 targeted rebind before enumeration. New 2026 SAME-CHIP macOS project OctoDevStudios/MateBookFP reports 50us CS-to-clock eliminates digest/no-IRQ; sibling Sigfrodr goodixtls GitHub issue#6 reports MCU dead after repeated handshakes: GPIO resets insufficient, REACTIVE spidev rebind recovery only (preemptive makes worse). Rejected progressive short TLS window. New research branch rel71.69 starts rel71.67 source, adds opt-in GXFP_SPI_CS_SETUP_QA=1 zero-byte first transfer delay50us then identical payload within one SPI_IOC_MESSAGE(2); default path single original transfer. Kernel API delay_usec alone AFTER transfer, not CS setup. Bench on Pegasus pxa2xx with ftrace must verify physical CS held over zero-length delay before enabling; cannot claim hardware fix. Fake-ioctl native unit test source RED then GREEN; make verify PASS. NO deployment and no forced S3/reboot, threshold7 GCM and PAM unchanged. Full research report fingerprint/docs/rel71.69-cs-setup-and-upstream-audit-20261027.md. Avoid another speculative reset tweak or unsafe sysfs rebind.

## CURRENT 2026-10-27 00:25 CET — rel71.68 hardware FAILED and ROLLED BACK TO rel71.67
User LOCK1 two poses (first score4/7 rejected then score12/7), LOCK2 one pose (PIXEL_RESCUE score7/7), genuine S3 many poses FAIL. After initial long detached reset first S3 TLS fails, many more failed handshakes and resets, GET_IMAGE ack accepted but zero IRQ/TLS frame from 200 polls, no PREWARM_READY, fallback after39.569s; no unlock. Independent Sigfrodr libfprint-goodixtls GitHub issue#6 Sept2026 similar SPI-MCU handshake exhaustion (5187/51A7) and rescue only reactive spidev unbind/rebind; reset pulse alone does NOT fix, rebind before every authentication worsens. On OUR GXFP51A0 bus this is hypothesis not trace-proven. rel71.68 S3+LOCK NO-GO, PR#30 CLOSED. Live Pegasus rollback rel71.67 completed, removed LONG_FIRST option; native fprintd active PID331542, package32 intact, snapshots1934/1935, rel71.64 remains backup. User asks best of HOF + latest GitHub ideas for first-touch reliable all LOCK/S3/BOOT. New DEV candidate must start stable 71.67 source and look at protocol/MCU state & allow recovery ONLY on proven wedge; never repeat bad 71.65/66/68 reset delay guesses, no PAM/GPU/PMK/GPIO112 manipulations, no automatic reboot/sleep.

## CURRENT 2026-10-27 00:01 CET — rel71.68 QA INSTALLED Pegasus; physical S3 not tested yet
Package 1.94.100.goodix51a0-71.68 SHA256 20c4c54469bd188a7c5860fe4d861ad0f0245a887af8bc6897a15274a79c836f installed with Snapper pre1932/post1933, package 32 intact, native fprintd PID323421 ACTIVE with EXACTLY GXFP_S3_FRESH_BG_QA=1 GXFP_FDT_DOWN_PROOF_QA=1 GXFP_S3_LONG_FIRST_QA=1. Right-index enrolled. Source based on rel71.67, long-first reset only true S3 and strict FDT proof retained, hot LOCK/BOOT driver paths unchanged. Previous 71.67 real S3 eventually unlocked wake+13.129s after short-first TLS failure, user ~2 poses uncertain; Hall of Fame rel71.55 S3 wake->success6.951s but 2/3 first pose reliable. 71.68 actual LOCK and S3 pending: do not claim fast, first pose or certification. GitHub PR#30 draft (10/12 checks pass at install, 2 still running in dependency setup). Rel71.67 pinned rollback sha08681c32... and rel71.64 c3df31e8... BOTH on Pegasus, no forced reboot/S3. Developer package retained in LXC700 DEV temp, shared transfers/build folders cleaned. Follow QA trial and rollback ASAP if recurrent GET_IMAGE/TLS failures. Native full software verify passed.

## CURRENT 2026-10-26 — rel71.68 S3-ONLY HYBRID SOFTWARE QA; NOT INSTALLED
LIVE Pegasus remains rel71.67: real S3 eventually match score13/7 at wake+13.129sec, first short-reset TLS digest fail and full recovery prewarm9.665sec; owner approx 2 poses uncertain. HOF best rel71.55 S3 wake->match6.951sec but 2/3 first-touch reliability. Source rel71.68 combines first long detached reset from rejected rel71.66 (initial TLS success 5.844sec then image timeout) WITH rel71.67 strict FDT down two replies/ack, authenticated TLS receive diagnostics, presleep clean bg and fallback. Opt-in GXFP_S3_LONG_FIRST_QA=1 only true S3; no normal LOCK/boot path altered. Software validation only until Arch+GitHub CI and hardware S3. Current KDE Ant-Dark locally installed, wallpaper only374KB1600x1200, so no proof theme/asset causes slow startup; preserve default UI/PAM.

## CURRENT 2026-10-26 23:39 CET — rel71.67 genuine S3 eventually SUCCESS, owner ~2 poses uncertain, prewarm9.665s
Kernel confirms genuine deep S3. User thinks ~2 physical finger placements, possibly second unlocked: first-touch UNKNOWN. fprintd: preS3 freshly validated finger-off ImageBase preserved, first short detached reset after resume, TLS failed DIGEST at wake+4.906sec, then long detached reset/reopen and TLS finally ready at wake+9.665sec. One scored Verify 13/7, DETECTED->positive 1993ms, wake->positive 13129ms, no image timeout with FDT_DOWN_PROOF enabled. Native LOCK preceding 71.67 2/2 one-pose PASS but 4.76-5.52sec UI. Hall of Fame best rel71.55 S3 6951ms wake->positive (2/3 first-touch reliability), rel71.64 9541ms (first-pose unknown), rel71.67 materially slower S3 even though capture works. Need improvement aimed at eliminating unsuccessful initial TLS attempt and shortening safe recovery without new desync; boot cold TLS also flaky. Do not call 71.67 first-touch S3 PASS nor certification; don't merge PR29. All prior experiment failure evidence retained.

## CURRENT 2026-10-26 23:32 CET — rel71.67 second 1-pose LOCK, ~0.77s faster UI; S3 pending
User attests 1 successful pose, slightly faster still slow. KDE native Qt trace: QML first property 23:32:10.358, both PAM workers launch 23:32:11.139, hello 446ms after, fprintd FAST_READY83ms at 23:32:12.728, DETECTED 23:32:13.237, image score14/7 and positive 23:32:15.115. Comparable QML initial-property->positive 4757ms vs prior rel71.67 5523ms, 766ms faster; finger-to-positive1878ms. Earliest KDE greeter process starts 23:32:09.180, total to KDE unlock 5948ms. No FDT proof or TLS image timeout. Remaining lag KDE QML/PAM orchestration, not proven fingerprint image transfer bug. Preserve strict PAM password fallback, matcher threshold7, AES-GCM, mandatory cleanup and upstream QML cache safety. Do not change installed rel71.67 until true natural S3 validation. Log trial lock-20261026-7167-2332 and remove temporary Qt diagnostic file after capturing logs.

## 2026-10-26 23:28 CET — NATIVE KDE/PAM LOCK-LATENCY TRACE ENABLED
Measured 71.67 one-pose LOCK: greeter startup 23:22:47.419, fprintd FAST_READY 23:22:50.531 (291ms; FDT ACK retry), Verify READY 23:22:50.660, finger DETECTED 23:22:51.043, positive 23:22:52.941. Greeter-to-positive 5523ms, finger-to-positive 1898ms (score exactly7/7). Previous rel71.66 LOCK 5539ms; no speedup. KDE Worker not ready warnings also occur in historical 71.64 and independent upstream 6.8; causality not yet established, avoid unsafe PAM edits. For next natural owner LOCK, the native Qt per-user log config $HOME/.config/QtProject/qtlogging.ini has exactly '[Rules]' and 'kscreenlocker_greet=true', mode0600; no previous file overwritten. Built-in KDE log category, not a new service, no password/template/key dump. Correlate worker kde-fingerprint start/auth start with fprintd FAST_READY, Verify DETECTED/positive to isolate real delay. Can later remove this single diagnostic file. No automatic lock/reboot/suspend. Keep installed rel71.67 unchanged pending real natural S3; only source/docs modified, PR#29 still DRAFT.

## CURRENT 2026-10-26 23:22 CET — rel71.67 1-POSE LOCK PASS but very slow; S3 NOT YET TESTED
Physical owner attests 1 successful LOCK pose, very slow. Real KDE greeter 23:22:47.419 -> fprintd positive 23:22:52.941 = 5523ms (prior 71.66 5539ms), physical DETECTED 23:22:51.043 -> positive 23:22:52.941 = 1898ms; score exactly7/7, one frame, FAST_READY291ms (one FDT ACK retry). Most perceived 5.52sec is UI + native biometric worker readiness before finger recognition, not slower TLS image after ACK. KDE repeated Worker not ready warnings also historically seen in 71.64 and independently on 6.8 systems; avoid assuming fault without controlled measurements. Need optimize native KDE/PAM worker startup separately after evidence; do NOT switch/alter fingerprint driver until rel71.67 true natural S3 gate is observed. Record trial lock-20261026-7167-2323. Keep original right-index and rel71.64 rollback, PR #29 DRAFT.

## CURRENT 2026-10-26 23:11 CET — rel71.67 QA PEGASUS INSTALLED; PHYSICAL LOCK/S3 UNPROVEN
Native rel71.67 candidate from source c4c2350 tested libfprint build PASS; GitHub PR#29 12/12 CI PASS. Package SHA256 08681c32c1234b0fae0f6f61017d50df18f75bc1528a62fdb3d864b300e4d09c. Pegasus pacman 71.64->71.67 success, Snapper1930/1931, 32/32 package intact, native fprintd active PID311522, exactly GXFP_S3_FRESH_BG_QA=1 plus GXFP_FDT_DOWN_PROOF_QA=1, right index fingerprint still enrolled. rel71.64 rollback pinned on Pegasus c3df31e8dd06bfc778c661a023333a632f2824b4e841bbdf524ea1c4aa46e932; previous rel71.55 retained. Temporary Arch build stage and NFS/tmp transfers deleted. Do not force sleep/reboot. fprintd at startup had two failed digest TLS handshakes before success, previously seen on baseline; record as reliability signal, NOT success proof. Need human first LOCK and actual natural S3 with physical pose count and timings; on any regression rollback. PR#29 stays DRAFT, NO PRODUCTION CERTIFICATION. Previous PR#27/#28 CLOSED due genuine S3 failures.

## CURRENT 2026-10-26 — rel71.67 FDT proof and missing TLS telemetry, DEV QA ONLY
Based directly on rel71.64, not rejected 71.65/71.66. Root source bug: FDT-down 0x32 expects ACK + state response but gx_send_plain_drain accepted missing replies; possible reason subsequent accepted GET_IMAGE produced no authenticated TLS frame. Added STRICT opt-in GXFP_FDT_DOWN_PROOF_QA=1 requiring real successful ACK AND 2 plaintext FDT replies, else fail closed into existing full session recovery (never replay acknowledged image). New safe timeout-only TLS IRQ/frame count diagnostics, no image data. Test red->6/6 green and native libfprint software verify PASS; 71.67 Arch pkgrel prepared. Real S3 success NOT proved. Pegasus remains checksum-verified rel71.64 until a fully validated QA package is installed with rollback and user can naturally observe S3. PR #27 and #28 closed NO-GO; source-only evidence docs preserved.

## 2026-10-26 22:33 CET — REJECTED QA BRANCHES CLOSED / ROLLBACK FINAL VERIFIED
GitHub PR #27 (rel71.65) and PR #28 (rel71.66) explicitly CLOSED (NOT MERGED) after independently user-attested genuine S3 regressions. Pegasus final verified: libfprint-goodix51a0 rel71.64, 32 files/0 altered; fprintd ACTIVE PID301702, Environment ONLY GXFP_S3_FRESH_BG_QA=1; right-index-finger enrolled via native fprintd-list. No reboot, forced sleep, threshold/PAM/PMK change. Source audit: in GX capture path, gx_send_plain_drain sees GET_IMAGE ACK but gx_take_tls_frame exhausts IRQ polls without authenticated TLS image. Replay after acceptance is correctly forbidden to avoid image stream/GCM semantic desync; repeated hard resets failed to rescue this natural cycle. Dedicated future DEV research: bounded IRQ/frame-type delivery diagnostics and Linux/Windows GET_IMAGE capture transport parity, without logging biometric material or changing TLS validation; validate source-only before another installation. Do not claim rel71.64 S3 production certified, nor rel71.66 any successes on S3.

## CURRENT 2026-10-26 22:25 CET — rel71.66 TRUE S3 MULTIPLE POSES, NO UNLOCK; ROLLED BACK
User reports multiple physical poses and biometric failure after natural deep S3 on rel71.66. Actual logs: clean presleep reference + long detached GPIO264 reset, PREWARM_READY in 5844ms, but post-resume Verify GET_IMAGE ACKed then timed out without TLS image, 0 scored images. Subsequent detached resets/TLS handshakes fail; no terminal positive, multiple 'lift-required' while user touching. Root problem post-ACK TLS image delivery despite nominal prewarm, NOT fingerprint matching. rel71.66 hardware NO-GO, no production certification. Rolled back on Pegasus to verified rel71.64 pacman package, removed GXFP_S3_LONG_FIRST_QA; only GXFP_S3_FRESH_BG_QA=1, fprintd active PID301702, 32/32 unchanged. No reboot/S3 forced, saved enrolled finger preserved. rel71.65 was also S3 NO-GO. Branch rel71.66 test-only, do not merge PR#28. Next: research strict GET_IMAGE ACK/TLS receive path in DEV, prevent speculative modifications to Pegasus. Evidence docs/rel71.66-s3-long-first-qa-20261026.md, Hall of Fame CSV trial s3-20261026-7166-natural-2225.

## CURRENT 2026-10-26 17:01 CET — 71.66 physical LOCK PASS 1 pose, SLOW UI
Owner attests first human pose success but perceives LOCK as slow. Verified 17:01:46.587 DETECTED -> 17:01:48.462 positive: 1875ms, score13/7, one frame, FAST_READY83ms. KDE greeter 17:01:42.923 -> success 17:01:48.462: 5539ms versus 4797ms for prior 71.64 LOCK. Background age 957 seconds triggered in-session warm ImageBase refresh, but finger landed before it finished and new ImageBase was correctly discarded at 17:01:46.198 (~1.21s of delayed preparation). No evidence biometric matcher slower; actual end-to-end unlock delayed. Driver source rel71.66 MUST remain unchanged before genuine S3 test to isolate S3 fix. Preserve first-touch PASS in HARDWARE_TRIALS.csv, notes in docs/rel71.66-s3-long-first-qa-20261026.md; no reboot or forced sleep. Evaluate native concurrency / proactive fresh ImageBase without dropping strict image safety, preferably as follow-up branch after rel71.66 S3 result.

## CURRENT 2026-10-26 16:46 CET — rel71.66 QA INSTALLED PEGASUS / NATURAL S3 PENDING
The user-attested natural S3 at 16:09 under rel71.65 was NO-GO (3 physical poses; first post-wake TLS failure, 13.422s prewarm, 18.082s wake-to-match score16/7). Reverted to rel71.64 after fail. New native rel71.66 source branches directly from rel71.64 and keeps the rejected rel71.65 evidence docs only. Opt-in GXFP_S3_LONG_FIRST_QA=1 selects pre-existing HIGH300/LOW600 detached reset at first true S3 boundary rather than 10/150, no change normal LOCK/TLS/matcher/threshold7/PMK. TDD RED then 4/4 GREEN, make verify PASS, isolated Arch package built, GitHub draft PR#28 at commit 613044b CI 12/12 PASS. Package SHA256 8d76909098157eb5e27166395c04fb9ff336e38d7cf038e44459b3b860937de5. Pegasus pacman -U EXIT0, Snapper PRE1916/POST1917, 32/32 package files intact, fprintd PID129793 active with exactly GXFP_S3_FRESH_BG_QA=1 GXFP_S3_LONG_FIRST_QA=1. The original right-index-finger is still enrolled, verified with native fprintd-list. Pinned baseline rel71.64 package exists directly on Pegasus SHA256 c3df31e8dd06bfc778c661a023333a632f2824b4e841bbdf524ea1c4aa46e932; rel71.55 historic rollback preserved. NO REBOOT or suspend initiated. No human postinstall LOCK or S3 yet. Immediate next external acceptance gate: human actual first-touch after naturally occurring S3, correlate precise native logs and 30..90ms reset hypothesis. If regression, rollback to rel71.64 automatically and record FAIL. Never merge PR#28 or promote production without human hardware proofs.

## CURRENT 2026-10-26 — rel71.66 LONG-FIRST QA SOURCE ONLY / rel71.64 PEGASUS LIVE
After rel71.65 genuine natural S3 3 physical poses, severe first handshake TLS failure and 13.422-second prewarm, installed Pegasus rollback to native rel71.64 completed (SHA-verified 32/32, fprintd active only GXFP_S3_FRESH_BG_QA=1, enrolled right-index preserved, snapshots 1914/1915). rel71.65 PR#27 is NO-GO and was not reinstalled. New branch qa/gxfp51a0-rel71.66-s3-long-first-20261026 based directly on last rel71.64 source (304dec2), importing failed 71.65 HOF and evidence DOCS only. Tiny isolated C change: opt-in GXFP_S3_LONG_FIRST_QA=1 chooses existing reviewed detached HIGH300/LOW600 instead of short HIGH10/LOW150 on genuine first idle S3 resume; normal LOCK, TLS and biometric threshold 7 unchanged. Source TDD RED then GREEN 4/4, full safe software regression PASS, native libfprint-v1.94.100 Meson/Ninja build PASS (no sensor IO). NO ARCH QA PACKAGE/NO HARDWARE TEST AS YET; do not call release production. Next: push code, CI, isolated Arch build, cautious reversible QA installation if all gates clear; never force S3/reboot. Reference fingerprint/docs/rel71.66-s3-long-first-qa-20261026.md and prior rel71.65 NO-GO.

## 2026-10-26 16:14 CET — rel71.64 ROLLBACK COMPLETE / rel71.65 NO-GO
Pegasus pacman downgraded exactly one package libfprint-goodix51a0 71.65 -> 71.64 from verified SHA256 c3df31e8dd06bfc778c661a023333a632f2824b4e841bbdf524ea1c4aa46e932, transaction EXIT0. Snapper PRE1914/POST1915. Pacman -Qkk 32 files/0 altered. fprintd ACTIVE PID122523; Environment=GXFP_S3_FRESH_BG_QA=1 ONLY. Experimental cleanup-settle drop-in removed and replaced by single native S3 background drop-in. Enrolled right-index-finger confirmed via fprintd-list. No reboot or forced suspend, kernel/PAM/PMK untouched. 71.65 physical S3 FAILED (3 human placements), HOF CSV correctly retained, PR #27 remains DRAFT/NO-GO. Investigate guarded full detached GPIO264 reset at first true S3 wake in separate QA branch, preserving strict TLS/image/matcher gates. Do not reinstall 71.65.

## CURRENT 2026-10-26 16:09 CET — rel71.65 natural S3 FAIL / 3 human poses; rollback planned
Original user S3 observed 3 poses; first post-wake TLS handshake failed and detached recovery pushed PREWARM_READY to 13.422s, wake-to-positive 18.082s, accepted final score16/7; cleanup GET_IMAGE no-ACK additional 1s persists despite cleanup-settle QA. Hardware CSV/HOF records NO-GO, no quality threshold regression, no evidence that settle itself caused TLS failure. Roll back rel71.65 to checksum-verified rel71.64 with GXFP_S3_FRESH_BG_QA=1 only, preserve 71.55 and 71.65 packages, no reboot/S3. New QA work must address first handshake native lifecycle and bounded recovery; no promotion of PR#27. Use fingerprint/docs/rel71.65-cleanup-settle-qa-20261026.md for detailed evidence.

## CURRENT — 2026-10-26 rel71.65 first live LOCK documented

Actual Pegasus rel71.65 unchanged, fprintd active, 2 QA flags enabled. LOCK at 12:49:26.854306 -> positive 12:49:28.829444: 1975 ms finger detection to terminal, one scored frame 15/7, FAST_READY 83ms, no second scored press in logs. Physical first-touch count UNKNOWN (owner did not attest poses). Compared to rel71.64 LOCK 1994 ms, difference 19ms is not an established speedup; cleanup GET_IMAGE hypothesis remains unproved. HOF CSV trial lock-20261026-7165-1249 recorded, HOF generated and --check PASS. Software regression make verify-no-build EXIT0 (native libfprint build reused from earlier 71.65 CI; no hardware access). No reboot, suspend, package or PAM change; rel71.64 and rel71.55 rollback intact. PR #27 remains draft. Next: human-observed true natural S3 first-touch and wrong-finger/reboot/cold gates; only then promote, remove QA flags and clean build experiments. Avoid repeating already-passed software test without source changes.

## 2026-10-26 12:30 CET — native integration audit (read-only; no hardware touch)
CachyOS kernel 7.3.0-4-cachyos; rel71.65 fprintd service active with ONLY GXFP_S3_FRESH_BG_QA=1 and GXFP_CLEANUP_SETTLE_QA=1. Installed pacman integrity: 32 files/0 altered. KDE uses its native kde-fingerprint PAM chain; polkit-1 PAM tries pam_fprintd first with system-auth password fallback; system-auth itself was not modified. Bitwarden 2026.9.1 AppImage launcher and its system-auth Polkit policy were present; application's own biometric setting and end-to-end prompt NOT tested. No physical LOCK or genuine S3 has been recorded on rel71.65 yet. Do not merge PR #27, remove pinned rollback, or clean active QA/build sources until physical LOCK/S3/cold-boot and negative-finger gates pass. libfprint/fprintd cannot authenticate apps that do not support the standard Linux auth interfaces.

## CURRENT 2026-10-26 12:20 CET — rel71.65 QA INSTALLED on Pegasus

On Pegasus native libfprint-goodix51a0 rel71.65, fprintd PID 58124 ACTIVE. EXACT flags GXFP_S3_FRESH_BG_QA=1 plus GXFP_CLEANUP_SETTLE_QA=1, preserving the successful rel71.64 clean pre-S3 reference plus only a 30..90ms additional bounded settle before mandatory authenticated GET_IMAGE cleanup. Score threshold=7 and TLS sequence cleanup unchanged. This is a small unproven speed optimization, not production certified. No reboot/forced S3, user right-index finger must be preserved.

All 12 Github CI checks plus native verify and 5 targeted cleanup source tests PASS on source commit 69ebe09, PR #27 DRAFT. Installed package SHA256 5db44952796634540e55896292c9be1cfea31993d5c31bef7ea02347737058a1, integrity 32/32 intact, Snapper 1912/1913. Immediate baseline rollback rel71.64 package kept on Pegasus in /var/lib/gxfp51a0-qa-rollback/rel71.64.pkg.tar.zst SHA256 c3df31e8dd06bfc778c661a023333a632f2824b4e841bbdf524ea1c4aa46e932, plus rel71.55 pinned SHA256 f1432414c19fd13bf5abbde9b8b713b3a606e18af89dcdb85d0c6334e5a14deb.

Next trial: owner reports natural LOCK/S3 physical placements and subjective speed; fetch precise fprintd logs, pre-sleep clean background and presence/absence of 1-second cleanup GET_IMAGE missed ACK; record immutable HARDWARE_TRIALS.csv and HALL_OF_FAME.md. Avoid falsely classifying 71.64 S3 as first-touch attested; that owner was uncertain. If regressions restore rel71.64. No host reboot/suspend automatically. Source checkout: CT700 project worktree; see private Pegasus workstation handoff for host-local path. GitHub branch qa/gxfp51a0-rel71.65-cleanup-settle-20261026.

CURRENT 2026-10-26: rel71.65 QA SOURCE branch, based on rel71.64 which is still installed on Pegasus. Adds only optional GXFP_CLEANUP_SETTLE_QA=1 extra 30..90ms delay before mandatory cleanup GET_IMAGE. No ACK/TLS bypass, no matcher/contrast/threshold change; no proven speedup. At natural deep S3 11:20 rel71.64 genuine clean pre-sleep reference worked, score20/7 single scored Verify press, physical pose count uncertain, cleanup GET_IMAGE required one 1000ms no-evidence retry; see docs/rel71.64-s3-presleep-fresh-qa-20261025.md. Isolated Arch rel71.65 package built SHA256 5db44952796634540e55896292c9be1cfea31993d5c31bef7ea02347737058a1, software tests passed. Exact commit GitHub CI and real hardware QA pending. No auto reboot or suspend; pinned rel71.55 fallback retained. PR stays DRAFT.

## CURRENT 2026-10-26 11:20 CET — real rel71.64 clean S3 reference succeeds, physical pose count uncertain

Pegasus rel71.64 QA still active, original enrolled right-index-finger intact. User reports natural S3 unlocked but unsure whether first long press or second quick press. Kernel confirms genuine deep S3 11:18:58->11:20:17. fprintd logs rel71.64 successful 1.211s clean pre-S3 ImageBase refresh, SLEEP ACK and saved reference; post-wake prewarm 3789ms, one scored Verify press at 11:20:24.436, 20/7 PASS, terminal 11:20:27.431 (2995ms finger-to-terminal / 9541ms wake-to-terminal). A cleanup GET_IMAGE command got no ACK/TLS for 1000ms and was safely retried: this added measured latency after successful scoring. Safety requires full cleanup and AES-GCM synchronized TLS read. Human physical first-touch classification UNKNOWN until attested. Separate BOOT unspecified 08:44 score20/7 one press 1811ms and LOCK 08:46 score17/7 one press 1994ms. BOOT must not be counted as COLD or REBOOT. HOF CSV and driver evidence updated. Continue optimizations automatically after user trials; preserve rollback rel71.55; no forced reboot/S3, threshold 7 unchanged. Source references and details: docs/rel71.64-s3-presleep-fresh-qa-20261025.md.

## CURRENT 2026-10-25 23:50 CET — rel71.64 QA INSTALLED on Pegasus

Pegasus actual libfprint-goodix51a0 rel71.64, active fprintd PID 276490, ONE opt-in GXFP_S3_FRESH_BG_QA=1, retained enrolled right-index finger. No reboot or forced S3. Root snapshots 1888/1889, pacman integrity 32/32, QA package SHA256 c3df31e8dd06bfc778c661a023333a632f2824b4e841bbdf524ea1c4aa46e932. Pinned rel71.55 native rollback intact SHA256 f1432414c19fd13bf5abbde9b8b713b3a606e18af89dcdb85d0c6334e5a14deb. rel71.63 partial flag removed.

Code on branch qa/gxfp51a0-rel71.64-presleep-fresh-bg-20261025, driver hardening fc3f32e, 7 specific safety tests PASS, native compile/baseline PASS, cross distro CI 12/12 PASS. Source improvement opportunistically refreshes a PROVEN CLEAN reference before S3 with 2.3s hard deadline inside 5s native logind inhibitor; fail-closed on incomplete TLS. No matcher threshold reduction, no enrollment changes.

Genuine natural deep S3 hardware test under rel71.64 NOT DONE. Need physical user observation of first pose after natural resume; inspect S3_FRESH_BG_QA, image quality, scored biometric decision and native timestamps. Do not claim resolved or merge PR #26. Prior rel71.63 S3 first image contrast failed, second finger 15/7 pass; prior S3 lock performance is not proof for the new candidate.

## CURRENT 2026-10-25 — rel71.64 clean presleep ImageBase QA development

On hardware Pegasus STILL runs rel71.63 with optional partial FDT S3 handler, with real first-touch S3 FAIL (2 touches; first pre-scoring contrast reject), regular LOCK 1 touch/15 score PASS. Source branch qa/gxfp51a0-rel71.64-presleep-fresh-bg-20261025 builds only in CT700. New GXFP_S3_FRESH_BG_QA=1 opt-in collects a FINGER-OFF fresh authenticated ImageBase BEFORE native idle S3 sleep command, only if the previous ImageBase was stale, with independent live FDT before/after and deny saving a stale or contaminated snapshot if validation fails. No contrast or biometric threshold loosening. Historical failed rel71.60/61/62/63 trials retained.

DO NOT auto-suspend/reboot; no user fingerprint re-enrolment or PAM/kernel/GPU modifications. Must pass full QA, cross distro CI, isolated Arch build, reversible package and human-observed true deep-S3 first-touch to promote. rel71.63 stays installed until confirmed ready and explicit safe state; pinned rel71.55 rollback remains.

CURRENT 2026-10-25: rel71.63 QA Pegasus; LOCK first touch 15/7 pass 1869ms. Natural deep S3 first press rejected by contrast gate BEFORE score; second press 15/7 passed. Prewarm 4668ms, total native wake-to-positive 12555ms. This first touch was full 0x3f, NOT testing new 4-5-zone branch. S3 first-touch NO-GO. No downgrade yet; rel71.55 pinned. See 71.63 QA doc and HOF.

## CURRENT 2026-10-25 22:23 CET — rel71.63 QA INSTALLED on Pegasus

This supersedes historical rel71.55-installed entries. Pegasus currently runs libfprint-goodix51a0 1.94.100.goodix51a0-71.63; fprintd PID 256412 active with GXFP_S3_EARLY_PARTIAL_QA=1 only. Original right-index-finger enrollment intact; native D-Bus fprintd-list successful, package 32 files 0 altered, Snapper 1886/1887, no reboot or forced sleep. QA package SHA256 4c67d31538bed8dc6a6e8ce82f984afc41f0b7caea83959d2ecb4b52f3aaee77. Pinned rel71.55 rollback SHA256 f1432414c19fd13bf5abbde9b8b713b3a606e18af89dcdb85d0c6334e5a14deb verified. All 12 GitHub checks on driver commit ff514a3 pass.

The previous interactive pacman -Syu finished before installation. Source branch qa/gxfp51a0-rel71.63-s3-early-partial-20261025, PR #25 DRAFT. Hardware gate for genuine natural deep S3 and early four-zone finger at resume REMAINS UNTESTED. Do not claim rel71.63 S3 fixed until normal physical first-touch, latency, negative finger and real cold boot evidence; do not force reboot or sleep. Log the user's next S3/LOCK physically observed results with exact fprintd journals and update HALL_OF_FAME.md/HARDWARE_TRIALS.csv. If failure, rollback rel71.55 natively.

## 2026-10-25 — rel71.63 S3 early-partial QA source IN DEVELOPMENT

Pegasus is running rollback rel71.55 with active native fprintd, original right-index-finger enrolment, no experimental flags. This branch qa/gxfp51a0-rel71.63-s3-early-partial-20261025 is based on rel71.55 source and contains ONLY opt-in new GXFP_S3_EARLY_PARTIAL_QA behavior: allow an independently double-verified 4-5/6-zone early finger to use authenticated RAM-only clean pre-S3 image reference under existing 12h-age and genuine S3 guards. The real failure was 0x3c at 21:06:12 after natural S3, while historical gate required 0x3f; 4 full calibration attempts then consumed 37.625s and S3 authentication failed. Threshold stays 7, existing SIGFM and necessary cleanup unmodified.

Software QA and native build / isolated Arch / cross distro CI / real S3 hardware gates must ALL pass before promotion. No remote forced reboot/S3, no auto enrollment, no global PAM, PMK, kernel or GPIO112 changes. Canonical current Hall of Fame including rel71.62 S3 FAIL and rel71.62 after-S3 LOCK PASS migrated into this source branch as docs only, NO failed source patches copied. Read docs/rel71.63-s3-early-partial-qa-20261025.md.

## CURRENT 2026-10-25 21:18 CET — rel71.62 S3 NO-GO and VERIFIED rel71.55 rollback

Real Pegasus currently runs rel71.55 again (pacman verified 32/32
intact; active native fprintd PID 198098, empty experimental
Environment), Snapper 1870/1871; no reboot/forced S3.
Owner natural deep S3 rel71.62 failed after multiple contacts:
S3 wake 21:06:08, four touch-contaminated reference attempts,
no valid biometric Verify before its close, prewarm 37.625s.
Distinct subsequent LOCK 21:07 first physical finger PASS, score 9/7,
finger->terminal 1.956s, greeter->terminal 4.843s, subjectively
very slow. Two other genuine S3 cycles had long initialization and
a first 4/7 rejection, documented in
docs/rel71.62-fast-retry-qa-20261025.md. Canonical hardware
CSV/HOF record S3 FAIL and LOCK PASS separately. PR #24 must
remain DRAFT, hardware NO-GO, no merge. Do not misclassify
successful LOCK after S3 as S3 success.
Pursue early-finger S3 session calibration with strict threshold 7;
user enrolment and Linux native fprintd/PAM preserved. No automatic
host suspend/reboot. Use SentinelX via Cloud9 for authorized SSH.

## CURRENT 2026-10-25 17:36 CET — Pegasus rel71.62 QA INSTALLED

Supersedes earlier note claiming rel71.55 currently installed. Pegasus currently runs libfprint-goodix51a0 1.94.100.goodix51a0-71.62 with GXFP_FAST_RETRY_QA=1 (ONLY) in active fprintd, original enrolled right-index-finger intact, package 32 files/0 altered. All 12 GitHub CI PASS for source ae27dd6. SHA256 917502e83a55569236a3a4fe36e4b7916001aacc51b9aa52bc9ac71ea59d953b. Snapper 1868/1869. Native rollback rel71.55 SHA256 f1432414c19fd13bf5abbde9b8b713b3a606e18af89dcdb85d0c6334e5a14deb verified. No reboot or suspend, global PAM/kernel/PMK/GPU changes.

REMAINING HARDWARE GATE: user first physical LOCK trial (one/two/multiple, perceived speed). Read exact fprintd journal for score, READY, DETECTED, release, FAST_RETRY_QA and positive terminal; update HARDWARE_TRIALS.csv and HALL_OF_FAME.md. Reject on any regression; restore pinned rel71.55 if necessary. PR #24 DRAFT, not to merge until hardware and wrong-finger/S3/cold-boot/reboot pass. Candidate only reduces an unnecessary second-press rebase and does NOT fix first image quality. Detailed docs/rel71.62-fast-retry-qa-20261025.md.

## CURRENT 2026-10-25 17:25 CET — rel71.62 fast retry QA source, NOT installed

Pegasus physically powered and verified installed rel71.55, fprintd active,
original right-index-finger enrollment intact; no experimental service
Environment. Previous rel71.61 failed first touch (5/7 rejected, second 7/7),
rollback rel71.55 confirmed, Snapper 1866/1867.

CT700 now has QA rel71.62 opt-in GXFP_FAST_RETRY_QA=1, designed solely to
skip an approximately 996 ms stale background recalibration during
second-physical-press recovery when TLS/FDT/production state intact.
Matcher threshold stays 7, first pose behavior unchanged.
The 4 focused tests and full native software baseline compiled and PASS.
DO NOT claim first-touch improvement; this is a second-press recovery
latency experiment and does not fix original first score 5/7.
The former rel71.61 score-cache feature remains present but OFF by default;
never enable both flags together without separate A/B evidence.
See docs/rel71.62-fast-retry-qa-20261025.md.
Before any deployment ensure package SHA256, CI, rollback and owner physical
LOCK after install; no host reboot/suspend, no PAM changes.

## CURRENT 2026-10-25 02:17 CEST: rel71.61 first-touch NO-GO, rel71.55 restored

Real first LOCK rel71.61 user 2 poses, first 5/7 rejected, second
7/7 pixel rescue accepted; first finger->terminal 7453ms, second
finger->terminal 1761ms. ImageBase 958665ms old, 1s attempted
retry refresh discarded due quick retouch. Installed rel71.55 restored
02:16:58, active fprintd/no QA Environment, package 32/32 intact,
snapshots 1866/1867. PR #23 remains draft NO-GO. Evidence in
docs/rel71.61-score-cache-qa-20261025.md and complete hardware CSV.
Previous installed-rel71.61 entries are historical, not current.
No forced reboot/suspend, use SentinelX only. Keep all Hall of Fame
and pre-Oct13 31-config evidence intact.

## CURRENT 2026-10-25 01:46 — Pegasus rel71.61 QA INSTALLED

This replaces the older note saying rel71.55 is currently installed.
On Pegasus now: libfprint-goodix51a0 1.94.100.goodix51a0-71.61,
fprintd PID 130421 active, GXFP_VERIFY_SCORE_CACHE_QA=1 in ACTUAL
service process Environment. Device GXFP51A0 ST411 enumerated with
right-index-finger enrolled, 32/32 package files intact.
Native package installation Snapper 1864/1865; no host reboot, no
forced S3, no global PAM or kernel/PMK/GPIO112 change.
12/12 CI software green for source commit d3541e1; isolated Arch
package SHA256 4ffbb16eee7a440374a7f72fdec4b8ef534d1963be570bb97385f1704e2ed389.
Rollback pinned rel71.55 SHA256
f1432414c19fd13bf5abbde9b8b713b3a606e18af89dcdb85d0c6334e5a14deb.

CANDIDATE STILL QA ONLY. Measured synthetic 20-view SIGFM comparator
saving ~192ms CPU cannot certify actual KDE unlock. Human physical
LOCK rel71.61 one-placement latency/score is NOT measured yet.
Need user physical test; then capture fprintd logs and append
an exact rel71.61 trial to docs/HARDWARE_TRIALS.csv, regenerate
docs/HALL_OF_FAME.md, commit/push. If failure, restore rel71.55
before any more experimentation. Keep PR #23 DRAFT.
Doc: docs/rel71.61-score-cache-qa-20261025.md.
No automated reboot or forced S3.

## CURRENT 2026-10-25 — rel71.61 opt-in score-cache QA development

Live Pegasus is still production fallback rel71.55, not rel71.61.
Source QA branch qa/gxfp51a0-rel71.61-score-cache-20261025
derived directly from research/gxfp51a0-rel71.55-firsttouch-20261025,
without any rel71.58/.59/.60 S3 modifications.
Changed ONLY verification scoring reuse (fixed threshold 7,
one image per pose, worker-owned exact FpPrint pointer guard and
mandatory cleanup). Flag GXFP_VERIFY_SCORE_CACHE_QA=1, OFF by
default. Other desktop/PAM/kernel/firmware/PMK/GPU settings remain
untouched.

Reused canonical HALL_OF_FAME.md, HARDWARE_TRIALS.csv and historical
evidence files from prior 71.60 QA documentation commit 66d26ff;
this is a docs-only migration, NOT an import of failed 71.60 code.
Full 2026-10-25 real CPU synthetic benchmark at
docs/rel71.61-score-cache-qa-20261025.md and
research/eval/bench-verify-score-cache.py: 20 views median double
386.2613ms, single 194.1066ms, ~192.1546ms saved purely in CPU
matcher (NOT proven full unlock improvement).

Native libfprint build and software suite must pass at final commit,
then isolated Arch package SHA256 and 12 cross-distro GitHub CI tests.
Only afterward install reversible QA onto Pegasus and request one
human normal first-touch LOCK. No remote reboot, suspend, forced login,
enrolment, PAM or personal biometric material extraction.
Keep pinned rel71.55 package for rollback, do not merge to main until
real first-touch/S3/boot security gate passes.

## 2026-10-25 rel71.55 optimisation research — no code change

Pegasus runs pinned rel71.55 and must NOT be switched to the rejected
71.58/71.59/71.60 QA builds. The research branch based on the rel71.55
certification source is
research/gxfp51a0-rel71.55-firsttouch-20261025.
Read docs/RESEARCH_REL7155_FIRSTTOUCH_20261025.md and the latest
historical Hall of Fame (commit 66d26ff on the failed 71.60 QA branch).
NEW Oct22 SAME SENSOR real-SIGFM benchmarks suggest offline testing
20 vs 25 distinct enrolment views, while retaining fixed threshold 7
and validating many unrelated impostor fingers.
Do not adopt RootSIFT, score fusion, multiple scored draws per pose,
or lower threshold based on sibling-model claims.
Instrument capture vs mandatory TLS cleanup as separate bottlenecks;
no user biometric images/templates ever in the public repo.
No driver code or Pegasus configuration was altered by this research.
Continue only with controlled QA and user-supervised hardware trials.

# Public status — GXFP51A0 / ST411 fingerprint driver

**Default:** [rel71.30 recommended preview](https://github.com/GodsQuantum/Huawei-Matebook-13-Linux/releases/tag/matebook13-rel71.30).

**Last fully cold-boot/deep-S3 validated rollback:** [rel71.24](docs/validated-checkpoint-71.24.md).

On the reference MateBook 13, rel71.30 completed enrollment using the **native KDE graphical interface**; eight observed verification captures passed at the unchanged threshold 7. A separate wrong-finger control rejected an unenrolled finger (scores 2, 3, 4) before the enrolled right index succeeded (16). These are **single-machine results**, not a population-level false-acceptance measurement.

Ubuntu 24.04/26.04 source builds pass; Ubuntu 26.04.1 native build and fprintd ABI compatibility also pass. A real Ubuntu fingerprint unlock and MX250 GPU test have **not** yet been completed. Cold-boot/deep-S3 testing of rel71.30 and testing on an independent GXFP51A0 device remain open.

## Start here

- [Driver-only installation, all supported distributions](DRIVER_ONLY.md)
- [Current driver README](README.md)
- [Technical evidence and remaining acceptance criteria](docs/candidate-71.30.md)
- [Hardware safety and privacy requirements](docs/safety.md)
- [Previous investigations](docs/history/)

There is no firmware flashing, automatic enrollment, or redistribution of fingerprint templates or machine-specific keys. Internal personal handoffs and development materials are excluded from the public repository.
