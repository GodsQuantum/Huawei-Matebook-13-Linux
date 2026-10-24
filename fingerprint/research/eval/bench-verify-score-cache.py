#!/usr/bin/env python3
"""Synthetic, no-biometric matcher CPU cost of 20-view duplicated verification.

This only measures SIGFM CPU comparison; it deliberately does not claim to
measure SPI transport, TLS cleanup, KDE or real first-touch FRR/FAR.
"""
import ctypes
import json
import statistics
import subprocess
import tempfile
import time
from pathlib import Path

HERE = Path(__file__).resolve().parent
N_VIEWS = 20
RUNS = 25

with tempfile.TemporaryDirectory(prefix="gxfp-score-cache-bench-") as td:
    so = Path(td) / "gq-sigfm.so"
    subprocess.run([str(HERE / "build-gq-sigfm-fpeval.sh"), str(so)],
                   check=True, stdout=subprocess.DEVNULL)
    lib = ctypes.CDLL(str(so))
    lib.fpeval_extract.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_int, ctypes.c_int]
    lib.fpeval_extract.restype = ctypes.c_void_p
    lib.fpeval_score.argtypes = [ctypes.c_void_p, ctypes.c_void_p]
    lib.fpeval_score.restype = ctypes.c_double
    lib.fpeval_free.argtypes = [ctypes.c_void_p]

    def features(idx):
        w, h = 80, 64
        data = (ctypes.c_uint8 * (w*h))()
        for y in range(h):
            for x in range(w):
                data[y*w+x] = (x*17 + y*29 + ((x ^ y)*7)
                               + idx*13 + ((x*y*(idx+3)) % 31)) & 255
        ptr = lib.fpeval_extract(data, w, h)
        if not ptr:
            raise RuntimeError("synthetic feature extractor rejected a frame")
        return ptr

    gallery = [features(i) for i in range(N_VIEWS)]
    probe = features(0)
    def score_gallery():
        return max(lib.fpeval_score(probe, view) for view in gallery)

    try:
        expected = score_gallery()
        # The cached score must reproduce the raw match result bit-for-bit.
        # This does not model the app's conditional photometric rescue.
        for _ in range(4):
            assert score_gallery() == expected
        baseline_ns, cached_ns = [], []
        for _ in range(RUNS):
            start = time.perf_counter_ns()
            first = score_gallery()
            second = score_gallery()
            baseline_ns.append(time.perf_counter_ns() - start)
            assert first == second == expected
            start = time.perf_counter_ns()
            once = score_gallery()
            reused = once
            cached_ns.append(time.perf_counter_ns() - start)
            assert reused == expected

        before = statistics.median(baseline_ns) / 1_000_000
        after = statistics.median(cached_ns) / 1_000_000
        assert before > after > 0
        print(json.dumps({
            "schema": "gxfp51a0-synthetic-score-cache-cpu-v1",
            "gallery_views": N_VIEWS, "rounds": RUNS,
            "synthetic_only": True, "real_fingerprint_frames": 0,
            "includes_spi_or_tls_or_pam": False,
            "score_equivalent": True,
            "median_double_match_ms": round(before, 4),
            "median_single_match_ms": round(after, 4),
            "median_cpu_match_saved_ms": round(before - after, 4),
            "matcher_cpu_speedup_factor": round(before / after, 3),
            "warning": "CPU microbenchmark does not predict end-to-end KDE unlock",
        }, indent=2))
    finally:
        lib.fpeval_free(probe)
        for f in gallery:
            lib.fpeval_free(f)
