#include "webserver.h"


#include <yaml-cpp/yaml.h>
#include <nlohmann/json.hpp>

#include "llm_api.h"

#include "routes/routes.h"

using json = nlohmann::json;

struct _Config {
  std::string ApiKey;
  int32_t Port;
};



_Config load_config() {

  YAML::Node node;

  try {
    node = YAML::LoadFile("config.yaml");
  } catch (YAML::ParserException e) {
    printf("ERROR: %s\n", e.what());
    return {};
  }
  _Config config;

  config.ApiKey = node["API_KEY"].as<std::string>();
  config.Port = node["WEBSERVER_PORT"].as<int32_t>();

  return config;

}

int main(void) {
  _Config config = load_config();
  if (config.ApiKey.empty()) {
    return -1;
  }

  LlmAPI::Init(config.ApiKey);

  Webserver server(config.Port);

  server.HandleRoute(_Method::GET, "/api/home", HandleRoot);
  server.HandleRoute(_Method::GET, "/api/login", HandleLogin);

  server.HandleRoute(_Method::POST, "/api/auth/login", HandleLoginLogic);
  server.HandleRoute(_Method::POST, "/api/reconcile/medication", HandleReconcileMedication);
  server.HandleRoute(_Method::POST, "/api/validate/data_quality", HandleValidateDataQuality);

  server.HandleRoute(_Method::GET, "/api/debug", HandleDebugPage);


  server.Run();
 
  LlmAPI::Shutdown();
  return 0;

}
