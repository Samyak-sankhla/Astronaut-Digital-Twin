
# Astronaut Digital Twin

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

A real-time astronaut physiology digital twin backend built in C++ on top of BioGears. The system simulates cardiovascular and related physiological behavior under mission-relevant conditions such as microgravity transitions, exercise load, hydration/nutrition, medication infusion, stress, and fluid therapy.

GitHub repository: https://github.com/Samyak-sankhla/Astronaut-Digital-Twin

## 1. Project Goal

Long-duration spaceflight introduces measurable physiological changes, including reduced plasma volume, autonomic adaptation, re-entry intolerance, and orthostatic instability. This project provides an interactive simulation service that supports:

- Continuous simulation state (no reset between events)
- Real-time event injection during runtime
- Scenario playback from BioGears XML definitions
- API-first integration with dashboards or mission UI tools
- Time-accelerated simulation for rapid what-if analysis

## 2. What This Repository Contains

This repository is centered on the backend simulation service and a lightweight frontend stub.

- C++ simulation server and API in src
- Build system and dependency setup in CMakeLists.txt
- API and scientific audit docs in docs
- Frontend static dashboard prototype in frontend
- Startup helper scripts in run_backend.sh, run_frontend.sh, and run_paths.sh
- Smoke tests in tests/smoke_test.sh

## 3. Architecture Overview

### 3.1 Runtime Layers

1. Client layer
     Sends event commands and reads live physiology state.
2. API layer
     HTTP server with JSON endpoints and CORS handling.
3. Simulation control layer
     Event processing, scenario playback, and simulation timing.
4. Physiology engine layer
     BioGears physiology and action execution.
5. State and history layer
     Thread-safe current snapshot and rolling historical timeline.

### 3.2 Core Components

- main.cpp
    Bootstraps runtime paths, initializes BioGears, starts simulation loop, starts API server, and handles graceful shutdown.
- BioGearsEngineManager
    Owns engine integration and encapsulates physiology action calls (exercise, hydration, nutrition, infusion, stress, microgravity, etc.).
- SimulationLoop
    Advances physiology at fixed time step (default 0.02 s) and updates the shared state.
- PhysiologyState
    Thread-safe latest snapshot plus capped history buffer (500 snapshots).
- EventProcessor
    Converts incoming high-level API events into engine actions; supports timed action scheduling.
- ScenarioPlayer
    Scans scenario XML files, runs one scenario at a time in worker thread, tracks progress/status.
- ApiServer
    Exposes REST endpoints for state queries, event injection, scenario orchestration, and substance/compound discovery.

### 3.3 Data Flow

1. API request arrives
2. Payload is validated and mapped to domain event
3. EventProcessor/ScenarioPlayer applies action to BioGearsEngineManager
4. SimulationLoop advances engine clock
5. PhysiologyState is refreshed
6. Clients read state via polling or SSE stream

## 4. Physiological Scope

Implemented event families:

- Exercise
    Generic exercise and scenario-based exercise modes
- Hydration and Nutrition
    Fluid intake and nutrient consumption actions
- Pharmacology and Fluids
    Substance infusion, compound infusion (fluid therapy), bolus, oral dose support via scenarios
- Stress and Injury Proxies
    Acute stress and hemorrhage proxies where required
- Environment and Gravity Transition Approximation
    Environment changes and microgravity workflow

Scientific note:
Direct gravity-level control is not exposed in this project path. Partial-gravity transitions are represented through environment and autonomic loading proxies, which is suitable for comparative trend studies but not a fully mechanistic gravity model.

## 5. API Surface

Primary endpoints exposed by the backend:

- GET /state
- GET /state/history
- GET /state/stream
- POST /event/exercise
- POST /event/hydration
- POST /event/nutrition
- POST /event/drug
- POST /event/fluid_therapy
- POST /event/microgravity
- POST /event/stress
- GET /substances
- GET /compounds
- GET /scenarios
- POST /scenario/run
- GET /scenario/status
- POST /scenario/stop

Detailed request/response contract is documented in docs/api.md.

## 6. Scenario System

Scenario behavior is driven by XML files in the BioGears runtime Scenarios tree.

- Discovery
    ScenarioPlayer scans category directories and builds metadata list.
- Playback
    Actions are executed in sequence with support for timed waits and progress tracking.
- Status model
    Exposes active scenario, category, completed actions, total actions, elapsed simulation time, and error field.
- Concurrency model
    One active scenario at a time; duplicate starts are rejected.

The repository also includes scenario validation and scientific notes in docs/scenario_scientific_audit.md.

## 7. Build and Runtime Requirements

### 7.1 Toolchain

- Linux environment
- CMake 3.16 or newer
- C++20-capable compiler
- pthread-capable system toolchain

### 7.2 Native/Third-party Dependencies

- BioGears shared libraries and headers
- tinyxml2
- cpp-httplib (fetched by CMake)
- nlohmann/json (fetched by CMake)

### 7.3 Runtime Inputs

- BioGears runtime folder (environments, patients, substances, scenarios, etc.)
- Valid library path for BioGears shared objects

## 8. Configuration Model

Runtime control is environment-variable driven.

- BIOGEARS_ROOT
    CMake-time root for include/lib discovery
- BIOGEARS_RUNTIME
    Runtime asset directory
- SIM_SPEED
    Simulation speed multiplier (default 1.0)

Helper scripts provide portable launcher defaults:

- run_paths.sh
- run_backend.sh
- run_frontend.sh

run_paths.sh auto-detects project-root-local resources first and falls back to HOME-based BioGears defaults. You can still override any path with environment variables.

## 9. How to Run

1. Ensure BioGears libraries and runtime assets are installed locally.
2. Configure BIOGEARS_ROOT (or pass it to CMake).
3. Configure and build with CMake in build directory.
4. Start backend using run_backend.sh or the built binary.
5. (Optional) Start frontend static server using run_frontend.sh.
6. Validate with tests/smoke_test.sh.

The backend listens on port 8080 by default.

## 10. Testing and Verification

Smoke tests cover:

- Health/state endpoints
- Event endpoints with success and invalid JSON cases
- Scenario discovery, run, status, stop, and conflict behavior
- History and CORS checks

Scientific and XML consistency checks are summarized in docs/scenario_scientific_audit.md.

## 11. Repository Layout (Recommended Source of Truth)

- src: backend source code
- docs: API contract and scientific audit
- frontend: static prototype UI
- tests: smoke validation
- CMakeLists.txt: build configuration
- README.md and LICENSE: project documentation and licensing

## 12. GitHub Upload Guidance (Only Necessary Files)

Push these:

- biogears
- src
- docs
- frontend
- tests
- CMakeLists.txt
- README.md
- LICENSE
- run_backend.sh
- run_frontend.sh
- run_paths.sh (if you want to keep your local launch helper documented)
- digital_twin_project_overview.md (optional, as high-level concept note)

Do not push these:

- build artifacts
- local Python virtual environments
- local editor/tooling folders
- logs/core dumps

If the biogears directory becomes very large in future revisions, consider enabling Git LFS for large binary artifacts.

This repository is now set up with ignore rules so accidental local artifacts are not committed.

## 13. Known Limitations

- Gravity transitions are approximated rather than mechanistically modeled.
- External BioGears installation path conventions can differ by machine and may require environment-variable overrides.
- Frontend is a prototype UI, not a production visualization stack.

## 14. Roadmap Suggestions

- Add startup environment validation checks with clear remediation messages.
- Add CI for build and smoke tests.
- Add versioned API documentation and changelog.
- Add containerized development/runtime option.
- Expand validation suite for physiology acceptance thresholds.

## 15. License

MIT License. See LICENSE.
