#!/usr/bin/env python3
"""Opt-in live anonymous/free planning probe using synthetic data only.

This is not a live editor tool execution or a visual acceptance test. No project
files, Git changes, repository paths, credentials or environment values are sent.
"""

# Copyright (c) 2026 SampleCraft / GO. Game Engine contributors
# SPDX-License-Identifier: MIT

from __future__ import annotations

import argparse
import json
import subprocess
import tempfile
import urllib.error
import urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
BASE = "https://ai-gateway.kilo.ai/api/v1"


class NoRedirect(urllib.request.HTTPRedirectHandler):
    def redirect_request(self, req, fp, code, msg, headers, newurl):
        raise ValueError("Gateway redirect refused")


def request(endpoint: str, payload: bytes | None = None) -> bytes:
    req = urllib.request.Request(
        BASE + endpoint,
        data=payload,
        headers={
            "Authorization": "Bearer anonymous",
            "Content-Type": "application/json",
            "User-Agent": "opencode-kilo-provider",
            "X-KILOCODE-EDITORNAME": "GO. Game Engine (Code Agentics)",
        },
    )
    with urllib.request.build_opener(NoRedirect).open(req, timeout=45) as response:
        body = response.read(4 * 1024 * 1024 + 1)
    if len(body) > 4 * 1024 * 1024:
        raise ValueError("Gateway response exceeded limit")
    return bytes(body)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--allow-live-synthetic-request", action="store_true", required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    report = {"scope": "synthetic live planning only; no editor execution", "success": False, "detail": ""}
    try:
        with tempfile.TemporaryDirectory(prefix="go-live-kilo-") as tmp:
            probe = Path(tmp) / "probe"
            module = ROOT / "modules/go_suite"
            subprocess.run(
                [
                    "g++",
                    "-std=c++17",
                    "-I",
                    str(module),
                    *[
                        str(module / name)
                        for name in (
                            "go_suite_core.cpp",
                            "go_kilo_models.cpp",
                            "go_agentics_path.cpp",
                            "go_agentics_tools.cpp",
                        )
                    ],
                    str(ROOT / "misc/scripts/go_live_kilo_probe.cpp"),
                    "-o",
                    str(probe),
                ],
                check=True,
                timeout=120,
            )
            catalog = request("/models")
            prepared = subprocess.run([str(probe), "prepare"], input=catalog, capture_output=True, timeout=10)
            if prepared.returncode:
                raise ValueError(prepared.stderr.decode("utf-8", errors="replace"))
            response = request("/chat/completions", prepared.stdout)
            checked = subprocess.run([str(probe), "check"], input=response, capture_output=True, timeout=10)
            if checked.returncode:
                raise ValueError(checked.stderr.decode("utf-8", errors="replace"))
            report["success"] = True
            report["detail"] = checked.stdout.decode("utf-8", errors="replace").strip()
    except urllib.error.HTTPError as error:
        # Do not leak arbitrary gateway response bodies or retry anonymous limits.
        report["detail"] = f"Gateway HTTP {error.code}; no retries or authenticated/paid fallback"
    except (OSError, ValueError, subprocess.SubprocessError) as error:
        report["detail"] = str(error)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(report))
    if not report["success"]:
        message = str(report["detail"]).replace("%", "%25").replace("\r", "%0D").replace("\n", "%0A")
        print(f"::error title=Live anonymous Kilo probe::{message}")
    return 0 if report["success"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
