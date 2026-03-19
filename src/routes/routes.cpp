#include "pch.h"
#include "routes/routes.h"

#include <fstream>
#include <nlohmann/json.hpp>

#include "llm_api.h"

using json = nlohmann::json;

static std::string ReadFile(const std::filesystem::path &filePath) {
  std::ifstream in(filePath, std::ios::binary);
  if (in)
  {
    std::string contents;
    in.seekg(0, std::ios::end);
    contents.resize(in.tellg());
    in.seekg(0, std::ios::beg);
    in.read(&contents[0], contents.size());
    in.close();
    return contents;
  }
  return "";
}

static std::unordered_map<std::string, std::string> s_AuthorisedCredentials = {
  {"admin", "admin"}
};

std::string HandleHome(const std::string &jsonData) {
  return "HTTP/1.1 200 OK\r\n\r\n" + ReadFile("resources/html/index.html");
}

std::string HandleHomeStyles(const std::string &jsonData) {
  return "HTTP/1.1 200 OK\r\n\r\n" + ReadFile("resources/html/styles.css");
}

std::string HandleHomeScripts(const std::string &jsonData) {
  return "HTTP/1.1 200 OK\r\n\r\n" + ReadFile("resources/html/scripts.js");
}

std::string HandleLogin(const std::string &jsonData) {
  return "HTTP/1.1 200 OK\r\n\r\n" + ReadFile("resources/html/login.html");
}

std::string HandleLoginLogic(const std::string &jsonData) {
  // For example use, such as this, a full datebase isn't necessary
  json j = json::parse(jsonData);

  std::string username = j["user"];
  std::string password = j["pass"];

  if (s_AuthorisedCredentials.contains(username)) {
    if (s_AuthorisedCredentials[username] == password) {
      return "HTTP/1.1 200 OK\r\n\r\n";
    }
  }
  return "HTTP/1.1 401 Unauthorized user";
}

std::string HandleReconcileMedication(const std::string &jsonData) {
  std::string prompt = R"**(
    Role: Clinical Data Extractor (JSON-Only).
    Objective: Fast, structured extraction with zero conversational overhead.

    Strict Rules:
    1. OUTPUT: Raw single-line JSON only. No markdown, no whitespace, no preamble.
    2. LIMITS: 'reasoning' <25 words. 'recommended_actions' max 2 items, < 8 words each.
    3. KEYS: lowercase_with_underscores.

    Logic:
    - Reconcile by Recency < Source Reliability.
    - Tie-breaker: If dosage/frequency match across sources, prioritize by reliability in order: [1] High, [2] Medium, [3] Low
    - Identify the most likely current prescription.
    - Ensure 'reasoning' and 'recommended_actions' are ultra-short to minimize latency.

    JSON Schema: {"reconciled_medication":"str","confidence_score":int,"reasoning":"str","recommended_actions":["str"],"clinical_safety_check":"PASSED|NEEDS ATTENTION|FAILED"}

    Patient Input Data:
  )**";

  LlmResponseData data = LlmAPI::ProcessPrompt(prompt + jsonData);

  if (data.ResponseCode == LlmResponseCode::OK) {
      return "HTTP/1.1 200 OK\r\n\r\n" + data.JsonBody;
  }

  return "HTTP/1.1 500 Internal Server Error\r\n\r\n" + data.JsonBody;
}

std::string HandleValidateDataQuality(const std::string &jsonData) {
  std::string prompt = R"**(
    Role: Clinical Data Extractor (JSON-Only).
    Objective: Fast, structured extraction with zero conversational overhead.

    Strict Extraction Rules:
    - Output: Single-line JSON. No Markdown/preamble/whitespace.
    - Case: lowercase_with_underscores.
    - Date Reference: March 2026. (Age >=6mo: medium; >=12mo: high).

    [MAPPING PROTOCOL]
    - Scan for Vital Signs (e.g., BP, HR, Temp) -> Map to "vital_sign.[name]".
    - Gap Check (Mandatory): 
      - Out-of-Range: Any vital/value indicating clinical risk (e.g., BP >140/90, HR >100). Severity: high.
      - Missing Data: If 'allergies' is null/empty. Severity: medium.
      - Untreated: Any 'condition' lacking 'medication'. Issue: "Missing treatment. Recommend: [Med] [Dose]". Severity: high.
    - Limit 'issue' descriptions to <10 words.

    JSON Schema:
    {"overall_score":(0-100),"breakdown":{"completeness":(0-100),"accuracy":(0-100),"timeliness":(0-100),"clinical_plausibility":(0-100)},"issues_detected":[{"field":"string","issue":"string","severity":"low|medium|high"}]}

    Input Data:
  )**";

  LlmResponseData data = LlmAPI::ProcessPrompt(prompt + jsonData);
  if (data.ResponseCode == LlmResponseCode::OK) {
      return "HTTP/1.1 200 OK\r\n\r\n" + data.JsonBody;
  }

  return "HTTP/1.1 500 Internal Server Error\r\n\r\n" + data.JsonBody;
}
