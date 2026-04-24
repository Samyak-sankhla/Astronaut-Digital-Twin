#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/run_paths.sh"

exec "${DT_BIOGEARS_PYTHON}" -m http.server "${DT_FRONTEND_PORT}" --directory "$DT_BIOGEARS_ROOT/frontend"
