#include "llm_api.h"
#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include <iostream>
#include <sstream>
#include <iomanip>

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

  std::string validatePrompt;

  if(m_CURL) {
    std::string url = "https://generativelanguage.googleapis.com/v1beta/models/gemini-flash-latest:generateContent?key=AIzaSyDNEZSlfAlVeV9R5hIwLRtM1blMF97m-hA";

    std::string prompt = request == "RECONCILE" ? reconcilePrompt : validatePrompt;
    prompt += jsonBody;

    std::string json_body = "{\"contents\":[{\"parts\":[{\"text\":\"" + json_escape(prompt) + "\"}]}]}";

    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(m_CURL, CURLOPT_URL, url.c_str());
    curl_easy_setopt(m_CURL, CURLOPT_POSTFIELDS, json_body.c_str());
    curl_easy_setopt(m_CURL, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(m_CURL, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(m_CURL, CURLOPT_WRITEDATA, &readBuffer);

    curl_easy_perform(m_CURL);

    std::cout << readBuffer << "\n";

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
