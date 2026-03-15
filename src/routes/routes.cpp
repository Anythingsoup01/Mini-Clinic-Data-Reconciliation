#include "routes/routes.h"

#include <fstream>
#include <nlohmann/json.hpp>

#include "llm_api.h"

using json = nlohmann::json;

static std::string ReadFile(const std::filesystem::path &filePath) {
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

static std::unordered_map<std::string, std::string> s_AuthorisedCredentials = {
  {"admin", "admin"}
};



std::string HandleRoot(const std::string &jsonData) {
  return "HTTP/1.1 200 OK\r\n\r\n" + ReadFile("resources/html/index.html");
}

std::string HandleLogin(const std::string &jsonData) {
  return "HTTP/1.1 200 OK\r\n\r\n" + ReadFile("resources/html/login.html");
}

std::string HandleLoginLogic(const std::string &jsonData) {
  // For example use, such as this, a full datebase isn't necessary
  json j = json::parse(jsonData);

  std::string username = j["user"];
  std::string password = j["pass"];

  if (s_AuthorisedCredentials.contains(username)) {
    if (s_AuthorisedCredentials[username] == password) {
      return "HTTP/1.1 200 OK\r\n\r\n";
    }
  }
  return "HTTP/1.1 401 Unauthorized user";
}

std::string HandleReconcileMedication(const std::string &jsonData) {
  LlmResponseData data = LlmAPI::ParseJSON("RECONCILE", jsonData);

  if (data.ResponseCode == LlmResponseCode::OK) {
      return "HTTP/1.1 200 OK\r\n\r\n" + data.JsonBody;
  }

  return "HTTP/1.1 500 Internal Server Error\r\n\r\n" + data.JsonBody;
}

std::string HandleValidateDataQuality(const std::string &jsonData) {
  LlmResponseData data = LlmAPI::ParseJSON("VALIDATE", jsonData);
  if (data.ResponseCode == LlmResponseCode::OK) {
      return "HTTP/1.1 200 OK\r\n\r\n" + data.JsonBody;
  }

  return "HTTP/1.1 500 Internal Server Error\r\n\r\n" + data.JsonBody;
}
