#pragma once
#include <string>
#include <gumbo.h>
#include <curl/curl.h>
#include <math.h>
#include <mutex>
#include <future>
#include <vector>
#include <iostream>
#include "bloom/bloom_filter.hpp"
#include <nlohmann/json.hpp>
#include <rocksdb/db.h>
#include <rocksdb/options.h>
#include "processing.h"

using json = nlohmann::json;

size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output);
std::string fetchHTML(const std::string& url);
std::string getText(GumboNode* node);
void parseHTML(GumboNode* node, json& result);
json htmlToJSON(const std::string& url);



class ScraperWorker {
private:
    std::unordered_map<std::string, json> cache;
    std::mutex cacheMutex;

    bloom_filter filter;

    rocksdb::DB* db;   // 👈 NOT owned anymore
    std::mutex dbMutex;

public:
    // ✅ DB is injected from outside
    explicit ScraperWorker(rocksdb::DB* externalDb)
        : db(externalDb)
    {
        bloom_parameters parameters;
        parameters.projected_element_count = 100000;
        parameters.false_positive_probability = 0.01;
        parameters.compute_optimal_parameters();

        filter = bloom_filter(parameters);

        if (!db) {
            throw std::runtime_error("ScraperWorker received null RocksDB pointer");
        }
    }

    void scrape(const std::string& url) {
        if (filter.contains(url)) {
            return;
        }

        filter.insert(url);

        try {
            std::cout << "Scraping: " << url << std::endl;

            json result = htmlToJSON(url);
            Cleaner::clean(result);

            std::string serialized = result.dump();

            {
                std::lock_guard<std::mutex> lock(cacheMutex);
                cache[url] = result;
            }

            // ✅ DB write (thread-safe wrapper)
            {
                std::lock_guard<std::mutex> lock(dbMutex);

                rocksdb::Status s = db->Put(
                    rocksdb::WriteOptions(),
                    url,
                    serialized
                );

                if (!s.ok()) {
                    std::cerr << "RocksDB write failed: "
                              << s.ToString() << std::endl;
                }
            }

            std::cout << "\n--- SCRAPED DATA ---\n";
            std::cout << result.dump(4) << std::endl;

        } catch (const std::exception& e) {
            std::cerr << "Error while scraping: " << e.what() << std::endl;
        }
    }

    std::future<void> runAsync(const std::string& url) {
        return std::async(std::launch::async,
                          &ScraperWorker::scrape,
                          this,
                          url);
    }

    bool getCached(const std::string& url, json& out) {
        {
            std::lock_guard<std::mutex> lock(cacheMutex);
            auto it = cache.find(url);
            if (it != cache.end()) {
                out = it->second;
                return true;
            }
        }

        std::string value;
        {
            std::lock_guard<std::mutex> lock(dbMutex);

            rocksdb::Status s = db->Get(
                rocksdb::ReadOptions(),
                url,
                &value
            );

            if (!s.ok()) {
                return false;
            }
        }

        out = json::parse(value);

        std::lock_guard<std::mutex> lock(cacheMutex);
        cache[url] = out;

        return true;
    }

    bool hasCached(const std::string& url) {
        std::lock_guard<std::mutex> lock(cacheMutex);
        return cache.find(url) != cache.end();
    }
};