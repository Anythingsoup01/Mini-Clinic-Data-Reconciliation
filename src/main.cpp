#include "webserver.h"

#include <iostream>
#include <filesystem>
#include <fstream>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

void ProcessReceivedData(const std::string &rawBody) {
  try {
    json j = json::parse(rawBody);

    std::cout << "--- Patient Context ---\n";

    std::string age = j["patient_context"]["age"];
    std::cout << "Age: "<< age << "\n";

    std::cout << "Conditions: ";
    for (auto &condition : j["patient_context"]["conditions"]) {
      std::cout << "[" << condition << "] ";
    }
    std::cout << "\n";

    std::cout << "--- Medication Sources ---" << std::endl;
    for (const auto& source : j["sources"]) {
      std::cout << "Source: " << source["system"] << " | Med: " << source["medication"] << "\n";
    }

  } catch (json::parse_error &e) {
    std::cerr << "Failed to parse JSON: " << e.what() << "\n";
  }
}

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

std::string HandleRoot(const std::string &jsonData) {
  return ReadFile("resources/html/reconcile/medication.html");
}

std::string HandleSubmit(const std::string &jsonData) {
  ProcessReceivedData(jsonData);

  return "HTTP/1.1 200 OK\r\n";
}

int main(void) {

  Webserver server(8080);

  server.HandleRoute(_Method::GET, "/api/reconcile/medication", HandleRoot);
  server.HandleRoute(_Method::POST, "/api/reconcile/medication", HandleSubmit);

  server.Run();
 
  return 0;

}
