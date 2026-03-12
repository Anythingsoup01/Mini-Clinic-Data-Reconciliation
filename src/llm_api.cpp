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

std::string LlmAPI::ParseJSON(const std::string &jsonBody) {
  std::string readBuffer;
  std::string prompt = 
    "System Instruction: Clinical Data Reconciler\n"
    "Role: You are an expert Clinical Data Reconciler. Your objective is to parse unstructured clinical data and raw screenshots to generate a structured, verified medication reconciliation report in JSON format.\n"
    "CRITICAL INSTRUCTION: You are a data extraction engine. Do not output any conversational text, explanations, or pleasantries. You must respond only with raw JSON that adheres exactly to the schema below. If you cannot parse the data, return an error object inside the JSON. Do not use Markdown code blocks if possible; just provide the raw JSON.\n"
    "Task:\n"
    "Analyze the provided Patient Context and Sources.\n"
    "Perform a reconciliation to identify the most accurate, current medication list.\n"
    "Assess the reliability of the sources to determine a Confidence Score.\n"
    "Output your analysis strictly according to the defined JSON schema.\n"
    "You will recieve data formatted as such:\n"
    "Patient Context:\n"
    "  Age: Integer,\n"
    "  Conditions: List[String],\n"
    "  Recent Labs: Dict[String -> Integer],\n"
    "  Sources: List[\n"
    "   System: Location,\n"
    "   Medication: Medicine Dosage Frequency,\n"
    "   Last Updated: Date of last changed,\n"
    "   Reliability: (low, medium, high)\n"
    " ]\n\n"
    "Input Data:\n"
    + jsonBody + 
    "Keep Track Of:\n"
    "INSTRUCTION: Look throught the Sources section of the Patient Context and determine the most probable up to date information by the System section. The date format is as followed: YYYY-MM-DD. The reliability tiers are as followed from lowest to highest: low, medium, high.\n"
    "Most likely perscription - To determine this the dataset must have been updated the LATEST and a reliability score must be THE SAME OR HIGHER to the previous most likely choice. MAKE SURE DOSAGES AND MEDICINES ARE CORRECT.\n"
    "Output Contraints:\n"
    "CRITICAL INSTRUCTION: Do not repeat any data that is provided to you. You must start your response with '{', Do Not Use Markdown Notation.\n"
    "You must return only a valid JSON object.\n"
    "The output must formatted as followed:\n"
    "Reconciled Medication: Medicine Dosage Frequency of the Most likely perscription,\n"
    "Confidence Score: 0-100 of the Most likely perscription,\n"
    "Reasoning: String,\n"
    "Recommended Actions: List[String] (What should be done internally to make sure everyone is on the same page),\n"
    "Clinical Safety Check: Whether They Passed or Not (Based on confidence)\n"; 

  if(m_CURL) {
    std::string url = "https://generativelanguage.googleapis.com/v1beta/models/gemini-flash-latest:generateContent?key=AIzaSyAiqVrzYxXCtynJ35QhGtHdUraIYOQwLIY";
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
