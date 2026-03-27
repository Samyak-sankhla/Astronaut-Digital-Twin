#include "ApiServer.h"

#include <nlohmann/json.hpp>

#include "EventProcessor.h"
#include "PhysiologyState.h"

ApiServer::ApiServer(EventProcessor& event_processor, const PhysiologyState& physiology_state, int port)
  : event_processor_(event_processor)
  , physiology_state_(physiology_state)
  , port_(port)
{
  ConfigureRoutes();
}

ApiServer::~ApiServer()
{
  Stop();
}

bool ApiServer::Start()
{
  if (running_.exchange(true)) {
    return false;
  }

  server_thread_ = std::thread([this]() {
    server_.listen("0.0.0.0", port_);
    running_.store(false);
  });

  return true;
}

void ApiServer::Stop()
{
  if (!running_.exchange(false)) {
    if (server_thread_.joinable()) {
      server_thread_.join();
    }
    return;
  }

  server_.stop();
  if (server_thread_.joinable()) {
    server_thread_.join();
  }
}

bool ApiServer::IsRunning() const
{
  return running_.load();
}

void ApiServer::ConfigureRoutes()
{
  server_.Get("/state", [this](const httplib::Request&, httplib::Response& res) {
    res.set_content(physiology_state_.ToJson().dump(), "application/json");
  });

  server_.Post("/event/exercise", [this](const httplib::Request& req, httplib::Response& res) {
    const auto payload = nlohmann::json::parse(req.body, nullptr, false);
    if (payload.is_discarded()) {
      res.status = 400;
      res.set_content(R"({"error":"Invalid JSON body"})", "application/json");
      return;
    }

    const double intensity = payload.value("intensity", 0.0);
    const int duration = payload.value("duration", 0);

    if (!event_processor_.TriggerExercise(intensity, duration)) {
      res.status = 500;
      res.set_content(R"({"error":"Failed to apply exercise event"})", "application/json");
      return;
    }

    res.set_content(R"({"status":"ok"})", "application/json");
  });

  server_.Post("/event/hydration", [this](const httplib::Request& req, httplib::Response& res) {
    const auto payload = nlohmann::json::parse(req.body, nullptr, false);
    const double water_liters = payload.is_discarded() ? 0.25 : payload.value("water_liters", 0.25);

    if (!event_processor_.TriggerHydration(water_liters)) {
      res.status = 500;
      res.set_content(R"({"error":"Failed to apply hydration event"})", "application/json");
      return;
    }

    res.set_content(R"({"status":"ok"})", "application/json");
  });

  server_.Post("/event/drug", [this](const httplib::Request& req, httplib::Response& res) {
    const auto payload = nlohmann::json::parse(req.body, nullptr, false);
    if (payload.is_discarded()) {
      res.status = 400;
      res.set_content(R"({"error":"Invalid JSON body"})", "application/json");
      return;
    }

    const std::string substance = payload.value("substance", std::string("Epinephrine"));
    const double concentration = payload.value("concentration_mg_per_ml", 0.0);
    const double rate = payload.value("rate_ml_per_min", 0.0);

    if (!event_processor_.TriggerDrugInfusion(substance, concentration, rate)) {
      res.status = 500;
      res.set_content(R"({"error":"Failed to apply drug event"})", "application/json");
      return;
    }

    res.set_content(R"({"status":"ok"})", "application/json");
  });

  server_.Post("/event/microgravity", [this](const httplib::Request& req, httplib::Response& res) {
    const auto payload = nlohmann::json::parse(req.body, nullptr, false);
    const bool enabled = payload.is_discarded() ? true : payload.value("enabled", true);

    if (!event_processor_.TriggerMicrogravity(enabled)) {
      res.status = 500;
      res.set_content(R"({"error":"Failed to apply microgravity transition"})", "application/json");
      return;
    }

    res.set_content(R"({"status":"ok"})", "application/json");
  });
}
