#include "webserver.h"

#include <iostream>
#include <filesystem>
#include <fstream>

#include "llm_api.h"

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
  return ReadFile("resources/html/reconcile/medication.html");
}

std::string HandleSubmit(const std::string &jsonData) {
  s_LLM.ParseJSON(jsonData);
  return "HTTP/1.1 200 OK\r\n";
}

int main(void) {

  Webserver server(8080);

  server.HandleRoute(_Method::GET, "/api/reconcile/medication", HandleRoot);
  server.HandleRoute(_Method::POST, "/api/reconcile/medication", HandleSubmit);

  server.Run();
 
  return 0;

}
