#!/usr/bin/env bash
set -euo pipefail
awslocal s3 mb s3://odin-level0 2>/dev/null || true