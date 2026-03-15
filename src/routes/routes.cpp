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

std::string HandleDebugPage(const std::string &jsonData) {
  return "HTTP/1.1 200 OK\r\n\r\n" + ReadFile("resources/html/debug.html");
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
  return "HTTP/1.1 401 OK";
}

std::string HandleReconcileMedication(const std::string &jsonData) {
  LlmResponseData data = LlmAPI::ParseJSON("RECONCILE", jsonData);
  switch (data.ResponseCode) {
    case LlmResponseCode::OK: {
      return "HTTP/1.1 200 OK\r\n\r\n" + data.JsonBody;
    }
    case LlmResponseCode::INVALID_PROMPT:
    case LlmResponseCode::PARSING_ERROR:
    case LlmResponseCode::OUT_OF_BOUNDS:
      return "HTTP/1.1 500 Internal Server Error\r\n\r\n" + data.JsonBody;
  }
  return "HTTP/1.1 500 Internal Server Error\r\n\r\n{ \"code\": \"OUT_OF_BOUNDS\", \"message\": \"Function 'ParseJSON' went out of bounds\" }";
}

std::string HandleValidateDataQuality(const std::string &jsonData) {
  LlmResponseData data = LlmAPI::ParseJSON("VALIDATE", jsonData);
  switch (data.ResponseCode) {
    case LlmResponseCode::OK: {
      return "HTTP/1.1 200 OK\r\n\r\n" + data.JsonBody;
    }
    case LlmResponseCode::INVALID_PROMPT:
    case LlmResponseCode::PARSING_ERROR:
    case LlmResponseCode::OUT_OF_BOUNDS:
      return "HTTP/1.1 500 Internal Server Error\r\n\r\n" + data.JsonBody;
  }
  return "HTTP/1.1 500 Internal Server Error\r\n\r\n{ \"code\": \"OUT_OF_BOUNDS\", \"message\": \"Function 'ParseJSON' went out of bounds\" }";
}
