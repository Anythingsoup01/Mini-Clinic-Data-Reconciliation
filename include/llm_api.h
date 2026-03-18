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
  static bool Init(const std::string &apiKey);
  static void Shutdown();

  static LlmResponseData ParseJSON(const std::string &request, const std::string &jsonBody);
private:
  static inline CURL *m_CURL = nullptr;
  static inline std::string m_ApiKey = "";
};
