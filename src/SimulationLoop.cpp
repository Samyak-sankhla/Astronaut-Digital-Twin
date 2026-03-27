#include "SimulationLoop.h"

#include <chrono>
#include <thread>

#include "BioGearsEngineManager.h"
#include "PhysiologyState.h"

SimulationLoop::SimulationLoop(BioGearsEngineManager& engine_manager, PhysiologyState& physiology_state, double time_step_s)
  : engine_manager_(engine_manager)
  , physiology_state_(physiology_state)
  , time_step_s_(time_step_s)
{
}

SimulationLoop::~SimulationLoop()
{
  Stop();
}

bool SimulationLoop::Start()
{
  if (running_.exchange(true)) {
    return false;
  }

  worker_ = std::thread(&SimulationLoop::Run, this);
  return true;
}

void SimulationLoop::Stop()
{
  if (!running_.exchange(false)) {
    return;
  }

  if (worker_.joinable()) {
    worker_.join();
  }
}

bool SimulationLoop::IsRunning() const
{
  return running_.load();
}

void SimulationLoop::Run()
{
  using clock = std::chrono::steady_clock;
  const auto tick = std::chrono::duration<double>(time_step_s_);
  auto next_tick = clock::now() + tick;

  while (running_.load()) {
    engine_manager_.AdvanceAndExtract(time_step_s_, physiology_state_);
    std::this_thread::sleep_until(next_tick);
    next_tick += std::chrono::duration_cast<clock::duration>(tick);
  }
}
