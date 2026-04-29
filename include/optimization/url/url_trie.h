#pragma once

#include <unordered_map>
#include <memory>
#include <string_view>
#include <vector>
#include "url_list.h"

struct TrieNode {
    std::unordered_map<std::string_view, std::unique_ptr<TrieNode>> children;
    bool is_end = false;
};

class UrlTrie {
private:
    std::unique_ptr<TrieNode> root;

public:
    UrlTrie() : root(std::make_unique<TrieNode>()) {}
    
    void addUrl(const std::string& url) {
        UrlList list(url);
        const auto& parts = list.getParts();
        TrieNode* current = root.get();

        for(const auto& part : parts) {
            std::string key(part);

            auto& next = current->children[key];
            if(!next)
                next = std::make_unique<TrieNode>();

            current = next.get();
        }

        current->is_end = true;
    }
};