#pragma once

#include <atomic>
#include <thread>

class BioGearsEngineManager;
class PhysiologyState;

class SimulationLoop {
public:
  SimulationLoop(BioGearsEngineManager& engine_manager, PhysiologyState& physiology_state, double time_step_s = 0.02);
  ~SimulationLoop();

  bool Start();
  void Stop();
  bool IsRunning() const;

private:
  void Run();

  BioGearsEngineManager& engine_manager_;
  PhysiologyState& physiology_state_;
  double time_step_s_;
  std::atomic<bool> running_{false};
  std::thread worker_;
};
