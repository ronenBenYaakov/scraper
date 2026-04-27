#pragma once
#include <map>
#include <vector>
#include <mutex>
#include "searxng_client.h"
#include "url_fetcher.h"

class PageBuffer {
private:
    std::map<int, std::vector<ScrapedURL>> buffer;
    std::mutex mtx;

public:
    void setPage(int page, const std::vector<ScrapedURL>& data) {
        std::lock_guard<std::mutex> lock(mtx);
        buffer[page] = data;
    }

    std::vector<ScrapedURL> getPage(int page) {
        std::lock_guard<std::mutex> lock(mtx);
        return buffer[page];
    }

    const std::map<int, std::vector<ScrapedURL>>& getAll() const {
        return buffer;
    }
};

#include <fstream>

class BatchWriter {
private:
    std::mutex fileMutex;

public:
    void writeBatch(int start, int end, PageBuffer& buffer, std::ofstream& file) {
        std::lock_guard<std::mutex> lock(fileMutex);

        file << "\n========== BATCH " << start << " - " << end << " ==========\n";

        for (int p = start; p <= end; p++) {
            file << "\n--- PAGE " << p << " ---\n";

            auto pageData = buffer.getPage(p);
            for (const auto& r : pageData) {
                file << "URL: " << r.url << "\n";
                file << "--------------------------------------\n";
            }
        }

        file.flush();
    }
};