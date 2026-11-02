#!/usr/bin/env python3
"""Run native GO. tests with isolated caches and authoritative doctest XML.

A GUI executable returning zero or an empty console log is not a successful test.
All failed or missing reports produce API-readable GitHub error annotations.
"""

# Copyright (c) 2026 SampleCraft / GO. Game Engine contributors
# SPDX-License-Identifier: MIT

from __future__ import annotations

import argparse
import os
import subprocess
import tempfile
import xml.etree.ElementTree as ET
from pathlib import Path

from go_annotate_build_log import escape_command, summarize


def validate_report(path: Path, minimum: int) -> int:
    root = ET.parse(path).getroot()
    cases = root.find("OverallResultsTestCases")
    assertions = root.find("OverallResultsAsserts")
    if root.tag != "doctest" or cases is None or assertions is None:
        raise ValueError("Missing complete doctest totals")
    passed = int(cases.attrib["successes"])
    if int(cases.attrib["failures"]) != 0 or int(assertions.attrib["failures"]) != 0:
        raise ValueError("Native assertions or test cases failed")
    if passed < minimum or int(assertions.attrib["successes"]) < 1:
        raise ValueError(f"Only {passed} tests completed; at least {minimum} with assertions are required")
    return passed


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--editor", type=Path, required=True)
    parser.add_argument("--suite", required=True)
    parser.add_argument("--minimum", type=int, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    if args.minimum < 1:
        parser.error("--minimum must be positive")
    output = args.output.resolve()
    output.parent.mkdir(parents=True, exist_ok=True)
    report = output.with_suffix(".xml")
    report.unlink(missing_ok=True)  # Never accept a report from a previous invocation.
    error = ""
    try:
        with tempfile.TemporaryDirectory(prefix="go-native-tests-") as temporary:
            env = os.environ.copy()
            for key in ("HOME", "APPDATA", "LOCALAPPDATA", "XDG_DATA_HOME", "XDG_CONFIG_HOME", "XDG_CACHE_HOME"):
                directory = Path(temporary) / key.lower()
                directory.mkdir()
                env[key] = str(directory)
            with output.open("w", encoding="utf-8") as log:
                result = subprocess.run(
                    [
                        str(args.editor.resolve()),
                        "--headless",
                        "--test",
                        f"--test-case=[GO.][{args.suite}]*",
                        "--reporters=xml",
                        f"--out={report}",
                    ],
                    env=env,
                    stdout=log,
                    stderr=subprocess.STDOUT,
                    timeout=300,
                    check=False,
                )
            if result.returncode != 0:
                raise ValueError(f"Native test process exited with {result.returncode}")
            passed = validate_report(report, args.minimum)
            print(f"OK: {args.suite}: {passed} native tests passed (XML verified)")
    except (OSError, ValueError, KeyError, ET.ParseError, subprocess.TimeoutExpired) as exc:
        error = str(exc)
    if error:
        text = output.read_text(encoding="utf-8", errors="replace") if output.is_file() else "No console log"
        if report.is_file():
            text += "\n" + report.read_text(encoding="utf-8", errors="replace")
        summary, _ = summarize(text)
        message = f"{args.suite}: {error}\n{summary}"[:60000]
        print(f"::error title=GO. native test failure::{escape_command(message)}")
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
