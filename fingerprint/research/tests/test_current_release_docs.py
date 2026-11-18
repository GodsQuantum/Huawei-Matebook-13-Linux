#!/usr/bin/env python3
import pathlib
import re
import unittest
ROOT=pathlib.Path(__file__).resolve().parents[3]
PKG=ROOT/'fingerprint/packaging/arch/PKGBUILD'
PKGVER=re.search(r'^pkgver=(.+)$',PKG.read_text(),re.M).group(1)
PKGREL=re.search(r'^pkgrel=(\d+(?:\.\d+)?)$',PKG.read_text(),re.M).group(1)
# Experimental QA packages must not redefine mainline release pointers.
CURRENT=('rel'+PKGREL if '.' in PKGREL else
         re.search(r'releases/tag/matebook13-(rel\d+\.\d+)',
                   (ROOT/'README.md').read_text()).group(1))
assert '.' in PKGREL or re.fullmatch(r'1\.94\.100\.goodix51a0\.qa\d+', PKGVER)
class CurrentRelease(unittest.TestCase):
 def test_current_mainline_in_navigation_and_agent_contract(self):
  for file in ('README.md','AGENTS.md','docs/PROJECT_MAP.md',
               'fingerprint/HANDOFF_CURRENT.md','fingerprint/README.md'):
   with self.subTest(file=file):
    path=ROOT/file
    # The sensitive local handoff is intentionally excluded from the public
    # GitHub source (privacy scrub). Validate it only when actually present.
    if file=='fingerprint/HANDOFF_CURRENT.md' and not path.exists():
     continue
    front=path.read_text().splitlines()[:28]
    self.assertIn(CURRENT,' '.join(front),file+' current release pointer')
 def test_old_version_not_marked_only_or_current(self):
  for file in ('README.md','AGENTS.md','docs/PROJECT_MAP.md'):
   with self.subTest(file=file):
    content=(ROOT/file).read_text()
    for bad in ('only released reference is rel71.69',
                'Current stable reference for GXFP51A0/ST411: rel71.69',
                'Default fingerprint install: rel71.69.',
                '**Fingerprint** | **rel71.69 EXPERIMENTAL',
                'fingerprint-rel71.69%20EXPERIMENTAL'):
     self.assertNotIn(bad,content)
 def test_live_release_badge_points_to_current_tag(self):
  text=(ROOT/'README.md').read_text()
  self.assertIn('releases/tag/matebook13-'+CURRENT,text)
  self.assertIn('fingerprint-'+CURRENT,text)
 def test_release_warning_not_whitewashed(self):
  for file in ('README.md','fingerprint/README.md','fingerprint/HANDOFF_CURRENT.md'):
   with self.subTest(file=file):
    path=ROOT/file
    if file=='fingerprint/HANDOFF_CURRENT.md' and not path.exists():
     continue
    self.assertIn('rel71.69',path.read_text())
  self.assertIn('S4',(ROOT/'fingerprint/docs/rel71.74-production-20261028.md').read_text())
 def test_current_source_vs_published_release(self):
  pkgver=re.search(r'^pkgver=(.+)$',PKG.read_text(),re.M).group(1)
  self.assertIn('goodix51a0.qa63',pkgver)
  self.assertEqual((ROOT/'fingerprint/STABLE_RELEASE').read_text().strip(),'rel71.74')
  for file in ('README.md','README.FR.md','README.ZH-CN.md',
               'fingerprint/README.md','fingerprint/README.FR.md','fingerprint/README.ZH-CN.md'):
   with self.subTest(file=file):
    text=(ROOT/file).read_text()
    self.assertIn('QA63.1',text)
    self.assertIn('rel71.74',text)
    self.assertIn('gxfp51a0-enroll',text)
    self.assertIn('fprintd',text)
    self.assertIn('KDE',text)
  for file in ('AGENTS.md','docs/PROJECT_MAP.md'):
   with self.subTest(file=file):
    text=(ROOT/file).read_text()
    self.assertIn('QA63.1',text)
    self.assertIn('rel71.74',text)
  for file in ('README.md','README.FR.md','README.ZH-CN.md'):
   with self.subTest(file=file):
    text=(ROOT/file).read_text()
    self.assertIn('releases/tag/matebook13-rel71.74',text)
    self.assertNotIn('fingerprint-rel71.69%20EXPERIMENTAL',text)

 def test_published_asset_urls_and_historical_constraints(self):
  text=(ROOT/'fingerprint/DRIVER_ONLY.md').read_text()
  base='https://github.com/GodsQuantum/Huawei-Matebook-13-Linux/releases/download/matebook13-rel71.74/'
  self.assertIn(base+'gxfp51a0-driver-only-rel71.74.tar.gz',text)
  self.assertIn(base+'libfprint-goodix51a0-1.94.100.goodix51a0-71.74-x86_64.pkg.tar.zst',text)
  self.assertNotIn('from the rel71.69 GitHub release',text)
  self.assertIn('predate',text)
  zh=(ROOT/'fingerprint/README.ZH-CN.md').read_text()
  self.assertNotIn('推荐使用 rel71.30 预览版',zh)
  en=(ROOT/'fingerprint/README.md').read_text()
  self.assertNotIn('rel71.30 is the default install.',en)

 def test_active_markdown_relative_links_exist(self):
  import urllib.parse
  for file in ('README.md','README.FR.md','README.ZH-CN.md',
               'fingerprint/README.md','fingerprint/README.FR.md',
               'fingerprint/README.ZH-CN.md','fingerprint/DRIVER_ONLY.md',
               'fingerprint/docs/README.md','docs/PROJECT_MAP.md'):
   p=ROOT/file
   for match in re.finditer(r'(?<!!)\[[^][]+\]\(([^)]+)\)',p.read_text()):
    target=match.group(1).strip().split(' ')[0].strip('<>')
    if not target or target.startswith(('http:','https:','#','mailto:')):continue
    if '{' in target:continue
    resolved=urllib.parse.unquote(target.split('#')[0].split('?')[0])
    with self.subTest(file=file,target=target):
     self.assertTrue((p.parent/resolved).exists())

if __name__=='__main__':unittest.main()
