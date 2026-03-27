# BioGears Digital Twin TODO

## Current Scope

- [x] Scaffold CMake project and source tree
- [x] Add BioGears engine manager for continuous simulation
- [x] Add threaded simulation loop (`0.02s` timestep)
- [x] Add event processor for runtime actions
- [x] Add REST API endpoints (`/state`, `/event/exercise`, `/event/hydration`, `/event/drug`, `/event/microgravity`)
- [ ] Validate compile and runtime on this machine
- [ ] Add integration smoke tests for API endpoints
- [ ] Add frontend-facing API contract documentation

## Notes

- BioGears installation is consumed from:
  - Headers: `/opt/biogears/usr/include`
  - Libraries: `/opt/biogears/usr/lib`
  - Runtime: `/opt/biogears/runtime`
- Engine state file: `/opt/biogears/runtime/states/StandardMale@0s.xml`
