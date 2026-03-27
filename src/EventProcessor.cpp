#include "EventProcessor.h"

#include <algorithm>
#include <chrono>

#include "BioGearsEngineManager.h"

EventProcessor::EventProcessor(BioGearsEngineManager& engine_manager)
  : engine_manager_(engine_manager)
{
}

EventProcessor::~EventProcessor()
{
  accept_timers_.store(false);

  std::lock_guard<std::mutex> lock(timer_mutex_);
  for (std::thread& t : timer_threads_) {
    if (t.joinable()) {
      t.join();
    }
  }
}

bool EventProcessor::TriggerExercise(double intensity_0_to_1, int duration_seconds)
{
  const double intensity = std::clamp(intensity_0_to_1, 0.0, 1.0);
  if (!engine_manager_.ApplyExercise(intensity)) {
    return false;
  }

  if (duration_seconds > 0) {
    std::lock_guard<std::mutex> lock(timer_mutex_);
    timer_threads_.emplace_back([this, duration_seconds]() {
      std::this_thread::sleep_for(std::chrono::seconds(duration_seconds));
      if (accept_timers_.load()) {
        engine_manager_.StopExercise();
      }
    });
  }

  return true;
}

bool EventProcessor::TriggerHydration(double water_liters)
{
  return engine_manager_.ApplyHydration(water_liters);
}

bool EventProcessor::TriggerDrugInfusion(const std::string& substance_name, double concentration_mg_per_ml, double rate_ml_per_min)
{
  return engine_manager_.ApplyDrugInfusion(substance_name, concentration_mg_per_ml, rate_ml_per_min);
}

bool EventProcessor::TriggerStress(double severity_0_to_1)
{
  return engine_manager_.ApplyStress(severity_0_to_1);
}

bool EventProcessor::TriggerMicrogravity(bool enabled)
{
  return engine_manager_.SetMicrogravity(enabled);
}
