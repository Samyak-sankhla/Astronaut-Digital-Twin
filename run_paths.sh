#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

export DT_BIOGEARS_ROOT="${DT_BIOGEARS_ROOT:-$SCRIPT_DIR}"
export DT_BIOGEARS_BUILD="${DT_BIOGEARS_BUILD:-$DT_BIOGEARS_ROOT/build}"

if [[ -d "$DT_BIOGEARS_ROOT/biogears/runtime" ]]; then
	DEFAULT_RUNTIME="$DT_BIOGEARS_ROOT/biogears/runtime"
	DEFAULT_LIB="$DT_BIOGEARS_ROOT/biogears/lib"
	DEFAULT_BIN="$DT_BIOGEARS_ROOT/biogears/bin"
else
	DEFAULT_RUNTIME="${HOME}/opt/biogears/runtime"
	DEFAULT_LIB="${HOME}/opt/biogears/lib"
	DEFAULT_BIN="${HOME}/opt/biogears/bin"
fi

export DT_BIOGEARS_RUNTIME="${DT_BIOGEARS_RUNTIME:-$DEFAULT_RUNTIME}"
export DT_BIOGEARS_LIB="${DT_BIOGEARS_LIB:-$DEFAULT_LIB}"
export DT_BIOGEARS_BIN="${DT_BIOGEARS_BIN:-$DEFAULT_BIN}"

if [[ -x "$DT_BIOGEARS_ROOT/.venv/bin/python" ]]; then
	DEFAULT_PYTHON="$DT_BIOGEARS_ROOT/.venv/bin/python"
else
	DEFAULT_PYTHON="python3"
fi

export DT_BIOGEARS_PYTHON="${DT_BIOGEARS_PYTHON:-$DEFAULT_PYTHON}"
export DT_FRONTEND_PORT="${DT_FRONTEND_PORT:-5500}"
