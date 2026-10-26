#!/usr/bin/env python3
"""Test the Kilo free-model filter against a local mock HTTP server.

The C++ probe uses the exact `go_kilo_models.cpp` logic that the Godot client
uses. It only connects to 127.0.0.1 on an ephemeral port; no live Kilo Gateway
request is made.

GO.-owned test code is MIT licensed.
"""

# Copyright (c) 2026 SampleCraft / GO. Game Engine contributors
# SPDX-License-Identifier: MIT

from __future__ import annotations

import argparse
import json
import subprocess
import sys
import tempfile
import threading
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
MODULE = ROOT / "modules/go_suite"
PROBE = ROOT / "misc/scripts/go_kilo_probe.cpp"
SOURCES = [
	MODULE / "go_kilo_models.cpp",
	MODULE / "go_suite_core.cpp",
	PROBE,
]

MODELS_RESPONSE = {
	"data": [
		{"id": "kilo-auto/free", "name": "Kilo Auto", "isFree": True, "supported_parameters": ["tools"]},
		{"id": "zero/model", "name": "Zero", "pricing": {"prompt": "0", "completion": "0.0"}, "supported_parameters": ["tools"]},
		{"id": "paid/model", "name": "Paid", "pricing": {"prompt": "0.001", "completion": "0.002"}},
		{"id": "flag-free/model", "name": "Flag Free", "isFree": True},
	],
}


class MockHandler(BaseHTTPRequestHandler):
	protocol_version = "HTTP/1.1"

	def log_message(self, format: str, *args: object) -> None:
		return

	def _send(self, status: int, body: bytes, content_type: str, extra_headers: dict[str, str] | None = None) -> None:
		self.send_response(status)
		self.send_header("Content-Type", content_type)
		self.send_header("Content-Length", str(len(body)))
		self.send_header("Connection", "close")
		for key, value in (extra_headers or {}).items():
			self.send_header(key, value)
		self.end_headers()
		self.wfile.write(body)
		self.wfile.flush()

	def do_GET(self) -> None:  # noqa: N802 - http.server API
		if self.path == "/models":
			self._send(200, json.dumps(MODELS_RESPONSE).encode("utf-8"), "application/json")
		elif self.path == "/rate-limit":
			body = json.dumps({"error": {"message": "slow down"}}).encode("utf-8")
			self._send(429, body, "application/json", {"Retry-After": "17"})
		else:
			self._send(404, b"not found", "text/plain")


def compile_probe(compiler: str, executable: Path) -> None:
	command = [
		compiler, "-std=c++17", "-Wall", "-Wextra", "-Wpedantic",
		"-I", str(MODULE),
		*[str(path) for path in SOURCES],
		"-o", str(executable),
	]
	result = subprocess.run(command, cwd=ROOT, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
	if result.returncode != 0:
		print("C++ probe compilation failed:", file=sys.stderr)
		print(result.stdout, file=sys.stderr)
		raise SystemExit(1)
	if result.stdout.strip():
		print(result.stdout.strip())


def main() -> int:
	parser = argparse.ArgumentParser(description=__doc__)
	parser.add_argument("--compiler", default="g++")
	args = parser.parse_args()

	server = ThreadingHTTPServer(("127.0.0.1", 0), MockHandler)
	server.daemon_threads = True
	thread = threading.Thread(target=server.serve_forever, daemon=True)
	thread.start()
	try:
		with tempfile.TemporaryDirectory(prefix="go-kilo-probe-") as tmp:
			executable = Path(tmp) / "go_kilo_probe"
			compile_probe(args.compiler, executable)
			url = f"http://127.0.0.1:{server.server_address[1]}"
			result = subprocess.run(
				[str(executable), url],
				cwd=ROOT,
				text=True,
				stdout=subprocess.PIPE,
				stderr=subprocess.STDOUT,
				timeout=30,
			)
			output = result.stdout
			print(output.strip())
			if result.returncode != 0:
				print("Kilo mock probe failed", file=sys.stderr)
				return 1
			required = [
				"MODELS_STATUS 200",
				"MODEL kilo-auto/free",
				"MODEL zero/model",
				"MODEL flag-free/model",
				"RATE_LIMIT status=429 retry=17 message=slow down",
			]
			for expected in required:
				if expected not in output:
					print(f"missing expected output: {expected!r}", file=sys.stderr)
					return 1
			if "MODEL paid/model" in output:
				print("paid/model was not filtered from the free model picker", file=sys.stderr)
				return 1
	finally:
		server.shutdown()
		server.server_close()
		thread.join(timeout=5)
	print("OK: Kilo mock gateway free-model filter and 429 parsing")
	return 0


if __name__ == "__main__":
	raise SystemExit(main())
