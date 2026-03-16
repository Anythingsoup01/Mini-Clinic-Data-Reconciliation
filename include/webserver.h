#pragma once

#include <stdint.h>
#include <functional>
#include <unordered_map>
#include <string>

enum class _Method {
  UNSUPPORTED,
  GET,
  POST,
};

struct ResponseData {
  _Method Method;
  std::string Route;
  std::string JsonData;
};

class Webserver {
public:
  Webserver(uint16_t port);
  ~Webserver();

  void Run();

  void HandleRoute(const _Method &method, const std::string &route, const std::function<std::string(const std::string &)>& func);

  ResponseData GetData(int fd);

private:
  int m_FD;
  std::unordered_map<std::string, std::function<std::string(const std::string &)>> m_GETHandles;
  std::unordered_map<std::string, std::function<std::string(const std::string &)>> m_POSTHandles;
};
