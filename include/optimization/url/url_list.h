#include <string>
#include <vector>
#include <string_view>

#include <string>
#include <vector>
#include <string_view>

class UrlList {
private:
    std::string url;
    std::vector<std::string_view> parts;

public:
    explicit UrlList(std::string input) : url(std::move(input)) {parse();}

    void parse() {
        parts.clear();

        std::string_view view(url);

        size_t start = 0;
        for (size_t i = 0; i <= view.size(); ++i) {
            if (i == view.size() || view[i] == '/') {
                if (i > start) {
                    parts.emplace_back(view.substr(start, i - start));
                }
                start = i + 1;
            }
        }

        if (!parts.empty() &&
            (parts[0] == "http:" || parts[0] == "https:")) {
            parts.erase(parts.begin()); // O(k)
        }

        if (!parts.empty() &&
            parts[0].rfind("www.", 0) == 0) {
            parts.erase(parts.begin()); // O(k)
        }
    }

    const std::vector<std::string_view>& getParts() const {
        return parts;
    }
};