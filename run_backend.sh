#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/run_paths.sh"

export LD_LIBRARY_PATH="${DT_BIOGEARS_LIB}:${DT_BIOGEARS_BIN}:${LD_LIBRARY_PATH:-}"
export BIOGEARS_RUNTIME="${DT_BIOGEARS_RUNTIME}"

exec "${DT_BIOGEARS_BUILD}/biogears_digital_twin"
