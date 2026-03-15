#include "llm_api.h"
#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include <iostream>
#include <sstream>
#include <iomanip>

#include <ctime>

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

LlmAPI::LlmAPI() {
  m_CURL = curl_easy_init();
}

LlmAPI::~LlmAPI() {
  curl_easy_cleanup(m_CURL);
}

void LlmAPI::Init(const std::string &apiKey) {
  m_ApiKey = apiKey;
}

LlmResponseData LlmAPI::ParseJSON(const std::string &request, const std::string &jsonBody) {
  std::string readBuffer;
  std::string reconcilePrompt = R"**(
    Constraint Strategy:
    1. Output ONLY a valid JSON object. 
    2. NO markdown, NO newlines, NO conversational text.
    3. If input data contains quotes or special characters, they must be escaped according to RFC 8259, or replaced with a space if that preserves clarity.
    4. If an error occurs, return {"error": "description_of_parsing_failure"}.

    JSON Schema:
    {
    "reconciled_medication": "string",
    "confidence_score": (0-100),
    "reasoning": "string (Max 2 sentences. Focus on source reliability/recency. Strip all nested quotes from source data to prevent syntax breaks.)",
    "recommended_actions": ["string"],
    "clinical_safety_check": "PASSED|NEEDS ATTENTION|FAILED"
    }

    Processing Logic:
    1. Normalize Input: Strip all newlines and unescaped quotes from patient data before analysis.
    2. Reconcile: Compare sources by recency and reliability.
    3. Validate: Ensure the JSON object has NO internal line breaks.
    4. Final Output: Strictly JSON string.

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

  if(m_CURL) {
    std::string url = "https://generativelanguage.googleapis.com/v1beta/models/gemini-flash-latest:generateContent?key=" + m_ApiKey;

    std::string prompt = request == "RECONCILE" ? reconcilePrompt : request == "VALIDATE" ? validatePrompt : "{ \"error\": \"invalid backend prompt type\" }";

    if (prompt == "{ \"error\": \"invalid backend prompt type\" }") {
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

      // 1. Check if the expected structure exists
      if (j.contains("candidates") && j["candidates"].is_array() && !j["candidates"].empty()) {
        auto& first_candidate = j["candidates"][0];

        // 2. Safely traverse deeper
        if (first_candidate.contains("content") && 
          first_candidate["content"].contains("parts") && 
          !first_candidate["content"]["parts"].empty()) {

          std::string text = first_candidate["content"]["parts"][0].value("text", "");
          return { LlmResponseCode::OK, text };
        }
      }

      // If we reach here, the JSON was valid, but the structure wasn't what we expected
      return { LlmResponseCode::PARSING_ERROR, "Unexpected JSON structure" };
    } catch (const nlohmann::json::exception& e) {
      return {
        LlmResponseCode::PARSING_ERROR,
        "{ \"code\": \"PARSING_ERROR\", \"message\": \"" + std::string(e.what()) + "\" }"
      };
    }
  }

  return {
    LlmResponseCode::OUT_OF_BOUNDS,
    "{ \"code\": \"OUT_OF_BOUNDS\", \"message\": \"Function 'ParseJSON' went out of bounds\" }"
  };

}
