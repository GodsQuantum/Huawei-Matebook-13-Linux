#!/usr/bin/env python3
"""Read-only GXFP51A0 release ledger and generated Hall of Fame (no biometric data)."""
import argparse
import csv
import statistics
from datetime import datetime
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TRIALS = ROOT / "docs/HARDWARE_TRIALS.csv"
PAGE = ROOT / "docs/HALL_OF_FAME.md"
METRICS = ("prewarm_ms", "fast_ready_ms", "press_to_terminal_ms",
           "greeter_to_terminal_ms", "wake_to_terminal_ms")


def load():
    with TRIALS.open(newline="", encoding="utf-8") as f:
        rows = list(csv.DictReader(f))
    assert rows
    last, ids = None, set()
    for r in rows:
        rid = r["trial_id"]
        assert rid and rid not in ids, rid
        ids.add(rid)
        at = datetime.fromisoformat(r["datetime_local"])
        assert at.tzinfo is not None and at.utcoffset() is not None
        assert last is None or last <= at, rid
        last = at
        assert r["kind"] in {"LOCK", "S3", "COLD", "REBOOT"}, rid
        assert r["version"].startswith("rel71."), rid
        assert r["threshold"] == "7", rid
        assert r["first_touch"] in {"pass", "fail", "unknown"}, rid
        if r["first_touch"] == "pass":
            assert r["poses"] == "1", rid
        if r["first_touch"] == "fail":
            assert r["poses"] not in ("", "1"), rid
        assert r["evidence"] and (ROOT / "docs" / r["evidence"]).is_file(), rid
        for k in METRICS:
            if r[k]:
                assert int(r[k]) > 0, rid
        if r["kind"] != "S3":
            assert not r["prewarm_ms"] and not r["wake_to_terminal_ms"], rid
    return rows


def seconds(v):
    return f"{int(v)/1000:.3f}" if v else "—"


def render(rows):
    parts = [
        "# GXFP51A0 — Hall of Fame matériel",
        "",
        "Source canonique : [HARDWARE_TRIALS.csv](HARDWARE_TRIALS.csv). "
        "Les premières poses proviennent du retour humain, "
        "les mesures des journaux horodatés. "
        "Les captures biométriques et journaux bruts ne sont pas publiés. L'heure du LOCK du 23 octobre vers 13 h 17 est approximative et n'est pas utilisée pour un record de durée.",
        "",
        "## Records valides (réussite dès la première pose)",
        "",
        "| Discipline | Temps | Version | Essai |",
        "|---|---:|---|---|",
    ]
    for label, kind, metric in (
        ("LOCK : doigt → succès", "LOCK", "press_to_terminal_ms"),
        ("LOCK : KDE → succès", "LOCK", "greeter_to_terminal_ms"),
        ("S3 : préchauffage", "S3", "prewarm_ms"),
        ("S3 : réveil → succès", "S3", "wake_to_terminal_ms"),
    ):
        possible = [r for r in rows if r["kind"] == kind and
                    r["first_touch"] == "pass" and r[metric]]
        if possible:
            best = min(possible, key=lambda r: int(r[metric]))
            parts.append(f'| {label} | **{seconds(best[metric])} s** | '
                         f'{best["version"]} | {best["trial_id"]} |')
    parts += ["", "## Fiabilité par version", "",
              "| Version | Situation | 1ère pose | Certification de ce scénario |",
              "|---|---|---:|---|"]
    for version in dict.fromkeys(r["version"] for r in rows):
        for kind in ("LOCK", "S3", "COLD", "REBOOT"):
            group = [r for r in rows if r["version"] == version and
                     r["kind"] == kind and r["first_touch"] != "unknown"]
            if not group:
                continue
            good = sum(r["first_touch"] == "pass" for r in group)
            min_runs = 5 if kind == "LOCK" else 3
            status = ("ÉCHEC de première pose" if good != len(group) else
                      "Nombre insuffisant" if len(group) < min_runs else
                      "Scénario réussi ; autres gates ouverts")
            parts.append(f"| {version} | {kind} | {good}/{len(group)} | {status} |")
    parts += ["", "## Essais horodatés", "",
              "| Essai | Heure (Europe/Paris) | Version | Mode | Poses | Première | Préwarm S3 | FDT | "
              "Doigt→succès | KDE→succès | Réveil→succès | Score |",
              "|---|---|---|---|---:|---|---:|---:|---:|---:|---:|---:|"]
    for r in rows:
        parts.append(f'| {r["trial_id"]} | {r["datetime_local"]} | {r["version"]} | {r["kind"]} | '
                     f'{r["poses"]} | {r["first_touch"]} | '
                     f'{seconds(r["prewarm_ms"])} | {seconds(r["fast_ready_ms"])} | '
                     f'{seconds(r["press_to_terminal_ms"])} | '
                     f'{seconds(r["greeter_to_terminal_ms"])} | '
                     f'{seconds(r["wake_to_terminal_ms"])} | '
                     f'{r["score"] or "—"}/7 |')
    parts += ["", "## Registre antérieur au 13 octobre 2026", "", "L'archive d'origine couvre **31 configurations**, **125 références Git**, et **14 classements historiques**.", "Voir [le tableau historique](LEGACY_HALL_OF_FAME_20261013.md) et [le résumé JSON](LEGACY_HALL_OF_FAME_20261013.json). Les 31 configurations ne sont pas 31 essais indépendants.", ""]
    old = [int(r["press_to_terminal_ms"]) for r in rows
           if r["version"] == "rel71.55" and r["kind"] == "LOCK"
           and r["first_touch"] == "pass" and r["press_to_terminal_ms"]
           and r["datetime_local"] >= "2026-10-24T15:15"]
    new = [int(r["press_to_terminal_ms"]) for r in rows
           if r["version"] == "rel71.58" and r["kind"] == "LOCK"
           and r["first_touch"] == "pass" and r["press_to_terminal_ms"]]
    if old and new:
        before, after = statistics.median(old), statistics.median(new)
        parts += ["", "## Comparaison anti-régression des LOCK",
                  "", f"- rel71.55 après rollback : médiane **{before/1000:.3f} s** "
                  f"sur {len(old)} essais (doigt → authentification).",
                  f"- rel71.58 : **{after/1000:.3f} s** sur {len(new)} essai(s). "
                  f"Écart **{(after/before-1)*100:+.1f} %** ; "
                  "pas de gain revendiqué et échantillon insuffisant."]
    parts += [
        "", "## Gates obligatoires et politique d'enregistrement",
        "",
        "1. Ne pas mélanger différentes versions ou installations. "
        "Un score réussi après plusieurs poses ne compte PAS comme première pose.",
        "2. Pour une même version : 5 LOCK consécutifs première pose, "
        "3 vrais S3 deep (dont une suspension pendant Verify), "
        "3 cold boots, 3 reboots, doigt inconnu rejeté, "
        "mot de passe valide, aucune désynchronisation du capteur.",
        "3. Ne pas relâcher le seuil de correspondance **7**, "
        "ne pas supprimer le nettoyage TLS, ne pas imposer de reboot/veille "
        "sans la participation de l'utilisateur.",
        "4. Comparer uniquement des mesures de même nature. "
        "FDT FAST_READY ne mesure ni l'affichage du greeter ni "
        "le délai total de sortie de veille. Aucun S3 échoué ne "
        "peut détenir un record de vitesse.",
        "5. À chaque essai : lire journal, calculer les timings, "
        "attester les poses avec l'utilisateur, ajouter une ligne au CSV, "
        "réviser la fiche de preuve, régénérer cette page avec l'option "
        "--write du script gxfp51a0-hall-of-fame.py, "
        "lancer --check et make verify, commit et push.",
        "",
        "**Statut :** rel71.55 est le rollback de référence mais a échoué "
        "à la première pose sur 1 des 3 S3 mesurés. "
        "rel71.56 S3 rapide rejeté (27,725 s). "
        "rel71.57 instrumentation non installée. "
        "rel71.58 correction S3 en QA, un LOCK validé, 3 S3 observés dont 2 à poses multiples et un dernier à attribution humaine incertaine. "
        "**Aucune version n'est certifiée production.**",
        "",
    ]
    return "\n".join(parts)


def main():
    p = argparse.ArgumentParser(description=__doc__)
    mode = p.add_mutually_exclusive_group()
    mode.add_argument("--write", action="store_true")
    mode.add_argument("--check", action="store_true")
    opts = p.parse_args()
    expected = render(load())
    if opts.write:
        PAGE.write_text(expected, encoding="utf-8")
        print("HOF_WRITE_OK")
    elif opts.check:
        assert PAGE.exists() and PAGE.read_text(encoding="utf-8") == expected, \
            "HOF_OUT_OF_SYNC (run with --write)"
        print("HOF_CONSISTENCY_PASS")
    else:
        print(expected, end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
