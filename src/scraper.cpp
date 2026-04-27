#include "scraper.h"
#include <iostream>
#include <future>

size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    output->append((char*)contents, size * nmemb);
    return size * nmemb;
}

std::string fetchHTML(const std::string& url) {
    CURL* curl = curl_easy_init();
    std::string html;

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &html);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0");

        curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }

    return html;
}

std::string getText(GumboNode* node) {
    if (node->type == GUMBO_NODE_TEXT)
        return node->v.text.text;

    if (node->type != GUMBO_NODE_ELEMENT) return "";

    std::string result;
    const GumboVector* children = &node->v.element.children;

    for (unsigned i = 0; i < children->length; ++i)
        result += getText((GumboNode*)children->data[i]);

    return result;
}

void parseHTML(GumboNode* node, json& result) {
    if (node->type != GUMBO_NODE_ELEMENT) return;

    GumboTag tag = node->v.element.tag;

    if (tag == GUMBO_TAG_TITLE)
        result["title"] = getText(node);

    if (tag == GUMBO_TAG_A) {
        GumboAttribute* href =
            gumbo_get_attribute(&node->v.element.attributes, "href");

        if (href) {
            result["links"].push_back(href->value);
        }
    }

    if (tag == GUMBO_TAG_P) {
        std::string text = getText(node);
        if (!text.empty()) {
            result["paragraphs"].push_back(text);
        }
    }

    const GumboVector* children = &node->v.element.children;
    for (unsigned i = 0; i < children->length; ++i) {
        parseHTML((GumboNode*)children->data[i], result);
    }
}

json htmlToJSON(const std::string& url) {
    std::string html = fetchHTML(url);

    GumboOutput* output = gumbo_parse(html.c_str());

    json result;
    result["url"] = url;
    result["links"] = json::array();
    result["paragraphs"] = json::array();

    parseHTML(output->root, result);

    gumbo_destroy_output(&kGumboDefaultOptions, output);

    return result;
}