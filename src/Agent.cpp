#include "Agent.h"
#include <curl/curl.h>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

Agent::Agent(const std::string& serverUrl) : url(serverUrl) {}

std::string Agent::postRequest(const std::string& jsonBody) {
    CURL* curl = curl_easy_init();
    std::string response;

    if (!curl) return "";

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonBody.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        std::cerr << "curl error: " << curl_easy_strerror(res) << "\n";
        return "";
    }

    return response;
}

std::string Agent::chat(const std::string& userMessage) {
    json request = {
        {"messages", {
            {{"role", "user"}, {"content", userMessage}}
        }},
        {"temperature", 0.7}
    };

    std::string responseStr = postRequest(request.dump());

    try {
        json response = json::parse(responseStr);

        return response["choices"][0]["message"]["content"];
    }
    catch (const std::exception& e) {
        std::cerr << "JSON parse error: " << e.what() << "\n";
        return "";
    }
}