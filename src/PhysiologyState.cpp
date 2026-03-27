#include "PhysiologyState.h"

void PhysiologyState::Update(const VitalsSnapshot& snapshot)
{
  std::lock_guard<std::mutex> lock(mutex_);
  latest_ = snapshot;
}

VitalsSnapshot PhysiologyState::GetSnapshot() const
{
  std::lock_guard<std::mutex> lock(mutex_);
  return latest_;
}

nlohmann::json PhysiologyState::ToJson() const
{
  const VitalsSnapshot s = GetSnapshot();
  return {
    {"simulation_time_s", s.simulation_time_s},
    {"heart_rate", s.heart_rate_bpm},
    {"systolic_pressure", s.systolic_pressure_mmhg},
    {"diastolic_pressure", s.diastolic_pressure_mmhg},
    {"map", s.mean_arterial_pressure_mmhg},
    {"cardiac_output", s.cardiac_output_l_min},
    {"blood_volume", s.blood_volume_l},
    {"respiration_rate", s.respiration_rate_bpm}
  };
}
