#include "SiteScraper.h"

#include <curl/curl.h>
#include <iostream>

SiteScraper::SiteScraper() : output(nullptr) {
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

SiteScraper::~SiteScraper() {
    freeOutput();
    curl_global_cleanup();
}

void SiteScraper::freeOutput() {
    if (output) {
        gumbo_destroy_output(&kGumboDefaultOptions, output);
        output = nullptr;
    }
}

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* s) {
    size_t totalSize = size * nmemb;
    s->append((char*)contents, totalSize);
    return totalSize;
}

std::string SiteScraper::downloadHtml(const std::string& url) {
    CURL* curl = curl_easy_init();
    std::string response;

    if (!curl) return "";

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
        std::cerr << "curl error: " << curl_easy_strerror(res) << "\n";
    }

    curl_easy_cleanup(curl);
    return response;
}

void SiteScraper::parseHtml(const std::string& html) {
    freeOutput();
    output = gumbo_parse(html.c_str());
}

bool SiteScraper::fetch(const std::string& url) {
    std::string html = downloadHtml(url);

    if (html.empty()) {
        return false;
    }

    parseHtml(html);
    return true;
}

void SiteScraper::traverseForLinks(GumboNode* node, std::vector<std::string>& links) const {
    if (!node) return;

    if (node->type == GUMBO_NODE_ELEMENT) {
        GumboElement* element = &node->v.element;

        if (element->tag == GUMBO_TAG_A) {
            GumboAttribute* href = gumbo_get_attribute(&element->attributes, "href");
            if (href && href->value) {
                links.push_back(href->value);
            }
        }

        GumboVector* children = &element->children;
        for (unsigned int i = 0; i < children->length; ++i) {
            traverseForLinks((GumboNode*)children->data[i], links);
        }
    }
}

void SiteScraper::traverseForText(GumboNode* node, std::string& text) const {
    if (!node) return;

    if (node->type == GUMBO_NODE_TEXT) {
        text += node->v.text.text;
        text += " ";
    }

    if (node->type == GUMBO_NODE_ELEMENT) {
        GumboVector* children = &node->v.element.children;
        for (unsigned int i = 0; i < children->length; ++i) {
            traverseForText((GumboNode*)children->data[i], text);
        }
    }
}

std::vector<std::string> SiteScraper::getLinks() const {
    std::vector<std::string> links;
    if (!output) return links;

    traverseForLinks(output->root, links);
    return links;
}

std::string SiteScraper::getText() const {
    std::string text;
    if (!output) return text;

    traverseForText(output->root, text);
    return text;
}