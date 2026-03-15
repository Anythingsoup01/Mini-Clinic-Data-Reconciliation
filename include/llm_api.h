#pragma once

#include <string>
#include <curl/curl.h>

enum class LlmResponseCode {
  OK,
  INVALID_PROMPT,
  PARSING_ERROR,
  OUT_OF_BOUNDS
};

struct LlmResponseData {
  LlmResponseCode ResponseCode;
  std::string JsonBody;
};

class LlmAPI {
public:
  LlmAPI();
  ~LlmAPI();

  void Init(const std::string &apiKey);

  LlmResponseData ParseJSON(const std::string &request, const std::string &jsonBody);
private:
  CURL *m_CURL;
  std::string m_ApiKey;

  std::string m_ReadBuffer;
};
