#!/usr/bin/env python3
"""Check native test report validation without claiming native execution."""

# Copyright (c) 2026 SampleCraft / GO. Game Engine contributors
# SPDX-License-Identifier: MIT

from __future__ import annotations

import tempfile
import unittest
import xml.etree.ElementTree as ET
from pathlib import Path

from go_native_tests import validate_report


class NativeReportTests(unittest.TestCase):
    def test_complete_success(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "report.xml"
            path.write_text(
                '<doctest><OverallResultsAsserts successes="10" failures="0"/>'
                '<OverallResultsTestCases successes="4" failures="0"/></doctest>',
                encoding="utf-8",
            )
            self.assertEqual(validate_report(path, 4), 4)
            with self.assertRaises(ValueError):
                validate_report(path, 5)

    def test_missing_incomplete_and_zero_results_are_not_success(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "report.xml"
            with self.assertRaises(OSError):
                validate_report(path, 1)
            for document in (
                "",
                "<doctest>",
                "<doctest/>",
                '<doctest><OverallResultsAsserts successes="0" failures="0"/>'
                '<OverallResultsTestCases successes="0" failures="0"/></doctest>',
            ):
                path.write_text(document, encoding="utf-8")
                with self.assertRaises((ValueError, ET.ParseError)):
                    validate_report(path, 1)

    def test_failure_totals_cannot_be_hidden_by_passed_cases(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "report.xml"
            for assertions, cases in ((1, 0), (0, 1), (1, 1)):
                path.write_text(
                    f'<doctest><OverallResultsAsserts successes="10" failures="{assertions}"/>'
                    f'<OverallResultsTestCases successes="4" failures="{cases}"/></doctest>',
                    encoding="utf-8",
                )
                with self.assertRaises(ValueError):
                    validate_report(path, 1)


if __name__ == "__main__":
    unittest.main()
