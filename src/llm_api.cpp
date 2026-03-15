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

std::string LlmAPI::ParseJSON(const std::string &request, const std::string &jsonBody) {
  std::string readBuffer;
  std::string reconcilePrompt = R"**(
    System Instruction: Clinical Data Reconciler
    Role: You are an expert Clinical Data Reconciler. Your objective is to parse unstructured clinical data and raw screenshots to generate a structured, verified medication reconciliation report in JSON format.
    CRITICAL INSTRUCTION: You are a data extraction engine. Do not output any conversational text, explanations, or pleasantries. You must respond only with raw JSON that adheres exactly to the schema below. If you cannot parse the data, return an error object inside the JSON. Do not use Markdown code blocks if possible; just provide the raw JSON.
    Task:
    Analyze the provided Patient Context and Sources.
    Perform a reconciliation to identify the most accurate, current medication list.
    Assess the reliability of the sources to determine a Confidence Score.
    Output your analysis strictly according to the defined JSON schema.
    You will recieve data formatted as such:
    Patient Context:
      Age: Integer,
      Conditions: List[String],
      Recent Labs: Dict[String -> Integer],
      Sources: List[
       System: Location,
       Medication: Medicine Dosage Frequency,
       Last Updated: Date of last changed,
       Reliability: (low, medium, high)
     ]
    Output Contraints:
    CRITICAL INSTRUCTION: Do not repeat any data that is provided to you. You must start your response with '{', Do Not Use Markdown Notation.
    You must return only a valid JSON object."
    The output must formatted as followed:
    Reconciled Medication: Medicine Dosage Frequency of the Most likely perscription,
    Confidence Score: 0-100 of the Most likely perscription,
    Reasoning: String,
    Recommended Actions: List[String] (What should be done internally to make sure everyone is on the same page),
    Clinical Safety Check: Whether They Passed or Not (Based on confidence)
    Input Data:

  )**";

  std::string validatePrompt = R"**(
    System Instruction: Clinical Data Analyser
    Role: You are an expert Clinical Data Analyser. Your objective is to parse unstructured clinical data and raw screenshots to generate a structured, verified medication data report in JSON format.
    CRITICAL INSTRUCTION: You are a data extraction engine. Do not output any conversational text, explanations, or pleasantries. You must respond only with raw JSON that adheres exactly to the schema below. If you cannot parse the data, return an error object inside the JSON. Do not use Markdown code blocks if possible; just provide the raw JSON.
    Task:
    Analyze the provided paitient information.
    Output your analysis strictly according to the defined JSON schema.
    You will recieve data formatted as such:
    Demographics: Dic[String -> String] (This will contain a name, date of birth, and gender),
    Medications: List[String] (This will contain a list of medications and the dosages),
    Alergies: List[String],
    Conditions: List[String],
    Vital Signs: Dict[String -> Any] (This will contain metrics like blood pressure and heart rate),
    Last Updated: String (Format YYYY-MM-DD),
    Output Contraints:
    CRITICAL INSTRUCTION: Do not repeat any data that is provided to you. You must start your response with '{', Do Not Use Markdown Notation.
    You must return only a valid JSON object."
    The output must formatted as followed:
    Overall Score: Integer (0-100) (This is the overall rating of the provided data)
    Breakdown: {
      Completeness: Integer (0-100) (How complete the information is)
      Accuracy: Integer (0-100) (How accurate the information is)
      Timeliness: Integer (0-100) (Goes off of the Last Update field)
      Clinical Plausibility: Integer (0-100) (How plausible this information is)
    }
    Issues Detected: List[Dict[String -> String]] {
      CRITICAL INSTRUCTION: Demographics don't count as fields. All fields need to be lowercased and use '_' for any spaces only for the field. After all fields are noted. The following fields pertain to the aformentioned information, only list important fields. This includes but isn't limited to impossible vitals signs and drug-disease mismatches. If the data pertains to Vitals, the field name needs to be as followed "vital_sign.'field'", this only applies to vitals. Add last_updated to the very end, and display how many months are have gone by from the provided date, this should look as followed "Data is #+ months old", if the data is half a year or higher that is a medium Severity a year or more is high.
      Field: String,
      Issue: String (What about the field is the issue, if there is no documented items you must state "No 'field'(s) documented - likely incomplete")
      Severity: String (How severe the issue is from low - high)
    }
    Input Data:

  )**";

  if(m_CURL) {
    std::string url = "https://generativelanguage.googleapis.com/v1beta/models/gemini-flash-latest:generateContent?key=" + m_ApiKey;

    std::string prompt = request == "RECONCILE" ? reconcilePrompt : validatePrompt;
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

      std::string text = j["candidates"][0]["content"]["parts"][0]["text"].get<std::string>();

      std::cout << text << "\n";
      return text;
    } catch (const nlohmann::json::exception& e) {
      return "Error parsing API response: " + std::string(e.what());
    }
  }
  return "";
}
