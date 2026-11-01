#!/usr/bin/env python3
import subprocess
import unittest
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
src=(ROOT/"driver/goodix51a0/goodix51a0.c").read_text()
class QualityDiagnostics(unittest.TestCase):
    def test_nonbiometric_telemetry_gated(self):
        b=src.split("gx_score_probe_against_print (FpPrint *tmpl,",1)[1].split("static int\ngx_score_probe_against_gallery",1)[0]
        self.assertIn('if (g_getenv ("GXFP_MATCH_DIAGNOSTICS"))',b)
        self.assertIn("MATCH_QUALITY_QA probe_keypoints=",b)
        self.assertIn("MATCH_QUALITY_QA gallery scores:",b)
        self.assertIn("MATCH_QUALITY_QA pixel diagnostic:",b)
        self.assertIn("gx_sift_pixel_overlap_metrics",b)
    def test_only_observability_changed_from_qa49(self):
        s=subprocess.run(["git","diff","--","fingerprint/driver/goodix51a0/goodix51a0.c"],cwd=ROOT.parent,check=True,capture_output=True,text=True).stdout
        for line in s.splitlines():
            if line.startswith("+") and not line.startswith("+++"):
                self.assertFalse(any(word in line for word in ("GX_MATCH_THRESHOLD +","GX_MATCH_THRESHOLD -","GX_REPOSE_SCORE_CUTOFF")))
if __name__=="__main__": unittest.main(verbosity=2)
