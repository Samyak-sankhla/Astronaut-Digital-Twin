#pragma once

#include <mutex>

#include <nlohmann/json.hpp>

struct VitalsSnapshot {
  double simulation_time_s = 0.0;
  double heart_rate_bpm = 0.0;
  double systolic_pressure_mmhg = 0.0;
  double diastolic_pressure_mmhg = 0.0;
  double mean_arterial_pressure_mmhg = 0.0;
  double cardiac_output_l_min = 0.0;
  double blood_volume_l = 0.0;
  double respiration_rate_bpm = 0.0;
};

class PhysiologyState {
public:
  void Update(const VitalsSnapshot& snapshot);
  VitalsSnapshot GetSnapshot() const;
  nlohmann::json ToJson() const;

private:
  mutable std::mutex mutex_;
  VitalsSnapshot latest_;
};
