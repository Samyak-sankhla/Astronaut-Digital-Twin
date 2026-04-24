#pragma once

#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

class BioGearsEngineManager;

class ScenarioPlayer {
public:
  struct ScenarioInfo {
    std::string name;
    std::string description;
    std::string category;   // subdirectory name (e.g. "Astronaut", "Exercise")
    std::string file_path;  // absolute path to the XML file
  };

  struct Status {
    bool        active            = false;
    std::string scenario_name;
    std::string category;
    int         actions_completed = 0;
    int         actions_total     = 0;
    double      elapsed_sim_s     = 0.0;
    std::string error;
  };

  explicit ScenarioPlayer(BioGearsEngineManager& engine, double speed_multiplier = 1.0);
  ~ScenarioPlayer();

  // Scan <scenarios_root>/**/*.xml and return metadata for every scenario found.
  std::vector<ScenarioInfo> ListScenarios(const std::string& scenarios_root) const;

  // Start playing a scenario (non-blocking — fires a worker thread).
  // Returns false if a scenario is already active or the file cannot be parsed.
  bool Play(const ScenarioInfo& info);

  // Convenience: look up by name in <scenarios_root> then play.
  bool PlayByName(const std::string& name, const std::string& scenarios_root);

  // Request the current scenario to stop. Blocks until the worker exits.
  void Stop();

  bool   IsActive() const { return running_.load(); }
  Status GetStatus() const;

private:
  void Run(ScenarioInfo info);

  BioGearsEngineManager& engine_;
  double                 speed_multiplier_;

  mutable std::mutex status_mutex_;
  Status             status_;

  std::atomic<bool> running_{false};
  std::thread       worker_;
};
