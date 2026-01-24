#!/usr/bin/env bash
set -euo pipefail

# This script runs inside the dev container after it is created.

cd /workspace

# Ensure uv is on PATH (installed via devcontainer feature)
export PATH="${HOME}/.local/bin:${PATH}"

# Ensure a Python version compatible with the project (>=3.13)
# uv python install 3.13

# Create/sync project environment with dev dependencies
uv sync --all-groups

echo "post-create.sh completed (uv environment ready)."
