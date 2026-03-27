#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <thread>

#include "ApiServer.h"
#include "BioGearsEngineManager.h"
#include "EventProcessor.h"
#include "PhysiologyState.h"
#include "SimulationLoop.h"

namespace {
std::atomic<bool> g_running{true};

void HandleSignal(int)
{
  g_running.store(false);
}
} // namespace

int main()
{
  std::signal(SIGINT, HandleSignal);
  std::signal(SIGTERM, HandleSignal);

  BioGearsEngineManager engine_manager("/opt/biogears/runtime");
  if (!engine_manager.Initialize()) {
    std::cerr << "Failed to initialize BioGears engine: " << engine_manager.GetLastError() << '\n';
    return 1;
  }

  PhysiologyState physiology_state;
  if (!engine_manager.AdvanceAndExtract(0.0, physiology_state)) {
    std::cerr << "Failed to extract initial physiology state: " << engine_manager.GetLastError() << '\n';
    return 1;
  }

  EventProcessor event_processor(engine_manager);
  SimulationLoop simulation_loop(engine_manager, physiology_state, 0.02);
  if (!simulation_loop.Start()) {
    std::cerr << "Failed to start simulation loop" << '\n';
    return 1;
  }

  ApiServer api_server(event_processor, physiology_state, 8080);
  if (!api_server.Start()) {
    std::cerr << "Failed to start API server" << '\n';
    simulation_loop.Stop();
    return 1;
  }

  std::cout << "BioGears digital twin backend running on http://0.0.0.0:8080" << '\n';
  std::cout << "Press Ctrl+C to stop." << '\n';

  while (g_running.load()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
  }

  api_server.Stop();
  simulation_loop.Stop();

  return 0;
}
