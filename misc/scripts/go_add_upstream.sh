#!/usr/bin/env bash
# GO. helper: configure the Godot upstream remote in a fresh clone.
set -euo pipefail
repo=${1:-https://github.com/godotengine/godot.git}
if git remote get-url upstream >/dev/null 2>&1; then
  echo "upstream already configured: $(git remote get-url upstream)"
else
  git remote add upstream "$repo"
fi
git fetch upstream tag 4.7.2-stable
echo "GO. base: $(git rev-parse 4.7.2-stable)"
