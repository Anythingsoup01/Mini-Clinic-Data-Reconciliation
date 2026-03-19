#pragma once

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
  bool Init(uint16_t port);
  void Shutdown();

  void Run();

  void HandleRoute(const _Method &method, const std::string &route, const std::function<std::string(const std::string &)>& func);
private:
  int m_FD;
  std::unordered_map<std::string, std::function<std::string(const std::string &)>> m_GETHandles;
  std::unordered_map<std::string, std::function<std::string(const std::string &)>> m_POSTHandles;
};
