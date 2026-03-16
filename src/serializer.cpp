#include "serializer.h"

#include <fstream>
#include <sstream>


_Config LoadConfig() {

 _Config config = {"", 0};
  std::ifstream file(".config");

  if (!file.is_open()) {
    printf("ERROR: Could not open config file\n");
    return config;
  }

  std::string line;
  while (std::getline(file, line)) {
    std::istringstream is_line(line);
    std::string key;
    if (std::getline(is_line, key, ':')) {
      std::string value;
      if (std::getline(is_line, value)) {
        // Trim whitespace
        value.erase(0, value.find_first_not_of(" \t"));

        if (key == "API_KEY") {
          config.ApiKey = value;
        } else if (key == "WEBSERVER_PORT") {
          config.Port = std::stoi(value);
        }
      }
    }
  }

  return config;
}
