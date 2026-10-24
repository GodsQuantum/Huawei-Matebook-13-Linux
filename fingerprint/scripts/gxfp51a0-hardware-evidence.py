#!/usr/bin/env python3
"""Read-only GXFP51A0 deep-S3 evidence report.

Only aggregate event counts, timestamps and scores. Never export fingerprint
frames, enrollment templates, PMK/TLS material, hostnames, usernames, or raw
journal entries. A successful match DOES NOT prove first-touch behavior:
physical placement count must be attested by the person testing the sensor.

Usage:
  python3 scripts/gxfp51a0-hardware-evidence.py
  python3 scripts/gxfp51a0-hardware-evidence.py --fprintd-log test-fixture.log \
      --kernel-log kernel-fixture.log --json
"""
from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
from datetime import datetime, timedelta
from pathlib import Path

ISO_LINE = re.compile(r"^(?P<stamp>\S+)\s+(?P<rest>.*)$")
SCORE = re.compile(r"verified positive terminal.*score=(?P<score>\d+)\s+threshold=(?P<threshold>\d+)")
PRESS = re.compile(r"VERIFY_TRACE physical press (?P<press>\d+)/(?P<total>\d+)")
PARK_ACK = re.compile(r"NATIVE_S3_PARK.*ack=(?P<ack>[01])")
SESSION_SCORE = re.compile(r"AUTH_TRACE.*same-press.*score=(?P<score>\d+)\s+threshold=(?P<threshold>\d+)")


def read_journal(args: list[str]) -> str:
    try:
        run = subprocess.run(
            ["journalctl", "-b", "--no-pager", "-o", "short-iso", *args],
            capture_output=True, text=True, check=False, timeout=20,
        )
    except (OSError, subprocess.TimeoutExpired) as exc:
        raise RuntimeError(f"journal read failed: {type(exc).__name__}") from exc
    if run.returncode != 0:
        raise RuntimeError("journal inaccessible (journalctl returned error)")
    return run.stdout


def parse_events(raw: str, source: str) -> list[dict]:
    events = []
    for line in raw.splitlines():
        match = ISO_LINE.match(line)
        if not match:
            continue
        try:
            stamp = datetime.fromisoformat(match.group("stamp").replace("Z", "+00:00"))
        except ValueError:
            continue
        if stamp.tzinfo is None:
            continue
        message = match.group("rest")
        # All source text stays in-memory, never in the output JSON.
        events.append({"at": stamp, "message": message, "source": source})
    events.sort(key=lambda e: e["at"])
    return events


def last_marker(events: list[dict], marker: str):
    found = [item for item in events if marker in item["message"]]
    return found[-1] if found else None


def summarize_one(entry: dict, exit_event: dict, fprintd: list[dict], end: datetime):
    before = [e for e in fprintd if
              entry["at"] - timedelta(minutes=3) <= e["at"] <= entry["at"]]
    after = [e for e in fprintd if exit_event["at"] <= e["at"] < end]
    park = last_marker(before, "NATIVE_S3_PARK")
    cached = last_marker(before, "NATIVE_S3_CLEAN_REFERENCE_SAVED")
    not_saved = last_marker(before, "NATIVE_S3_CLEAN_REFERENCE_NOT_SAVED")
    resume = last_marker(after, "NATIVE_S3_RESUME_BEGIN")
    ready = last_marker(after, "NATIVE_S3_RESUME_PREWARM_READY")
    fallback = last_marker(after, "NATIVE_S3_RESUME_PREWARM_FALLBACK")
    reused = last_marker(after, "NATIVE_S3_EARLY_FINGER_VERIFIED_REFERENCE")
    quality_reject = sum("CALIBRATION_LIFT_REQUIRED" in e["message"] for e in after)
    assert_fail = sum("fpi_device_get_cancellable" in e["message"] and
                      "assertion" in e["message"] for e in after)
    verified = next(
        ((e, m) for e in after if (m := SCORE.search(e["message"]))),
        None,
    )
    first_press = next((e for e in after if PRESS.search(e["message"])), None)
    first_score = next((e for e in after if SESSION_SCORE.search(e["message"])), None)
    park_match = PARK_ACK.search(park["message"]) if park else None

    def since_wake(event):
        return round((event["at"] - exit_event["at"]).total_seconds(), 2) if event else None

    return {
        "suspend_entry": entry["at"].isoformat(),
        "resume_time": exit_event["at"].isoformat(),
        "deep_sleep_seconds": round((exit_event["at"] - entry["at"]).total_seconds(), 2),
        "sleep_packet_ack": bool(park_match and park_match.group("ack") == "1"),
        "clean_reference_saved": cached is not None,
        "clean_reference_not_saved": not_saved is not None,
        "native_resume_called": resume is not None,
        "prewarm_ready_seconds_after_wake": since_wake(ready),
        "prewarm_fallback": fallback is not None,
        "cached_reference_used": reused is not None,
        "calibration_lift_required_count": quality_reject,
        "idle_cancellable_assertions": assert_fail,
        "first_verify_armed_seconds_after_wake": since_wake(first_press),
        "first_scored_image_seconds_after_wake": since_wake(first_score),
        "successful_match_seconds_after_wake": since_wake(verified[0]) if verified else None,
        "matched_score": int(verified[1].group("score")) if verified else None,
        "match_threshold": int(verified[1].group("threshold")) if verified else None,
        "first_physical_placement": "unverified_requires_human_report",
        "certified": False,
    }


def report(fprintd_raw: str, kernel_raw: str, since: datetime | None = None) -> dict:
    fp = parse_events(fprintd_raw, "fingerprint")
    k = parse_events(kernel_raw, "kernel")
    boundaries = []
    entry = None
    for event in k:
        if "PM: suspend entry (deep)" in event["message"]:
            entry = event
        elif "PM: suspend exit" in event["message"] and entry is not None:
            if event["at"] >= entry["at"]:
                boundaries.append((entry, event))
            entry = None
    if since is not None and (since.tzinfo is None or since.utcoffset() is None):
        raise ValueError("since must be timezone-aware")
    sessions = []
    for index, (start, end) in enumerate(boundaries):
        # Never attribute a cycle that started under an earlier build to this build.
        if since is not None and start["at"] < since:
            continue
        # A positive match hours after a resume belongs to normal login,
        # not this S3 test. Inspect only the first two minutes after wake,
        # and never cross into another kernel suspend cycle.
        deadline = end["at"] + timedelta(minutes=2)
        next_start = boundaries[index+1][0]["at"] if index+1 < len(boundaries) else deadline
        sessions.append(summarize_one(start, end, fp, min(deadline, next_start)))
    return {
        "driver": "GXFP51A0",
        "analysis_since": since.isoformat() if since is not None else None,
        "records_examined": len(sessions),
        "deep_s3_sessions": sessions,
        "cold_boot_reboot_verdict": "unverified_requires_human_test",
        "first_touch_verdict": "unverified_requires_human_report",
        "hardware_certification": "PENDING",
        "note": "Software log success never proves first-touch, FAR or cold-boot behavior.",
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--fprintd-log", type=Path)
    parser.add_argument("--kernel-log", type=Path)
    parser.add_argument("--json", action="store_true")
    parser.add_argument("--since", metavar="ISO8601",
                        help="Only analyze S3 cycles beginning at or after this timezone-aware driver install timestamp")
    a = parser.parse_args()
    if bool(a.fprintd_log) != bool(a.kernel_log):
        parser.error("--fprintd-log and --kernel-log must be given together")
    since = None
    if a.since:
        try:
            since = datetime.fromisoformat(a.since.replace("Z", "+00:00"))
        except ValueError:
            parser.error("--since must be a valid ISO 8601 timestamp with timezone")
        if since.tzinfo is None or since.utcoffset() is None:
            parser.error("--since must include a timezone offset")
    try:
        fptext = a.fprintd_log.read_text() if a.fprintd_log else read_journal(["-u", "fprintd"])
        ktext = a.kernel_log.read_text() if a.kernel_log else read_journal(["-k"])
        result = report(fptext, ktext, since=since)
    except (OSError, RuntimeError) as exc:
        print(f"Unable to assess journal: {exc}", file=sys.stderr)
        return 2
    print(json.dumps(result, indent=2, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    sys.exit(main())
