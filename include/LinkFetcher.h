#pragma once

#include <string>
#include <vector>
#include <curl/curl.h>

std::string urlEncode(CURL* curl, const std::string& value);

struct SearchResult {
    std::string title;
    std::string url;
    std::string snippet;
};

class LinkFetcher {
private:
    std::string baseUrl;

    std::string httpGet(const std::string& url);

public:
    LinkFetcher(const std::string& baseUrl);

    std::vector<SearchResult> search(const std::string& query);
};