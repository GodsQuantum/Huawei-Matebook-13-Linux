# QA55 — annulation native d'une Identify pendant mise en veille

**Source** : [issue GitHub #6, commentaire du 1er novembre 2026](https://github.com/GodsQuantum/Huawei-Matebook-13-Linux/issues/6#issuecomment-6557944671), MateBook 13 **2020**, firmware ST411. Lors d'une suspend-then-hibernate pendant Identify WAIT_ON, libfprint signale `Cannot run while suspended`, ReleaseDevice échoue busy, puis les resumes avec `idle=0` ne préchauffent plus. Pas encore reproduit sur Pegasus 2021.

## Portée QA55

- Base QA54 (record LOCK 1.158s, S3 rotation +12°). **Ne modifie pas** son matching/rotation, seuil7, TLS, FDT, ImageBase, timing normal, enrollment ou PAM.
- Ajout du callback **natif** `FpDeviceClass.cancel`. Si `GXFP_CANCEL_PENDING_POLL_QA=1`, ce callback supprime immédiatement le timer GLib FDT **seulement** pour une SSM WAIT_ON ou WAIT_OFF, puis termine cette SSM avec `G_IO_ERROR_CANCELLED`. Les callbacks de fin vérifient / effacent le pointeur temporaire.
- Un worker de capture ou de TLS reste propriétaire de ses FDs jusqu'à sa fin; son chemin d'annulation existant est préservé. Ni thread additionnel, ni daemon, ni system-sleep hook, ni timer de surveillance. Flag désactivé par défaut.
- Dans libfprint 1.94.100, `fpi_device_suspend_complete(error)` annule le GCancellable et attend la fin de current_task avant de terminer Suspend. L'appel au callback cancel passe par le contexte principal; il évite la dépendance à un timer de polling restant actif.
- Le correctif vise le blocage à l'attente; il ne garantit pas les cas où le **worker TLS/SPI lui-même** se bloque pendant la suspension. La récurrence sur le matériel 2020 n'est pas certifiée.

## Gates

- Tests C extraits **du vrai callback QA55** sous GLib, avec sources de temps réelles : WAIT_ON, WAIT_OFF, worker en cours, flag désactivé, idempotence et source supprimée.
- `make verify` sur CT700, compilation native libfprint 1.94.100 et régressions; tests GitHub.
- **Gates physiques ultérieures** : annulation réelle de fprintd-verify/identify, retour à fprintd actif, LOCK première pose, S3, S4, wrong-finger rejeté; aucune veille forcée pendant une session utilisateur sans validation. Ne pas promouvoir en production sans tests du scénario.


## Pegasus QA55 hardware smoke — 2026-11-01 15:44 CET

- Build `makepkg` Arch Pegasus PASS, package `libfprint-goodix51a0 1.94.100.goodix51a0.qa55-1` installed (no reboot). SHA256 `da5ee21764af0183505adb2d520a15bda9276947bf306c1da903f259c848ebed`. Full `pacman -Qkk`: 32/32 intact. `fprintd` active. Enrollment index droit retained.
- Flags still active: `GXFP_ROTATION_RESCUE_QA=1` from QA54, `GXFP_CANCEL_PENDING_POLL_QA=1` from QA55. No match changes.
- First `fprintd-verify` without physical placement, interrupted after seven seconds: `VERIFY_TRACE physical press 1/3 READY` at 15:44:06.753457; `CANCEL_PENDING_POLL_QA state=1 removed_fdt_source=1` at 15:44:12.821577; warm `CLOSE_TRACE` at 15:44:12.830699 (approx **9 ms** after cancellation).
- Second `fprintd-verify -f any` (fprintd chose its 1:1 Verify path), interrupted after six seconds: `VERIFY_TRACE physical press 1/3 READY` at 15:44:43.011957; `CANCEL_PENDING_POLL_QA state=1 removed_fdt_source=1` at 15:44:47.890667; warm `CLOSE_TRACE` at 15:44:47.890950 (<1 ms after cancellation). Following Claim uses `FAST_READY` **60 ms** (no cold recovery). Neither test captured a print.
- These tests prove real **Verify waiting cancellation** and next session opening; they do NOT prove cancellation during true Identify, or that the 2020 s2idle/S4 bug is fixed. Do not mark production certified. No manual suspend/hibernate was triggered.
- Community follow-up posted at https://github.com/GodsQuantum/Huawei-Matebook-13-Linux/issues/6#issuecomment-6559488823 to request independent Identify→suspend evidence. Preserve QA54 rollback and 2021 fingerprint template.
