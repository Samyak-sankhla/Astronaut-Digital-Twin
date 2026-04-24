
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

## 12. Backend Source Documentation (src)

This section documents the implementation-level structure of the backend service in src.

### 12.1 Startup and Process Lifecycle

- main.cpp
    Initializes signal handlers for graceful shutdown (SIGINT, SIGTERM), resolves runtime directories, and starts all major runtime services.
- Initialization order
    1) BioGears engine initialization, 2) initial state extraction, 3) simulation loop start, 4) API server start.
- Shutdown order
    API server stop, scenario worker stop, simulation loop stop.

### 12.2 API Layer

- ApiServer.h / ApiServer.cpp
    Owns the HTTP server, route registration, and endpoint response handling.
- CORS behavior
    Supports cross-origin frontend usage with permissive origin/method/header policy.
- State delivery patterns
    Exposes both polling-friendly JSON endpoints and a server-sent events stream endpoint.
- Event handling behavior
    Parses JSON payloads, applies default values where defined, performs request validation, and returns structured JSON status/error responses.
- Activity logging
    Writes API activity lines with timestamp and status marker (OK/FAIL) to the runtime log file.

### 12.3 Event Translation Layer

- EventProcessor.h / EventProcessor.cpp
    Converts API-level event intents into engine-level actions.
- Timed actions
    Supports auto-stop logic for duration-scoped exercise and infusions using timer threads.
- Lifetime safety
    Destructor prevents timer leaks by stopping acceptance of new timers and joining all timer threads.

### 12.4 Engine Integration Layer

- BioGearsEngineManager.h / BioGearsEngineManager.cpp
    Central adapter for all BioGears interactions and physiology action APIs.
- Engine bootstrap
    Sets BioGears data root, creates engine instance, loads the standard initial patient state, and tracks initialization health.
- Action surface
    Exercise (generic/cycling/strength), hydration, nutrition, drug infusion, compound infusion, stress, hemorrhage, bolus, oral dose, pain stimulus, environment changes, and microgravity mode transitions.
- Drug interaction control logic
    Maintains active infusion map and recomputes effective delivery rates/concentrations with interaction-aware modifiers across drug classes.
- Microgravity controller
    Maintains internal state machine for microgravity progression and re-entry behavior, including stress and volume-loss proxy control updates and validation checkpoints.
- Concurrency model
    Uses a mutex-protected engine access pattern so simulation stepping and external events remain thread-safe.

### 12.5 Simulation Clock and State Store

- SimulationLoop.h / SimulationLoop.cpp
    Worker thread that advances model time at a fixed cadence and applies simulation speed scaling.
- Timing strategy
    Uses steady-clock scheduling with sleep-until behavior to keep update cadence stable.
- PhysiologyState.h / PhysiologyState.cpp
    Thread-safe latest snapshot plus rolling history buffer for time-series inspection.
- JSON contract mapping
    Converts internal vitals fields to API-facing names used by frontend and tests.

### 12.6 Scenario Execution System

- ScenarioPlayer.h / ScenarioPlayer.cpp
    Scenario discovery, metadata extraction, asynchronous scenario playback, and status tracking.
- Discovery model
    Scans scenario categories from runtime folders, parses descriptions, and sorts with astronaut category precedence.
- Playback model
    Processes action sequences from XML and dispatches action types to engine calls.
- Action coverage in dispatcher
    AdvanceTime, EnvironmentChange, AcuteStress, Exercise variants, ConsumeNutrients, Hemorrhage, SubstanceInfusion, SubstanceCompoundInfusion, SubstanceBolus, PainStimulus, and SubstanceOralDose.
- Unit normalization
    Includes unit conversions required by scenario content (for example ug/mL to mg/mL and mL/hr to mL/min where applicable).

## 13. Frontend Documentation (frontend)

The frontend is a single-page static monitoring and control console designed to operate directly against the backend API.

### 13.1 Technology and Runtime Model

- frontend/index.html
    Contains layout, styling, and JavaScript control logic in one static artifact.
- Serving model
    Launched as static content through run_frontend.sh (Python HTTP server).
- API target
    Default backend target is localhost:8080.

### 13.2 UI Composition

- Header status
    Shows connection health, simulation context, and active action progress indicators.
- Sidebar control panels
    Event controls for exercise, hydration, nutrition, drug infusion, fluid therapy, stress, microgravity, and scenario operations.
- Main telemetry area
    Real-time vital cards and trend charts for key physiological measures.
- Clinical assistive panels
    Recommendation and event-analysis surfaces that summarize directional physiological changes.

### 13.3 State Update and Polling Strategy

- Continuous polling
    Polls backend state at fixed interval for dashboard refresh.
- Scenario polling
    Polls scenario status independently to render progress, active state, and completion behavior.
- Local buffers
    Maintains short and medium windows of recent vitals to compute deltas and trends.

### 13.4 Visualization and Interpretation

- Charting
    Uses Chart.js line charts for heart rate, MAP, and respiration trends.
- Threshold system
    Applies normal/warn/critical classing to vital cards.
- Delta indicators
    Compares current readings with recent history to show directional arrows and magnitude hints.
- Trend estimation
    Uses rolling-window slope approximation for recommendation logic.

### 13.5 Action Workflow and UX Safety

- Locked action execution
    During active interventions, relevant controls are temporarily locked to reduce conflicting commands.
- Action progress
    Displays intervention progress bar and timing estimate for user feedback.
- Post-action summary
    Captures before/after snapshots and renders compact delta summaries.
- Scenario control
    Supports scenario list fetch, run, progress tracking, and explicit stop flow.

### 13.6 User Feedback and Diagnostics

- Toast notifications
    Displays API success/failure outcomes.
- Activity log
    Shows timestamped operation history in the UI.
- Connectivity indicator
    Reflects backend availability status for quick operator awareness.

## 14. GitHub Upload Guidance (Only Necessary Files)

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

## 15. Known Limitations

- Gravity transitions are approximated rather than mechanistically modeled.
- External BioGears installation path conventions can differ by machine and may require environment-variable overrides.
- Frontend is a prototype UI, not a production visualization stack.

## 16. Roadmap Suggestions

- Add startup environment validation checks with clear remediation messages.
- Add CI for build and smoke tests.
- Add versioned API documentation and changelog.
- Add containerized development/runtime option.
- Expand validation suite for physiology acceptance thresholds.

## 17. License

MIT License. See LICENSE.
