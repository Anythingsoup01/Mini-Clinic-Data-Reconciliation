#include "pch.h"
#include "llm_api.h"
#include <nlohmann/json.hpp>

#include <sstream>
#include <iomanip>

#include <ctime>
#include <format>

#include "log.h"

size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* s) {
    s->append((char*)contents, size * nmemb);
    return size * nmemb;
}

std::string json_escape(const std::string &s) {
    std::ostringstream o;
    for (auto c : s) {
        if (c == '"') o << "\\\"";
        else if (c == '\\') o << "\\\\";
        else if (c == '\n') o << "\\n";
        else if (c == '\r') o << "\\r";
        else if (c == '\t') o << "\\t";
        else if (c >= 0 && c < 32) o << "\\u" << std::hex << std::setw(4) << std::setfill('0') << (int)c;
        else o << c;
    }
    return o.str();
}

bool LlmAPI::Init(const std::string &apiKey) {
  m_CURL = curl_easy_init();
  m_ApiKey = apiKey;

  return m_CURL;
}

void LlmAPI::Shutdown() {
  curl_easy_cleanup(m_CURL);
}

LlmResponseData LlmAPI::ProcessPrompt(const std::string &prompt) {
  std::string url = "https://generativelanguage.googleapis.com/v1beta/models/gemini-flash-latest:generateContent?key=" + m_ApiKey;
  std::string json_body = "{\"contents\":[{\"parts\":[{\"text\":\"" + json_escape(prompt) + "\"}]}]}";

  std::string readBuffer;

  struct curl_slist* headers = NULL;
  headers = curl_slist_append(headers, "Content-Type: application/json");

  curl_easy_setopt(m_CURL, CURLOPT_URL, url.c_str());
  curl_easy_setopt(m_CURL, CURLOPT_POSTFIELDS, json_body.c_str());
  curl_easy_setopt(m_CURL, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(m_CURL, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(m_CURL, CURLOPT_WRITEDATA, &readBuffer);

  LogInfo("[LlmAPI] - Sending Request To Gemini");
  auto start = std::chrono::steady_clock::now();
  curl_easy_perform(m_CURL);
  auto end = std::chrono::steady_clock::now();

  std::chrono::duration<double> elapsed = end - start;

  std::string info = std::format("[LlmAPI] - Get Request From Gemini ({}s)", elapsed.count());
  LogInfo(info.c_str());

  try {

    auto j = nlohmann::json::parse(readBuffer);

    if (j.contains("candidates") && j["candidates"].is_array() && !j["candidates"].empty()) {
      auto& first_candidate = j["candidates"][0];

      if (first_candidate.contains("content") && 
        first_candidate["content"].contains("parts") && 
        !first_candidate["content"]["parts"].empty()) {

        std::string text = first_candidate["content"]["parts"][0].value("text", "");
        return { LlmResponseCode::OK, text };
      }
    }

    // If we reach here, the JSON was valid, but the structure wasn't what we expected
    LogWarning("[LlmAPI] - LLM returned unexpected error '" + json_escape(j) + "'");
    return {
      LlmResponseCode::PARSING_ERROR,
      "{\"code\": \"PARSING_ERROR\", \"message\": \"LLM Failed to process request and gave error, check server terminal!\"}" };

  } catch (const nlohmann::json::exception& e) {
    std::string error = e.what();
    LogWarning("[LlmAPI] - LLM returned parsing error '" + error + "'");
    return {
      LlmResponseCode::PARSING_ERROR,
      "{ \"code\": \"PARSING_ERROR\", \"message\": \"" + error + "\" }"
    };
  }

  LogWarning("[LlmAPI::ParseJSON] - OUT OF BOUNDS");

  return {
    LlmResponseCode::OUT_OF_BOUNDS,
    "{ \"code\": \"OUT_OF_BOUNDS\", \"message\": \"Function 'ParseJSON' went out of bounds\" }"
  };

}
