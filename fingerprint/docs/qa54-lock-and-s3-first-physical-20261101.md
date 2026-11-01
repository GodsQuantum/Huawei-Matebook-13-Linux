# Pegasus QA54 — LOCK et S3, 1er novembre 2026

Source : journal horodaté `fprintd` du Pegasus, fenêtre Europe/Paris 12:12:30–12:14:10, relevé via SSH par Cloud9 ; retour utilisateur : « LOCK : 1ere pose et plutot rapide », « S3 : 2eme pose je crois ou 1ere, ça avait l'air rapide aussi ». Logiciel effectivement installé : `libfprint-goodix51a0 1.94.100.goodix51a0.qa54-1`. Les horaires sont ceux des événements, pas ceux du message utilisateur. Aucune image, empreinte, template ni journal brut n'est conservé dans ce document.

## LOCK — succès direct au premier appui

- 12:12:38.250042 `FAST_READY` 52 ms, doigt absent, ImageBase ancienne 3910956 ms ; refresh `WARM_REBASE` en 801 ms (ACK miss et calibration protocole 150→200 %).
- 12:12:39.080479 `VERIFY_READY`; 12:12:39.943902 `DETECTED_HOLD` pose 1/3, zones=6.
- 12:12:40.532851 score SIGFM **12/7**, 67 points, **20 vues d'enrôlement**. Pas de `ROTATION_RESCUE_QA` (non nécessaire).
- Image 1 : frame 572 ms + SIFT 2 ms + matching 15 ms = **591 ms** ; cleanup **566 ms**, ok=1.
- 12:12:41.101760 `positive terminal` score12/7, sans attendre le retrait du doigt.
- **DETECTED_HOLD → terminal = 1157,858 ms** (CSV arrondi 1158 ms). **VERIFY_READY → terminal = 2021,281 ms** ; **FAST_READY → terminal = 2851,718 ms**, dont la remise à jour de la référence.
- Record précédent QA42 LOCK = 1180,552 ms au même marqueur ; **gain réel sur un essai = 22,694 ms (~1,92 %)**. Une seule observation, ne certifie pas fiabilité.

## S3 — succès par compensation de rotation, une seule pose détectée

- 12:13:26.873526 `NATIVE_S3_PARK ack=1`, référence fraîche protégée RAM.
- 12:13:53.766110 `NATIVE_S3_RESUME_BEGIN`.
- 12:13:57.554050 préchauffage natif **3787 ms**, TLS validé et ImageBase propre, cache PMK ; première tentative de TLS complète.
- 12:13:59.103306 `FAST_READY` **83 ms**, doigt déjà maintenu `touch=0x3f`; FIRST_HELD_POLL_QA signalé.
- 12:13:59.156371 `VERIFY_READY`; 12:13:59.438957 `DETECTED_HOLD` **pose 1/3**.
- 12:14:00.397378 `ROTATION_RESCUE_QA angle=+12`, score initial **3/7**, score rotation **15** sur vue13 ; overlap4391, ZNCC812/1000, agreement873/1000. Le score retourné à la voie Verify est le seuil nominal **7/7** ; on ne confond pas ce chiffre avec le score BRIEF rotation 15.
- Image 1 : frame **897 ms**, SIFT **5 ms**, matching avec rotation **54 ms**, total **958 ms** ; cleanup **752 ms** ok.
- 12:14:01.150290 `positive terminal` sans attendre le retrait.
- **DETECTED_HOLD → terminal = 1711,333 ms** ; **VERIFY_READY → terminal = 1993,919 ms** ; **FAST_READY → terminal = 2046,984 ms**. **RESUME_BEGIN → terminal = 7384,180 ms** (pas la latence du doigt).
- L'utilisateur hésite entre 1re/2e pose ; le journal montre seulement **une pose capteur et une capture classée** ; `first_touch=unknown` par rigueur, donc pas un succès première pose certifié dans le HOF. Aucune première pose antérieure au cycle de lecture ne peut être exclue.

## Sécurité, limites, suite

- Même doigt enregistré (index droit), pas de réenrôlement requis à ce stade. QA54 issu de QA49, opt-in rotation Verify ; aucun affaiblissement du seuil nominal7, mais l'exploration de plusieurs orientations peut changer le taux de faux positifs : **aucun corpus imposteur/genuine calibré**, pas de promotion prod certifiée.
- Comparer au record LOCK QA42 et aux échecs QA49 séparément. Demander S4 et test doigt inconnu rejeté, puis plusieurs essais LOCK/S3 avant certification.
- Éviter d'optimiser seulement le matching : coût ImageBase refresh **801 ms** au LOCK, et capture/cleanup encore ~1,16s après détection ; conserver les garanties TLS/FDT.
