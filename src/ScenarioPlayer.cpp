#include "ScenarioPlayer.h"

#include <chrono>
#include <filesystem>
#include <sstream>
#include <thread>

#include <tinyxml2.h>

#include "BioGearsEngineManager.h"

namespace fs = std::filesystem;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

static std::string ExtractDescription(const std::string& file_path)
{
  tinyxml2::XMLDocument doc;
  if (doc.LoadFile(file_path.c_str()) != tinyxml2::XML_SUCCESS) {
    return "";
  }
  const tinyxml2::XMLElement* scenario = doc.FirstChildElement("Scenario");
  if (!scenario) { return ""; }
  const tinyxml2::XMLElement* desc = scenario->FirstChildElement("Description");
  if (!desc || !desc->GetText()) { return ""; }
  return desc->GetText();
}

static int CountActions(const tinyxml2::XMLElement* actions_elem)
{
  int n = 0;
  for (const tinyxml2::XMLElement* a = actions_elem->FirstChildElement("Action");
       a; a = a->NextSiblingElement("Action")) {
    ++n;
  }
  return n;
}

// ─────────────────────────────────────────────────────────────────────────────
// ScenarioPlayer
// ─────────────────────────────────────────────────────────────────────────────

ScenarioPlayer::ScenarioPlayer(BioGearsEngineManager& engine, double speed_multiplier)
  : engine_(engine)
  , speed_multiplier_(speed_multiplier > 0.0 ? speed_multiplier : 1.0)
{
}

ScenarioPlayer::~ScenarioPlayer()
{
  Stop();
}

std::vector<ScenarioPlayer::ScenarioInfo>
ScenarioPlayer::ListScenarios(const std::string& scenarios_root) const
{
  std::vector<ScenarioInfo> result;

  std::error_code ec;
  if (!fs::is_directory(scenarios_root, ec)) {
    return result;
  }

  for (const auto& dir_entry : fs::directory_iterator(scenarios_root, ec)) {
    if (!dir_entry.is_directory()) { continue; }
    const std::string category = dir_entry.path().filename().string();

    for (const auto& file_entry : fs::directory_iterator(dir_entry.path(), ec)) {
      if (!file_entry.is_regular_file()) { continue; }
      if (file_entry.path().extension() != ".xml") { continue; }

      const std::string stem = file_entry.path().stem().string();
      const std::string full_path = file_entry.path().string();

      ScenarioInfo info;
      info.name        = stem;
      info.category    = category;
      info.file_path   = full_path;
      info.description = ExtractDescription(full_path);
      result.push_back(std::move(info));
    }
  }

  // Sort: Astronaut first, then alphabetically by category + name.
  std::sort(result.begin(), result.end(), [](const ScenarioInfo& a, const ScenarioInfo& b) {
    if (a.category != b.category) {
      if (a.category == "Astronaut") { return true; }
      if (b.category == "Astronaut") { return false; }
      return a.category < b.category;
    }
    return a.name < b.name;
  });

  return result;
}

bool ScenarioPlayer::PlayByName(const std::string& name, const std::string& scenarios_root)
{
  const auto scenarios = ListScenarios(scenarios_root);
  for (const auto& info : scenarios) {
    if (info.name == name) {
      return Play(info);
    }
  }
  return false;
}

bool ScenarioPlayer::Play(const ScenarioInfo& info)
{
  if (running_.exchange(true)) {
    return false; // already active
  }

  {
    std::lock_guard<std::mutex> lock(status_mutex_);
    status_ = Status{};
    status_.active        = true;
    status_.scenario_name = info.name;
    status_.category      = info.category;
  }

  worker_ = std::thread(&ScenarioPlayer::Run, this, info);
  return true;
}

void ScenarioPlayer::Stop()
{
  if (!running_.exchange(false)) {
    return;
  }
  if (worker_.joinable()) {
    worker_.join();
  }
}

ScenarioPlayer::Status ScenarioPlayer::GetStatus() const
{
  std::lock_guard<std::mutex> lock(status_mutex_);
  return status_;
}

// ─────────────────────────────────────────────────────────────────────────────
// Action dispatch
// ─────────────────────────────────────────────────────────────────────────────

// Returns the sim-time advance in seconds, or -1 if the action failed.
// Returns 0 for non-time actions (applied immediately).
static double DispatchAction(const tinyxml2::XMLElement* action,
                             BioGearsEngineManager& engine,
                             double speed_multiplier,
                             std::atomic<bool>& running)
{
  const char* type_attr = action->Attribute("xsi:type");
  if (!type_attr) { return 0.0; }
  const std::string type(type_attr);

  // ── AdvanceTime ───────────────────────────────────────────────────────────
  if (type == "AdvanceTimeData") {
    const tinyxml2::XMLElement* time_elem = action->FirstChildElement("Time");
    if (!time_elem) { return 0.0; }

    const double value = time_elem->DoubleAttribute("value", 0.0);
    const char* unit_c = time_elem->Attribute("unit");
    const std::string unit = unit_c ? unit_c : "s";

    double sim_seconds = value;
    if (unit == "min")  { sim_seconds = value * 60.0; }
    else if (unit == "hr") { sim_seconds = value * 3600.0; }

    // Sleep for real-world equivalent time.
    const double real_seconds = sim_seconds / speed_multiplier;
    const auto deadline = std::chrono::steady_clock::now()
                        + std::chrono::duration<double>(real_seconds);

    // Check running_ every 200 ms so we can be interrupted.
    while (running.load() &&
           std::chrono::steady_clock::now() < deadline) {
      const auto remaining = deadline - std::chrono::steady_clock::now();
      const auto chunk = std::chrono::milliseconds(200);
      std::this_thread::sleep_for(remaining < chunk ? remaining : chunk);
    }
    return sim_seconds;
  }

  // ── EnvironmentChange ────────────────────────────────────────────────────
  if (type == "EnvironmentChangeData") {
    const tinyxml2::XMLElement* cf = action->FirstChildElement("ConditionsFile");
    if (cf && cf->GetText()) {
      if (!engine.SetEnvironmentFile(cf->GetText())) { return -1.0; }
    }
    return 0.0;
  }

  // ── AcuteStress ──────────────────────────────────────────────────────────
  if (type == "AcuteStressData") {
    const tinyxml2::XMLElement* sev = action->FirstChildElement("Severity");
    const double severity = sev ? sev->DoubleAttribute("value", 0.0) : 0.0;
    if (!engine.ApplyStress(severity)) { return -1.0; }
    return 0.0;
  }

  // ── Exercise ─────────────────────────────────────────────────────────────
  if (type == "ExerciseData") {
    if (const tinyxml2::XMLElement* generic = action->FirstChildElement("GenericExercise")) {
      const tinyxml2::XMLElement* intensity_elem = generic->FirstChildElement("Intensity");
      const double intensity = intensity_elem ? intensity_elem->DoubleAttribute("value", 0.0) : 0.0;
      if (!engine.ApplyExercise(intensity)) { return -1.0; }
    } else if (const tinyxml2::XMLElement* cycling = action->FirstChildElement("CyclingExercise")) {
      const tinyxml2::XMLElement* cadence_elem = cycling->FirstChildElement("Cadence");
      const tinyxml2::XMLElement* power_elem   = cycling->FirstChildElement("Power");
      const double cadence = cadence_elem ? cadence_elem->DoubleAttribute("value", 0.0) : 0.0;
      const double power   = power_elem   ? power_elem->DoubleAttribute("value",   0.0) : 0.0;
      if (!engine.ApplyCyclingExercise(cadence, power)) { return -1.0; }
    } else if (const tinyxml2::XMLElement* strength = action->FirstChildElement("StrengthExercise")) {
      const tinyxml2::XMLElement* weight_elem = strength->FirstChildElement("Weight");
      const tinyxml2::XMLElement* reps_elem   = strength->FirstChildElement("Repetitions");
      const double weight = weight_elem ? weight_elem->DoubleAttribute("value", 0.0) : 0.0;
      const double reps   = reps_elem   ? reps_elem->DoubleAttribute("value",   0.0) : 0.0;
      if (!engine.ApplyStrengthExercise(weight, reps)) { return -1.0; }
    }
    return 0.0;
  }

  // ── ConsumeNutrients ─────────────────────────────────────────────────────
  if (type == "ConsumeNutrientsData") {
    if (const tinyxml2::XMLElement* nutrition = action->FirstChildElement("Nutrition")) {
      auto read = [&](const char* name) -> double {
        const tinyxml2::XMLElement* e = nutrition->FirstChildElement(name);
        return e ? e->DoubleAttribute("value", 0.0) : 0.0;
      };
      const double carbs   = read("Carbohydrate");
      const double protein = read("Protein");
      const double fat     = read("Fat");
      const double water   = read("Water");
      if (!engine.ApplyNutrition(carbs, protein, fat, water)) { return -1.0; }
    }
    // NutritionFile variant: skip (would require loading/parsing another file)
    return 0.0;
  }

  // ── Hemorrhage ───────────────────────────────────────────────────────────
  if (type == "HemorrhageData") {
    const tinyxml2::XMLElement* comp_elem = action->FirstChildElement("Compartment");
    const tinyxml2::XMLElement* rate_elem = action->FirstChildElement("InitialRate");
    if (comp_elem && comp_elem->GetText() && rate_elem) {
      const std::string compartment = comp_elem->GetText();
      const double rate = rate_elem->DoubleAttribute("value", 0.0);
      if (!engine.ApplyHemorrhage(compartment, rate)) { return -1.0; }
    }
    return 0.0;
  }

  // ── SubstanceInfusion (drug infusion from scenarios) ─────────────────────
  if (type == "SubstanceInfusionData") {
    const tinyxml2::XMLElement* sub_elem  = action->FirstChildElement("Substance");
    const tinyxml2::XMLElement* conc_elem = action->FirstChildElement("Concentration");
    const tinyxml2::XMLElement* rate_elem = action->FirstChildElement("Rate");
    if (sub_elem && sub_elem->GetText()) {
      const std::string substance = sub_elem->GetText();
      double concentration = conc_elem ? conc_elem->DoubleAttribute("value", 0.0) : 0.0;
      double rate = rate_elem ? rate_elem->DoubleAttribute("value", 0.0) : 0.0;
      // Convert units: scenarios may use ug/mL but our API uses mg/mL
      const char* conc_unit = conc_elem ? conc_elem->Attribute("unit") : nullptr;
      if (conc_unit && std::string(conc_unit).find("ug") != std::string::npos) {
        concentration /= 1000.0;  // ug/mL -> mg/mL
      }
      // Convert rate units: scenarios may use mL/s
      const char* rate_unit = rate_elem ? rate_elem->Attribute("unit") : nullptr;
      if (rate_unit && std::string(rate_unit) == "mL/s") {
        rate *= 60.0;  // mL/s -> mL/min
      }
      if (!engine.ApplyDrugInfusion(substance, concentration, rate)) { return -1.0; }
    }
    return 0.0;
  }

  // ── SubstanceCompoundInfusion (IV fluids from scenarios) ─────────────────
  if (type == "SubstanceCompoundInfusionData") {
    const tinyxml2::XMLElement* compound_elem = action->FirstChildElement("SubstanceCompound");
    const tinyxml2::XMLElement* bag_elem      = action->FirstChildElement("BagVolume");
    const tinyxml2::XMLElement* rate_elem     = action->FirstChildElement("Rate");
    if (compound_elem && compound_elem->GetText()) {
      const std::string compound_name = compound_elem->GetText();
      double bag_volume_ml = bag_elem ? bag_elem->DoubleAttribute("value", 500.0) : 500.0;
      double rate_ml_per_min = rate_elem ? rate_elem->DoubleAttribute("value", 100.0) : 100.0;
      // Convert rate units: scenarios often use mL/hr
      const char* rate_unit = rate_elem ? rate_elem->Attribute("unit") : nullptr;
      if (rate_unit && std::string(rate_unit) == "mL/hr") {
        rate_ml_per_min /= 60.0;  // mL/hr -> mL/min
      }
      if (!engine.ApplyCompoundInfusion(compound_name, bag_volume_ml, rate_ml_per_min)) { return -1.0; }
    }
    return 0.0;
  }

  // ── SubstanceBolus (rapid IV push from scenarios) ────────────────────────
  if (type == "SubstanceBolusData") {
    const tinyxml2::XMLElement* sub_elem  = action->FirstChildElement("Substance");
    const tinyxml2::XMLElement* conc_elem = action->FirstChildElement("Concentration");
    const tinyxml2::XMLElement* dose_elem = action->FirstChildElement("Dose");
    if (sub_elem && sub_elem->GetText()) {
      const std::string substance = sub_elem->GetText();
      double concentration = conc_elem ? conc_elem->DoubleAttribute("value", 0.0) : 0.0;
      double dose = dose_elem ? dose_elem->DoubleAttribute("value", 0.0) : 0.0;
      if (!engine.ApplySubstanceBolus(substance, concentration, dose)) { return -1.0; }
    }
    return 0.0;
  }

  // ── PainStimulus (pain from scenarios) ───────────────────────────────────
  if (type == "PainStimulusData") {
    const char* location_attr = action->Attribute("Location");
    const std::string location = location_attr ? location_attr : "Arm";
    const tinyxml2::XMLElement* sev = action->FirstChildElement("Severity");
    const double severity = sev ? sev->DoubleAttribute("value", 0.0) : 0.0;
    if (!engine.ApplyPainStimulus(location, severity)) { return -1.0; }
    return 0.0;
  }

  // ── SubstanceOralDose (oral medication from scenarios) ───────────────────
  if (type == "SubstanceOralDoseData") {
    const tinyxml2::XMLElement* sub_elem  = action->FirstChildElement("Substance");
    const tinyxml2::XMLElement* dose_elem = action->FirstChildElement("Dose");
    if (sub_elem && sub_elem->GetText()) {
      const std::string substance = sub_elem->GetText();
      double dose_mg = dose_elem ? dose_elem->DoubleAttribute("value", 0.0) : 0.0;
      if (!engine.ApplySubstanceOralDose(substance, dose_mg)) { return -1.0; }
    }
    return 0.0;
  }

  // Unknown action type — skip silently
  return 0.0;
}

// ─────────────────────────────────────────────────────────────────────────────
// Run (worker thread)
// ─────────────────────────────────────────────────────────────────────────────

void ScenarioPlayer::Run(ScenarioInfo info)
{
  tinyxml2::XMLDocument doc;
  if (doc.LoadFile(info.file_path.c_str()) != tinyxml2::XML_SUCCESS) {
    std::lock_guard<std::mutex> lock(status_mutex_);
    status_.active = false;
    status_.error  = "Failed to parse scenario XML: " + info.file_path;
    running_.store(false);
    return;
  }

  const tinyxml2::XMLElement* scenario = doc.FirstChildElement("Scenario");
  if (!scenario) {
    std::lock_guard<std::mutex> lock(status_mutex_);
    status_.active = false;
    status_.error  = "No <Scenario> root element in: " + info.file_path;
    running_.store(false);
    return;
  }

  const tinyxml2::XMLElement* actions_root = scenario->FirstChildElement("Actions");
  if (!actions_root) {
    std::lock_guard<std::mutex> lock(status_mutex_);
    status_.active = false;
    running_.store(false);
    return;
  }

  const int total = CountActions(actions_root);
  {
    std::lock_guard<std::mutex> lock(status_mutex_);
    status_.actions_total = total;
  }

  int completed = 0;
  double elapsed_sim_s = 0.0;

  for (const tinyxml2::XMLElement* action = actions_root->FirstChildElement("Action");
       action && running_.load();
       action = action->NextSiblingElement("Action"))
  {
    const double result = DispatchAction(action, engine_, speed_multiplier_, running_);
    if (result > 0.0) {
      elapsed_sim_s += result;
    }
    ++completed;
    {
      std::lock_guard<std::mutex> lock(status_mutex_);
      status_.actions_completed = completed;
      status_.elapsed_sim_s     = elapsed_sim_s;
    }
  }

  std::lock_guard<std::mutex> lock(status_mutex_);
  status_.active = false;
  running_.store(false);
}
