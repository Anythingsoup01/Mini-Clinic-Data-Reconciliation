#include "webserver.h"
#include "routes/routes.h"
#include "llm_api.h"
#include "serializer.h"

#include "log.h"

#include <signal.h>

Webserver m_Server;

void signalHandler(int signum) {
  std::string msg = "Interrupt signal";
  LogInfo(msg.c_str());

  LogInfo("Server Stopped Running");

  LlmAPI::Shutdown();
  m_Server.Shutdown();

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


  m_Server.Init(config.Port);

  //
  //  GET
  //

  m_Server.HandleRoute(_Method::GET, "/api/home", HandleHome);
  m_Server.HandleRoute(_Method::GET, "/api/styles.css", HandleHomeStyles);
  m_Server.HandleRoute(_Method::GET, "/api/scripts.js", HandleHomeScripts);

  m_Server.HandleRoute(_Method::GET, "/api/login", HandleLogin);
  
  //
  //  POST
  //

  m_Server.HandleRoute(_Method::POST, "/api/auth/login", HandleLoginLogic);
  m_Server.HandleRoute(_Method::POST, "/api/reconcile/medication", HandleReconcileMedication);
  m_Server.HandleRoute(_Method::POST, "/api/validate/data_quality", HandleValidateDataQuality);

  LogInfo("Server Running");
  m_Server.Run();
 
  return 0;
}
