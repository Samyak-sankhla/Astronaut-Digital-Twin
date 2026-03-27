#include "BioGearsEngineManager.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

#include <biogears/cdm/patient/actions/SEAcuteStress.h>
#include <biogears/cdm/patient/actions/SEConsumeNutrients.h>
#include <biogears/cdm/patient/actions/SEExercise.h>
#include <biogears/cdm/patient/actions/SESubstanceInfusion.h>
#include <biogears/cdm/properties/SEScalar0To1.h>
#include <biogears/cdm/properties/SEScalarFrequency.h>
#include <biogears/cdm/properties/SEScalarMass.h>
#include <biogears/cdm/properties/SEScalarMassPerVolume.h>
#include <biogears/cdm/properties/SEScalarPower.h>
#include <biogears/cdm/properties/SEScalarPressure.h>
#include <biogears/cdm/properties/SEScalarTime.h>
#include <biogears/cdm/properties/SEScalarVolume.h>
#include <biogears/cdm/properties/SEScalarVolumePerTime.h>
#include <biogears/cdm/substance/SESubstanceManager.h>
#include <biogears/cdm/system/environment/actions/SEEnvironmentChange.h>
#include <biogears/cdm/system/physiology/SECardiovascularSystem.h>
#include <biogears/cdm/system/physiology/SERespiratorySystem.h>
#include <biogears/engine/BioGearsPhysiologyEngine.h>

#include "PhysiologyState.h"

BioGearsEngineManager::BioGearsEngineManager(std::string runtime_dir)
  : runtime_dir_(std::move(runtime_dir))
{
}

bool BioGearsEngineManager::Initialize()
{
  std::lock_guard<std::mutex> lock(engine_mutex_);

  if (initialized_) {
    return true;
  }

  setenv("BIOGEARS_DATA_ROOT", runtime_dir_.c_str(), 1);

  engine_ = biogears::CreateBioGearsEngine(runtime_dir_, "biogears_digital_twin.log");
  if (!engine_) {
    SetLastError("Failed to create BioGears engine instance");
    return false;
  }

  const std::string initial_state = runtime_dir_ + "/states/StandardMale@0s.xml";
  if (!engine_->LoadState(initial_state)) {
    SetLastError("Failed to load initial state: " + initial_state);
    engine_.reset();
    return false;
  }

  initialized_ = true;
  return true;
}

bool BioGearsEngineManager::AdvanceAndExtract(double time_step_s, PhysiologyState& state)
{
  std::lock_guard<std::mutex> lock(engine_mutex_);
  if (!initialized_ || !engine_) {
    SetLastError("Engine not initialized");
    return false;
  }

  if (!engine_->AdvanceModelTime(time_step_s, biogears::TimeUnit::s)) {
    SetLastError("AdvanceModelTime failed");
    return false;
  }

  return ExtractVitals(state);
}

bool BioGearsEngineManager::ApplyExercise(double intensity_0_to_1)
{
  std::lock_guard<std::mutex> lock(engine_mutex_);
  if (!initialized_ || !engine_) {
    SetLastError("Engine not initialized");
    return false;
  }

  biogears::SEExercise exercise;
  biogears::SEExercise::SEGeneric generic_exercise;
  const double clamped_intensity = std::clamp(intensity_0_to_1, 0.0, 1.0);
  generic_exercise.Intensity.SetValue(clamped_intensity);
  generic_exercise.DesiredWorkRate.SetValue(clamped_intensity * 200.0, biogears::PowerUnit::W);
  exercise.SetGenericExercise(generic_exercise);

  if (!engine_->ProcessAction(exercise)) {
    SetLastError("Failed to process exercise action");
    return false;
  }

  return true;
}

bool BioGearsEngineManager::StopExercise()
{
  return ApplyExercise(0.0);
}

bool BioGearsEngineManager::ApplyHydration(double water_liters)
{
  std::lock_guard<std::mutex> lock(engine_mutex_);
  if (!initialized_ || !engine_) {
    SetLastError("Engine not initialized");
    return false;
  }

  (void)water_liters;
  biogears::SEConsumeNutrients hydration;
  hydration.SetNutritionFile("nutrition/Water.xml");

  if (!engine_->ProcessAction(hydration)) {
    SetLastError("Failed to process hydration (water nutrition) action");
    return false;
  }

  return true;
}

bool BioGearsEngineManager::ApplyDrugInfusion(const std::string& substance_name, double concentration_mg_per_ml, double rate_ml_per_min)
{
  std::lock_guard<std::mutex> lock(engine_mutex_);
  if (!initialized_ || !engine_) {
    SetLastError("Engine not initialized");
    return false;
  }

  biogears::SESubstance* substance = engine_->GetSubstanceManager().GetSubstance(substance_name);
  if (substance == nullptr) {
    SetLastError("Unknown substance: " + substance_name);
    return false;
  }

  biogears::SESubstanceInfusion infusion(*substance);
  infusion.GetConcentration().SetValue(std::max(0.0, concentration_mg_per_ml), biogears::MassPerVolumeUnit::mg_Per_mL);
  infusion.GetRate().SetValue(std::max(0.0, rate_ml_per_min), biogears::VolumePerTimeUnit::mL_Per_min);

  if (!engine_->ProcessAction(infusion)) {
    SetLastError("Failed to process drug infusion action");
    return false;
  }

  return true;
}

bool BioGearsEngineManager::ApplyStress(double severity_0_to_1)
{
  std::lock_guard<std::mutex> lock(engine_mutex_);
  if (!initialized_ || !engine_) {
    SetLastError("Engine not initialized");
    return false;
  }

  biogears::SEAcuteStress stress;
  stress.GetSeverity().SetValue(std::clamp(severity_0_to_1, 0.0, 1.0));

  if (!engine_->ProcessAction(stress)) {
    SetLastError("Failed to process acute stress action");
    return false;
  }

  return true;
}

bool BioGearsEngineManager::SetMicrogravity(bool enabled)
{
  std::lock_guard<std::mutex> lock(engine_mutex_);
  if (!initialized_ || !engine_) {
    SetLastError("Engine not initialized");
    return false;
  }

  biogears::SEEnvironmentChange env_change(engine_->GetSubstanceManager());
  env_change.SetConditionsFile(enabled ? "environments/Hypobaric4000m.xml" : "environments/StandardEnvironment.xml");

  if (!engine_->ProcessAction(env_change)) {
    SetLastError("Failed to process environment change for microgravity transition");
    return false;
  }

  biogears::SEAcuteStress stress;
  stress.GetSeverity().SetValue(enabled ? 0.2 : 0.0);
  if (!engine_->ProcessAction(stress)) {
    SetLastError("Failed to process stress modulation for microgravity transition");
    return false;
  }

  return true;
}

bool BioGearsEngineManager::IsInitialized() const
{
  std::lock_guard<std::mutex> lock(engine_mutex_);
  return initialized_;
}

std::string BioGearsEngineManager::GetLastError() const
{
  std::lock_guard<std::mutex> lock(error_mutex_);
  return last_error_;
}

bool BioGearsEngineManager::ExtractVitals(PhysiologyState& state)
{
  if (!engine_) {
    return false;
  }

  const biogears::SECardiovascularSystem* cardiovascular = engine_->GetCardiovascularSystem();
  const biogears::SERespiratorySystem* respiratory = engine_->GetRespiratorySystem();
  if (cardiovascular == nullptr || respiratory == nullptr) {
    SetLastError("Physiology systems unavailable");
    return false;
  }

  VitalsSnapshot snapshot;
  snapshot.simulation_time_s = engine_->GetSimulationTime(biogears::TimeUnit::s);
  snapshot.heart_rate_bpm = cardiovascular->GetHeartRate(biogears::FrequencyUnit::Per_min);
  snapshot.systolic_pressure_mmhg = cardiovascular->GetSystolicArterialPressure(biogears::PressureUnit::mmHg);
  snapshot.diastolic_pressure_mmhg = cardiovascular->GetDiastolicArterialPressure(biogears::PressureUnit::mmHg);
  snapshot.mean_arterial_pressure_mmhg = cardiovascular->GetMeanArterialPressure(biogears::PressureUnit::mmHg);
  snapshot.cardiac_output_l_min = cardiovascular->GetCardiacOutput(biogears::VolumePerTimeUnit::L_Per_min);
  snapshot.blood_volume_l = cardiovascular->GetBloodVolume(biogears::VolumeUnit::L);
  snapshot.respiration_rate_bpm = respiratory->GetRespirationRate(biogears::FrequencyUnit::Per_min);

  state.Update(snapshot);
  return true;
}

void BioGearsEngineManager::SetLastError(const std::string& message)
{
  std::lock_guard<std::mutex> lock(error_mutex_);
  last_error_ = message;
}
