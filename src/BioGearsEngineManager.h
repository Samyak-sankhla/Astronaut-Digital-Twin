#pragma once

#include <memory>
#include <mutex>
#include <string>

#include <biogears/cdm/engine/PhysiologyEngine.h>

class PhysiologyState;

class BioGearsEngineManager {
public:
  explicit BioGearsEngineManager(std::string runtime_dir = "/opt/biogears/runtime");

  bool Initialize();
  bool AdvanceAndExtract(double time_step_s, PhysiologyState& state);

  bool ApplyExercise(double intensity_0_to_1);
  bool StopExercise();
  bool ApplyHydration(double water_liters);
  bool ApplyDrugInfusion(const std::string& substance_name, double concentration_mg_per_ml, double rate_ml_per_min);
  bool ApplyStress(double severity_0_to_1);
  bool SetMicrogravity(bool enabled);

  bool IsInitialized() const;
  std::string GetLastError() const;

private:
  bool ExtractVitals(PhysiologyState& state);
  void SetLastError(const std::string& message);

  std::string runtime_dir_;
  mutable std::mutex engine_mutex_;
  std::unique_ptr<biogears::PhysiologyEngine> engine_;

  mutable std::mutex error_mutex_;
  std::string last_error_;
  bool initialized_ = false;
};
