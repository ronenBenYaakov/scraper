#include "LinkFetcher.h"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <iostream>

using json = nlohmann::json;

std::string urlEncode(CURL* curl, const std::string& value) {
    char* encoded = curl_easy_escape(curl, value.c_str(), value.length());
    std::string result(encoded);
    curl_free(encoded);
    return result;
}

// ---------------- CURL CALLBACK ----------------
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// ---------------- CONSTRUCTOR ----------------
LinkFetcher::LinkFetcher(const std::string& baseUrl)
    : baseUrl(baseUrl) {}

// ---------------- HTTP GET ----------------
std::string LinkFetcher::httpGet(const std::string& url) {
    CURL* curl = curl_easy_init();
    std::string response;

    if (!curl) return "";

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl);

    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        std::cerr << "curl error: " << curl_easy_strerror(res) << "\n";
        return "";
    }

    return response;
}

// ---------------- SEARCH ----------------
std::vector<SearchResult> LinkFetcher::search(const std::string& queryRaw) {
    std::vector<SearchResult> results;

    CURL* curl = curl_easy_init();
    if (!curl) return results;

    // ---------------- URL ENCODE ----------------
    char* encoded = curl_easy_escape(curl, queryRaw.c_str(), queryRaw.length());
    if (!encoded) {
        curl_easy_cleanup(curl);
        return results;
    }

    std::string query(encoded);
    curl_free(encoded);

    // ---------------- BUILD URL ----------------
    std::string url =
        baseUrl + "/search?q=" + query + "&format=json";

    // ---------------- FETCH ----------------
    std::string response = httpGet(url);

    curl_easy_cleanup(curl);

    if (response.empty()) return results;

    // ---------------- PARSE JSON ----------------
    try {
        json j = json::parse(response);

        if (!j.contains("results") || !j["results"].is_array())
            return results;

        for (const auto& item : j["results"]) {
            SearchResult r;

            r.title = item.value("title", "");
            r.url = item.value("url", "");
            r.snippet = item.value("content", "");

            if (!r.url.empty())
                results.push_back(std::move(r));
        }
    }
    catch (const std::exception& e) {
        std::cerr << "JSON parse error: " << e.what() << "\n";
    }

    return results;
}