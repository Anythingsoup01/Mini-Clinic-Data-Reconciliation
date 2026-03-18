#include "llm_api.h"
#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include <iostream>
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

LlmResponseData LlmAPI::ParseJSON(const std::string &request, const std::string &jsonBody) {
  std::string readBuffer;
  std::string reconcilePrompt = R"**(
    Role: Clinical Data Extractor (JSON-Only).
    Objective: Fast, structured extraction with zero conversational overhead.

    Strict Extraction Rules:
    1. OUTPUT: Single-line JSON only. No markdown, no whitespace, no preamble.
    2. NORMALIZATION: All keys/fields must be lowercase_with_underscores.
    3. LIMIT: 'issue' description must be under 10 words.
    4. If parsing fails, return {"error": "reason"}.

    JSON Schema:
    {
    "reconciled_medication": "string",
    "confidence_score": (0-100),
    "reasoning": "string",
    "recommended_actions": ["string"],
    "clinical_safety_check": "PASSED|NEEDS ATTENTION|FAILED"
    }

    Logic:
    - Reconcile input based on source reliability/recency.
    - Identify the most likely current prescription.
    - Ensure 'reasoning' and 'recommended_actions' are ultra-short to minimize latency.

    Patient Input Data:
  )**";

  std::string validatePrompt = R"**(
    Role: Clinical Data Extractor (JSON-Only).
    Objective: Fast, structured extraction with zero conversational overhead.

    Strict Extraction Rules:
    1. OUTPUT: Single-line JSON only. No markdown, no whitespace, no preamble.
    2. NORMALIZATION: All keys/fields must be lowercase_with_underscores.
    3. ALLERGY RULE: If 'allergies' is missing/empty, add: {"field":"allergies","issue":"missing","severity":"medium"}.
    4. LIMIT: 'issue' description must be under 10 words.

    JSON Schema:
    {
    "overall_score": 0,
    "breakdown": {"completeness":0, "accuracy":0, "timeliness":0, "clinical_plausibility":0},
    "issues_detected": [{"field":"string", "issue":"string", "severity":"low|medium|high"}]
    }

    Logic:
    - Map "Vital Signs" to "vital_sign.[name]".
    - Evaluate 'last_updated' against March 2026: (>=6mo=medium, >=12mo=high).
    - No hallucinations: If not in input, do not include.

    Input Data:
  )**";

  std::string url = "https://generativelanguage.googleapis.com/v1beta/models/gemini-flash-latest:generateContent?key=" + m_ApiKey;

  std::string prompt = request == "RECONCILE" ? reconcilePrompt : request == "VALIDATE" ? validatePrompt : "{ \"error\": \"invalid backend prompt type\" }";

  if (prompt == "{ \"error\": \"invalid backend prompt type\" }") {
    LogWarning("[LlmAPI] - Invalid prompt type");
    return {
      LlmResponseCode::INVALID_PROMPT,
      prompt
    };
  }

  prompt += jsonBody;

  std::string json_body = "{\"contents\":[{\"parts\":[{\"text\":\"" + json_escape(prompt) + "\"}]}]}";

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
    std::string warning = "[LlmAPI] - LLM returned unexpected error '" + json_escape(j) + "'";
    LogWarning(warning.c_str());
    return {
      LlmResponseCode::PARSING_ERROR,
      "{\"code\": \"PARSING_ERROR\", \"message\": \"LLM Failed to process request and gave error, check server terminal!\"}" };

  } catch (const nlohmann::json::exception& e) {
    std::string error = e.what();
    std::string warning = "[LlmAPI] - LLM returned parsing error '" + error + "'";
    LogWarning(warning.c_str());
    return {
      LlmResponseCode::PARSING_ERROR,
      "{ \"code\": \"PARSING_ERROR\", \"message\": \"" + error + "\" }"
    };
  }

  std::string warning = "[LlmAPI::ParseJSON] - OUT OF BOUNDS";
  LogWarning(warning.c_str());

  return {
    LlmResponseCode::OUT_OF_BOUNDS,
    "{ \"code\": \"OUT_OF_BOUNDS\", \"message\": \"Function 'ParseJSON' went out of bounds\" }"
  };

}
