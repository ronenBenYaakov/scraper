#pragma once

#include <string>
#include <vector>

#include <gumbo.h>

class SiteScraper {
public:
    SiteScraper();
    ~SiteScraper();

    bool fetch(const std::string& url);

    std::vector<std::string> getLinks() const;
    std::string getText() const;

private:
    GumboOutput* output;

    std::string downloadHtml(const std::string& url);

    void parseHtml(const std::string& html);
    void freeOutput();

    void traverseForLinks(GumboNode* node, std::vector<std::string>& links) const;
    void traverseForText(GumboNode* node, std::string& text) const;
};