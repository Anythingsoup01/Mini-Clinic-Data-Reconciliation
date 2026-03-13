#pragma once

#include <string>
#include <curl/curl.h>

class LlmAPI {
public:
  LlmAPI();
  ~LlmAPI();

  std::string ParseJSON(const std::string &request, const std::string &jsonBody);
private:
  CURL *m_CURL;
};
