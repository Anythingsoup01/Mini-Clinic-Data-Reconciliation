#include "llm_api.h"
#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include <iostream>
#include <sstream>
#include <iomanip>

#include <ctime>

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
    1. Output ONLY a valid JSON string. NO markdown, NO newlines, NO conversational filler.
    2. NORMALIZE: All keys must be lowercase_with_underscores.
    3. CONCISE DATA: 
    - 'reasoning': Max 15 words. Focus strictly on source delta.
    - 'recommended_actions': Max 2 items, max 10 words each.
    4. If parsing fails, return {"error": "reason"}.

    JSON Schema:
    {
    "reconciled_medication": "string",
    "confidence_score": (0-100),
    "reasoning": "string",
    "recommended_actions": ["string"],
    "clinical_safety_check": "PASSED|NEEDS ATTENTION|FAILED"
    }

    Task:
    1. Reconcile input based on source reliability/recency.
    2. Identify the most likely current prescription.
    3. Ensure 'reasoning' and 'recommended_actions' are ultra-short to minimize latency.

    Patient Input Data:
  )**";

  std::string validatePrompt = R"**(
    Role: Expert Clinical Data Analyser. 
    Objective: Parse provided clinical data into a structured JSON report.

    Normalization Protocol (MANDATORY):
    - All field names from the input must be converted to lowercase and use underscores (e.g., "Blood Pressure" -> "blood_pressure").
    - If the input contains "Vital Signs", map them to "vital_sign.[field_name]".

    Strict Operational Rules:
    1. STRICT DATA BINDING: You are prohibited from generating, inferring, or adding any fields not in the input, EXCEPT for mandatory structural checks.
    2. ALLERGY VALIDATION: 'allergies' is a required field. If 'allergies' is missing or empty in the input, you MUST include an object in 'issues_detected' with: {"field": "allergies", "issue": "No allergies documented - likely incomplete", "severity": "medium"}.
    3. Output ONLY raw, single-line JSON. No markdown, no backticks.

    JSON Schema:
    {
      "overall_score": 0,
      "breakdown": {
        "completeness": 0,
        "accuracy": 0,
        "timeliness": 0,
        "clinical_plausibility": 0
      },
      "issues_detected": [
        {
          "field": "string",
          "issue": "string",
          "severity": "low|medium|high"
        }
      ]
    }

    Logic Instructions:
    - For 'last_updated': Calculate months elapsed from March 2026. If >= 6 months, severity is 'medium'. If >= 12 months, 'high'.
    - Use format "vital_sign.[normalized_field_name]" only if the vital sign is present.
    - If no other clinical issues are found, return an empty list or only the allergy issue if applicable.

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

  std::time_t now = std::time(nullptr); // time(NULL) also works

  std::tm* local_time = std::localtime(&now);

  char buffer[80];
  std::strftime(buffer, sizeof(buffer), "%Y-%m-%d", local_time); // Format: YYYY-MM-DD

  prompt += "\n\nCurrent Date:\nYYYY-MM-DD\n" + std::string(buffer);

  std::string json_body = "{\"contents\":[{\"parts\":[{\"text\":\"" + json_escape(prompt) + "\"}]}]}";

  struct curl_slist* headers = NULL;
  headers = curl_slist_append(headers, "Content-Type: application/json");

  curl_easy_setopt(m_CURL, CURLOPT_URL, url.c_str());
  curl_easy_setopt(m_CURL, CURLOPT_POSTFIELDS, json_body.c_str());
  curl_easy_setopt(m_CURL, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(m_CURL, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(m_CURL, CURLOPT_WRITEDATA, &readBuffer);

  curl_easy_perform(m_CURL);

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
    return {
      LlmResponseCode::PARSING_ERROR,
      "{ \"code\": \"PARSING_ERROR\", \"message\": \"" + std::string(e.what()) + "\" }"
    };
  }

  return {
    LlmResponseCode::OUT_OF_BOUNDS,
    "{ \"code\": \"OUT_OF_BOUNDS\", \"message\": \"Function 'ParseJSON' went out of bounds\" }"
  };

}
