#include "searxng_client.h"

#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <stdexcept>

using json = nlohmann::json;

// helper for curl write callback
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    size_t totalSize = size * nmemb;
    output->append((char*)contents, totalSize);
    return totalSize;
}

SearXNGClient::SearXNGClient(std::string baseUrl)
    : baseUrl(std::move(baseUrl)) {}

std::string SearXNGClient::httpGet(const std::string& url) {
    CURL* curl = curl_easy_init();
    if (!curl) throw std::runtime_error("Failed to init curl");

    std::string response;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    // IMPORTANT: some SearXNG setups require user-agent
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0");

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        throw std::runtime_error("HTTP request failed");
    }

    return response;
}

std::vector<ScrapedURL> SearXNGClient::search(const std::string& query) {
    std::string url = baseUrl + "/search?q=" + curl_easy_escape(nullptr, query.c_str(), query.length())
        + "&format=json";

    std::string body = httpGet(url);

    json j;
    try {
        j = json::parse(body);
    } catch (...) {
        throw std::runtime_error("Failed to parse JSON (likely not JSON response)");
    }

    std::vector<ScrapedURL> results;

    if (!j.contains("results")) return results;

    for (auto& item : j["results"]) {
        ScrapedURL r;
        r.url = item.value("url", "");
        results.push_back(r);
    }

    return results;
}