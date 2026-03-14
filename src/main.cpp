#include "webserver.h"

#include <iostream>
#include <filesystem>
#include <fstream>

#include <yaml-cpp/yaml.h>

#include "llm_api.h"

struct _Config {
  std::string ApiKey;
  int32_t Port;
};

std::string ReadFile(const std::filesystem::path &filePath) {
  std::ifstream in(filePath, std::ios::binary);
  if (in)
  {
    std::string contents;
    in.seekg(0, std::ios::end);
    contents.resize(in.tellg());
    in.seekg(0, std::ios::beg);
    in.read(&contents[0], contents.size());
    in.close();
    return contents;
  }
  return "";
}

static LlmAPI s_LLM;

std::string HandleRoot(const std::string &jsonData) {
  return ReadFile("resources/html/index.html");
}

std::string HandleDebugPage(const std::string &jsonData) {
  return ReadFile("resources/html/debug.html");
}
// TODO: Return a status from ParseJSON

std::string HandleReconcileMedication(const std::string &jsonData) {
  s_LLM.ParseJSON("RECONCILE", jsonData);
  return "HTTP/1.1 200 OK\r\n";
}

std::string HandleValidateDataQuality(const std::string &jsonData) {
  
  return "HTTP/1.1 200 OK\r\n";
}

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


  //s_LLM.Init();

  Webserver server(8080);

  server.HandleRoute(_Method::GET, "/api/home", HandleRoot);
  server.HandleRoute(_Method::POST, "/api/reconcile/medication", HandleReconcileMedication);
  server.HandleRoute(_Method::POST, "/api/validate/data_quality", HandleValidateDataQuality);

  server.HandleRoute(_Method::GET, "/api/debug", HandleDebugPage);


  server.Run();
 
  return 0;

}
