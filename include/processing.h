#pragma once
#include <nlohmann/json.hpp>
#include <string>
#include <algorithm>
#include <regex>
#include <cctype>

class Cleaner {
private:
    static std::string stripHtml(const std::string& input) {
        static const std::regex tags("<[^>]*>");
        return std::regex_replace(input, tags, "");
    }

    static std::string trim(const std::string& s) {
        auto start = std::find_if_not(s.begin(), s.end(), ::isspace);
        auto end = std::find_if_not(s.rbegin(), s.rend(), ::isspace).base();

        if (start >= end) return "";
        return std::string(start, end);
    }

    static std::string normalizeSpaces(const std::string& s) {
        std::string out;
        bool inSpace = false;

        for (char c : s) {
            if (std::isspace(static_cast<unsigned char>(c))) {
                if (!inSpace) {
                    out.push_back(' ');
                    inSpace = true;
                }
            } else {
                out.push_back(c);
                inSpace = false;
            }
        }
        return out;
    }

    static std::string cleanString(std::string s) {
        s = stripHtml(s);
        s = normalizeSpaces(s);
        s = trim(s);
        return s;
    }

    static void cleanJson(nlohmann::json& j) {
        if (j.is_object()) {
            for (auto& [key, value] : j.items()) {
                cleanJson(value);
            }
        }
        else if (j.is_array()) {
            for (auto& item : j) {
                cleanJson(item);
            }
        }
        else if (j.is_string()) {
            std::string s = j.get<std::string>();
            j = cleanString(s);
        }
    }

public:
    static void clean(nlohmann::json& data) {
        cleanJson(data);
    }
};