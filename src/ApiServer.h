#pragma once

#include <atomic>
#include <thread>

#include <httplib.h>

class EventProcessor;
class PhysiologyState;

class ApiServer {
public:
  ApiServer(EventProcessor& event_processor, const PhysiologyState& physiology_state, int port = 8080);
  ~ApiServer();

  bool Start();
  void Stop();
  bool IsRunning() const;

private:
  void ConfigureRoutes();

  EventProcessor& event_processor_;
  const PhysiologyState& physiology_state_;
  int port_;

  httplib::Server server_;
  std::atomic<bool> running_{false};
  std::thread server_thread_;
};
