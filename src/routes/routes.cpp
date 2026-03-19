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
    Role: Clinical Data Reconciler (JSON-Only).
    Objective: High-speed, prioritized medication reconciliation.

    Reconciliation Hierarchy (MANDATORY):
    1. PRIMARY: Source Reliability (High > Medium > Low).
    2. SECONDARY: Recency (Latest date trumps older dates in the same tier).
    3. CONFLICT: High-reliability sources override Low-reliability regardless of date.

    Strict Rules:
    1. OUTPUT: Raw single-line JSON only. No markdown, no whitespace, no preamble.
    2. LIMITS: 'reasoning' (25 words). 'recommended_actions' (max 2 items, < 8 words each).
    3. KEYS: lowercase_with_underscores.

    Reasoning Requirement:
    - Template: "[Source A] prioritized over [Source B] due to [Reliability/Recency]; [observation]."

    JSON Schema:
    {
    "reconciled_medication": "string",
    "confidence_score": (0-100),
    "reasoning": "string",
    "recommended_actions": ["string"],
    "clinical_safety_check": "PASSED|NEEDS ATTENTION|FAILED"
    }

    Task:
    1. Reconcile input using the Hierarchy.
    2. Draft the reasoning using the Comparison Template to ensure specific source attribution.
    3. Output the JSON string.

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

  LlmResponseData data = LlmAPI::ProcessPrompt(prompt + jsonData);
  if (data.ResponseCode == LlmResponseCode::OK) {
      return "HTTP/1.1 200 OK\r\n\r\n" + data.JsonBody;
  }

  return "HTTP/1.1 500 Internal Server Error\r\n\r\n" + data.JsonBody;
}
