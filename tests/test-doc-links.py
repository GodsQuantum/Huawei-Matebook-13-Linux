#!/usr/bin/env python3
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
LINK = re.compile(r"\[[^\]]*\]\(([^)]+)\)")
broken = []

for path in ROOT.rglob("*.md"):
    if ".git" in path.parts:
        continue
    text = path.read_text(encoding="utf-8", errors="replace")
    for match in LINK.finditer(text):
        dest = match.group(1).strip().split()[0]
        if not dest or dest.startswith(("http://", "https://", "mailto:", "#", "app://")):
            continue
        dest = dest.split("#", 1)[0]
        if not dest:
            continue
        target = (path.parent / dest).resolve()
        if not target.exists():
            broken.append((path.relative_to(ROOT), dest))

if broken:
    for path, dest in broken:
        print(f"BROKEN_DOC_LINK {path}: {dest}")
    raise SystemExit(1)

print("PUBLIC_DOC_LINKS=PASS")
