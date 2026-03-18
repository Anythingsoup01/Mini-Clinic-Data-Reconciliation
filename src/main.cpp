#include "webserver.h"
#include "routes/routes.h"
#include "llm_api.h"
#include "serializer.h"

#include "log.h"

#include <signal.h>

Webserver server;

void signalHandler(int signum) {
  std::string msg = "Interrupt signal";
  LogInfo(msg.c_str());

  LogInfo("Server Stopped Running");

  LlmAPI::Shutdown();
  server.Shutdown();

  exit(EXIT_SUCCESS);
}

int main(void) {

  signal(SIGINT, signalHandler);

  LogInfo("Loading Config");
  _Config config = LoadConfig();
  if (config.ApiKey.empty()) {
    LogError("Failed to load config, file missing or invalid field");
    exit(EXIT_FAILURE);
  }
  LogInfo("Config Loaded");

  LogInfo("Initializing LLM");
  if (!LlmAPI::Init(config.ApiKey)) {
    LogError("LLM failed to initialize, exiting");
    exit(EXIT_FAILURE);
  }
  LogInfo("LLM Initialized");


  server.Init(config.Port);

  //
  //  GET
  //

  server.HandleRoute(_Method::GET, "/api/home", HandleHome);
  server.HandleRoute(_Method::GET, "/api/styles.css", HandleHomeStyles);
  server.HandleRoute(_Method::GET, "/api/scripts.js", HandleHomeScripts);

  server.HandleRoute(_Method::GET, "/api/login", HandleLogin);
  
  //
  //  POST
  //

  server.HandleRoute(_Method::POST, "/api/auth/login", HandleLoginLogic);
  server.HandleRoute(_Method::POST, "/api/reconcile/medication", HandleReconcileMedication);
  server.HandleRoute(_Method::POST, "/api/validate/data_quality", HandleValidateDataQuality);

  LogInfo("Server Running");
  server.Run();

 
  return 0;

}
