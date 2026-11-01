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
