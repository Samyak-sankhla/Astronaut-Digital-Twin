#pragma once

#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

class BioGearsEngineManager;

class EventProcessor {
public:
  explicit EventProcessor(BioGearsEngineManager& engine_manager);
  ~EventProcessor();

  bool TriggerExercise(double intensity_0_to_1, int duration_seconds);
  bool TriggerHydration(double water_liters);
  bool TriggerDrugInfusion(const std::string& substance_name, double concentration_mg_per_ml, double rate_ml_per_min);
  bool TriggerStress(double severity_0_to_1);
  bool TriggerMicrogravity(bool enabled);

private:
  BioGearsEngineManager& engine_manager_;
  std::atomic<bool> accept_timers_{true};
  std::mutex timer_mutex_;
  std::vector<std::thread> timer_threads_;
};
