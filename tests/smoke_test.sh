#!/usr/bin/env bash
# Smoke tests for the BioGears digital twin REST API.
# Usage: ./tests/smoke_test.sh [binary_path]
#   binary_path defaults to ./build/biogears_digital_twin
#
# Requires: curl, jq
# The script starts the binary, waits for it to be ready, runs all endpoint
# checks, then kills the binary regardless of test outcome.

set -euo pipefail

BINARY="${1:-./build/biogears_digital_twin}"
BASE_URL="http://127.0.0.1:8080"
READY_TIMEOUT=15  # seconds to wait for the server to start
PASS=0
FAIL=0

# ── helpers ───────────────────────────────────────────────────────────────────

die() { echo "FATAL: $*" >&2; exit 1; }

check_dep() { command -v "$1" >/dev/null 2>&1 || die "'$1' not found in PATH"; }

pass() { echo "  PASS: $1"; ((++PASS)); }
fail() { echo "  FAIL: $1"; ((++FAIL)); }

assert_status() {
  local label="$1" expected="$2" actual="$3"
  if [[ "$actual" == "$expected" ]]; then
    pass "$label (HTTP $actual)"
  else
    fail "$label — expected HTTP $expected, got HTTP $actual"
  fi
}

assert_json_field() {
  local label="$1" body="$2" field="$3"
  if echo "$body" | jq -e "has(\"$field\")" >/dev/null 2>&1; then
    pass "$label (field '$field' present)"
  else
    fail "$label — field '$field' missing in: $body"
  fi
}

check_dep curl
check_dep jq

[[ -x "$BINARY" ]] || die "binary not found or not executable: $BINARY"

# ── start binary ──────────────────────────────────────────────────────────────

echo "Starting binary: $BINARY"
"$BINARY" &
SERVER_PID=$!
trap 'echo "Stopping server (PID $SERVER_PID)"; kill "$SERVER_PID" 2>/dev/null; wait "$SERVER_PID" 2>/dev/null; exit' EXIT INT TERM

# Wait until the server is accepting connections
echo "Waiting for server to be ready (up to ${READY_TIMEOUT}s)..."
deadline=$(( $(date +%s) + READY_TIMEOUT ))
until curl -sf "$BASE_URL/state" >/dev/null 2>&1; do
  if [[ $(date +%s) -ge $deadline ]]; then
    die "Server did not become ready within ${READY_TIMEOUT}s"
  fi
  sleep 0.5
done
echo "Server ready."
echo

# ── tests ─────────────────────────────────────────────────────────────────────

echo "=== GET /state ==="
response=$(curl -s -o /tmp/smoke_body.json -w "%{http_code}" "$BASE_URL/state")
assert_status "GET /state" 200 "$response"
body=$(cat /tmp/smoke_body.json)
for field in simulation_time_s heart_rate systolic_pressure diastolic_pressure map cardiac_output blood_volume respiration_rate; do
  assert_json_field "GET /state" "$body" "$field"
done
echo

echo "=== POST /event/exercise ==="
response=$(curl -s -o /tmp/smoke_body.json -w "%{http_code}" -X POST \
  -H "Content-Type: application/json" \
  -d '{"intensity": 0.5, "duration": 2}' \
  "$BASE_URL/event/exercise")
assert_status "POST /event/exercise" 200 "$response"
assert_json_field "POST /event/exercise" "$(cat /tmp/smoke_body.json)" "status"

response=$(curl -s -o /tmp/smoke_body.json -w "%{http_code}" -X POST \
  -H "Content-Type: application/json" \
  -d 'not-json' \
  "$BASE_URL/event/exercise")
assert_status "POST /event/exercise bad JSON" 400 "$response"
echo

echo "=== POST /event/hydration ==="
response=$(curl -s -o /tmp/smoke_body.json -w "%{http_code}" -X POST \
  -H "Content-Type: application/json" \
  -d '{"water_liters": 0.5}' \
  "$BASE_URL/event/hydration")
assert_status "POST /event/hydration" 200 "$response"
assert_json_field "POST /event/hydration" "$(cat /tmp/smoke_body.json)" "status"
echo

echo "=== POST /event/drug ==="
response=$(curl -s -o /tmp/smoke_body.json -w "%{http_code}" -X POST \
  -H "Content-Type: application/json" \
  -d '{"substance": "Epinephrine", "concentration_mg_per_ml": 0.01, "rate_ml_per_min": 2.0}' \
  "$BASE_URL/event/drug")
assert_status "POST /event/drug" 200 "$response"
assert_json_field "POST /event/drug" "$(cat /tmp/smoke_body.json)" "status"

response=$(curl -s -o /tmp/smoke_body.json -w "%{http_code}" -X POST \
  -H "Content-Type: application/json" \
  -d 'not-json' \
  "$BASE_URL/event/drug")
assert_status "POST /event/drug bad JSON" 400 "$response"
echo

echo "=== POST /event/microgravity ==="
response=$(curl -s -o /tmp/smoke_body.json -w "%{http_code}" -X POST \
  -H "Content-Type: application/json" \
  -d '{"enabled": true}' \
  "$BASE_URL/event/microgravity")
assert_status "POST /event/microgravity (enable)" 200 "$response"
assert_json_field "POST /event/microgravity (enable)" "$(cat /tmp/smoke_body.json)" "status"

response=$(curl -s -o /tmp/smoke_body.json -w "%{http_code}" -X POST \
  -H "Content-Type: application/json" \
  -d '{"enabled": false}' \
  "$BASE_URL/event/microgravity")
assert_status "POST /event/microgravity (disable)" 200 "$response"
echo

echo "=== POST /event/stress ==="
response=$(curl -s -o /tmp/smoke_body.json -w "%{http_code}" -X POST \
  -H "Content-Type: application/json" \
  -d '{"severity": 0.3}' \
  "$BASE_URL/event/stress")
assert_status "POST /event/stress" 200 "$response"
assert_json_field "POST /event/stress" "$(cat /tmp/smoke_body.json)" "status"

response=$(curl -s -o /tmp/smoke_body.json -w "%{http_code}" -X POST \
  -H "Content-Type: application/json" \
  -d 'bad' \
  "$BASE_URL/event/stress")
assert_status "POST /event/stress bad JSON" 400 "$response"
echo

echo "=== POST /event/nutrition ==="
response=$(curl -s -o /tmp/smoke_body.json -w "%{http_code}" -X POST \
  -H "Content-Type: application/json" \
  -d '{"carbs_g": 50, "protein_g": 30, "fat_g": 10, "water_l": 0.2}' \
  "$BASE_URL/event/nutrition")
assert_status "POST /event/nutrition" 200 "$response"
assert_json_field "POST /event/nutrition" "$(cat /tmp/smoke_body.json)" "status"
echo

echo "=== POST /event/fluid_therapy ==="
response=$(curl -s -o /tmp/smoke_body.json -w "%{http_code}" -X POST \
  -H "Content-Type: application/json" \
  -d '{"fluid": "Saline", "rate_ml_per_min": 50}' \
  "$BASE_URL/event/fluid_therapy")
assert_status "POST /event/fluid_therapy" 200 "$response"
assert_json_field "POST /event/fluid_therapy" "$(cat /tmp/smoke_body.json)" "status"
echo

echo "=== GET /state/history ==="
response=$(curl -s -o /tmp/smoke_body.json -w "%{http_code}" "$BASE_URL/state/history")
assert_status "GET /state/history" 200 "$response"
body=$(cat /tmp/smoke_body.json)
if echo "$body" | jq -e 'if type=="array" then true else false end' >/dev/null 2>&1; then
  pass "GET /state/history (returns JSON array)"
else
  fail "GET /state/history — expected JSON array, got: $body"
fi
echo

echo "=== GET /scenarios ==="
response=$(curl -s -o /tmp/smoke_body.json -w "%{http_code}" "$BASE_URL/scenarios")
assert_status "GET /scenarios" 200 "$response"
body=$(cat /tmp/smoke_body.json)
if echo "$body" | jq -e 'if type=="array" and length>0 then true else false end' >/dev/null 2>&1; then
  pass "GET /scenarios (returns non-empty JSON array)"
else
  fail "GET /scenarios — expected non-empty JSON array, got: $body"
fi
FIRST_SCENARIO=$(echo "$body" | jq -r '.[0].name // empty')
echo "  First scenario: $FIRST_SCENARIO"
echo

echo "=== POST /scenario/run ==="
response=$(curl -s -o /tmp/smoke_body.json -w "%{http_code}" -X POST \
  -H "Content-Type: application/json" \
  -d '{"name": "'"$FIRST_SCENARIO"'"}' \
  "$BASE_URL/scenario/run")
assert_status "POST /scenario/run ($FIRST_SCENARIO)" 200 "$response"
assert_json_field "POST /scenario/run" "$(cat /tmp/smoke_body.json)" "status"

# 409 when one is already running
response=$(curl -s -o /tmp/smoke_body.json -w "%{http_code}" -X POST \
  -H "Content-Type: application/json" \
  -d '{"name": "'"$FIRST_SCENARIO"'"}' \
  "$BASE_URL/scenario/run")
assert_status "POST /scenario/run (duplicate → 409)" 409 "$response"

# 404 for unknown name
response=$(curl -s -o /tmp/smoke_body.json -w "%{http_code}" -X POST \
  -H "Content-Type: application/json" \
  -d '{"name": "DoesNotExist_XYZ"}' \
  "$BASE_URL/scenario/run")
# 409 because one is running; stop first then test 404
curl -s -X POST "$BASE_URL/scenario/stop" >/dev/null
response=$(curl -s -o /tmp/smoke_body.json -w "%{http_code}" -X POST \
  -H "Content-Type: application/json" \
  -d '{"name": "DoesNotExist_XYZ"}' \
  "$BASE_URL/scenario/run")
assert_status "POST /scenario/run (unknown name → 404)" 404 "$response"

# missing name field → 400
response=$(curl -s -o /tmp/smoke_body.json -w "%{http_code}" -X POST \
  -H "Content-Type: application/json" \
  -d '{}' \
  "$BASE_URL/scenario/run")
assert_status "POST /scenario/run (missing name → 400)" 400 "$response"
echo

echo "=== GET /scenario/status ==="
response=$(curl -s -o /tmp/smoke_body.json -w "%{http_code}" "$BASE_URL/scenario/status")
assert_status "GET /scenario/status" 200 "$response"
body=$(cat /tmp/smoke_body.json)
for field in active scenario_name category actions_completed actions_total elapsed_sim_s; do
  assert_json_field "GET /scenario/status" "$body" "$field"
done
echo

echo "=== POST /scenario/stop ==="
response=$(curl -s -o /tmp/smoke_body.json -w "%{http_code}" -X POST "$BASE_URL/scenario/stop")
assert_status "POST /scenario/stop" 200 "$response"
assert_json_field "POST /scenario/stop" "$(cat /tmp/smoke_body.json)" "status"
echo

echo "=== CORS headers ==="
cors_header=$(curl -s -I "$BASE_URL/state" | grep -i "access-control-allow-origin")
if [[ -n "$cors_header" ]]; then
  pass "CORS Access-Control-Allow-Origin header present"
else
  fail "CORS header missing on GET /state"
fi
echo

# ── results ───────────────────────────────────────────────────────────────────

echo "==============================="
echo "Results: $PASS passed, $FAIL failed"
echo "==============================="
[[ $FAIL -eq 0 ]]
