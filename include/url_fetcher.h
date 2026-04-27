#pragma once
#include <string>
#include <vector>

struct ScrapedURL {
    std::string url;
};

class URLFetcher {
public:
    virtual ~URLFetcher() = default;

    virtual std::vector<ScrapedURL> search(const std::string& query) = 0;
};