#!/usr/bin/env python3
import pathlib
import re
import unittest
ROOT=pathlib.Path(__file__).resolve().parents[3]
PKG=ROOT/'fingerprint/packaging/arch/PKGBUILD'
PKGREL=re.search(r'^pkgrel=(\d+\.\d+)$',PKG.read_text(),re.M).group(1)
CURRENT='rel'+PKGREL
class CurrentRelease(unittest.TestCase):
 def test_current_mainline_in_navigation_and_agent_contract(self):
  for file in ('README.md','AGENTS.md','docs/PROJECT_MAP.md',
               'fingerprint/HANDOFF_CURRENT.md','fingerprint/README.md'):
   with self.subTest(file=file):
    front=(ROOT/file).read_text().splitlines()[:28]
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
    self.assertIn('rel71.69',(ROOT/file).read_text())
  self.assertIn('S4',(ROOT/'fingerprint/docs/rel71.74-production-20261028.md').read_text())
if __name__=='__main__':unittest.main()
