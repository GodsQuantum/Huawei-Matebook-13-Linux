"""TDD: broaden rotation only under a validated, explicitly enabled QA63 gate."""
import pathlib, re, unittest
R = pathlib.Path(__file__).resolve().parents[2]
src = (R / "driver/goodix51a0/goodix51a0.c").read_text(encoding="utf-8")
conf = (R / "packaging/arch/fprintd-goodix51a0.conf").read_text(encoding="utf-8")

class QA63Rotation(unittest.TestCase):
    def test_wide_rotation_opt_in_and_bounded(self):
        self.assertIn('GXFP_WIDE_ROTATION_QA', src)
        self.assertRegex(src, r'additional_angles\[\]\s*=\s*\{\s*-18\.0,\s*18\.0\s*\}')
        self.assertIn('const double angles[] = { -12.0, 12.0, -6.0, 6.0 }', src)
        self.assertIn('GXFP_WIDE_ROTATION_QA=1', conf)

    def test_no_new_authorization_path(self):
        part = src.split('/* QA54: extra orientation coverage', 1)[1].split('if (best >= GX_PIXEL_RESCUE_CANDIDATE_BEST_MIN', 1)[0]
        for token in ('rotation_verify', 'best >= 3 && best < GX_MATCH_THRESHOLD',
                      'score < GX_MATCH_THRESHOLD', 'gx_sift_pixel_overlap_metrics',
                      'GX_PIXEL_RESCUE_MIN_OVERLAP', 'GX_PIXEL_RESCUE_MIN_ZNCC_MILLI',
                      'GX_PIXEL_RESCUE_MIN_AGREE_PERMILLE'):
            self.assertIn(token, part)
        self.assertIn('return GX_MATCH_THRESHOLD;', part)
        self.assertNotIn('score = score +', part)
        self.assertNotIn('gx_capture_frame_ex', part)

    def test_no_more_images_or_biometric_persistence(self):
        self.assertIn('decision=final-for-pose', src)
        self.assertIn('Once a frame passes both quality', src)
        self.assertNotIn('fwrite (rotated', src)
        self.assertNotIn('GXFP_WIDE_ROTATION_QA=1 GXFP_FDT_DOWN_PROOF_QA=0', conf)
        self.assertIn('GXFP_DECOUPLE_ACK_PACING_QA=1', conf)
        self.assertIn('GXFP_ROTATION_RESCUE_QA=1', conf)

if __name__ == "__main__":
    unittest.main()
