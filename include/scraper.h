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
    std::unordered_map<std::string, nlohmann::json> cache;
    std::mutex cacheMutex;

    bloom_filter filter;

    std::unique_ptr<rocksdb::DB> db;
    rocksdb::Options options;

public:
    ScraperWorker() {
        bloom_parameters parameters;
        parameters.projected_element_count = 100000;
        parameters.false_positive_probability = 0.01;
        parameters.compute_optimal_parameters();
        filter = bloom_filter(parameters);

        options.create_if_missing = true;
        options.compression = rocksdb::kSnappyCompression;

        rocksdb::DB* rawDb;
        rocksdb::Status status = rocksdb::DB::Open(options, "./scraper_db", &rawDb);

        if (!status.ok()) {
            throw std::runtime_error("Failed to open RocksDB: " + status.ToString());
        }

        db.reset(rawDb);
    }

    void scrape(const std::string& url) {
        if (filter.contains(url)) {
            return;
        }

        filter.insert(url);

        try {
            std::cout << "Scraping: " << url << std::endl;

            nlohmann::json result = htmlToJSON(url);
            Cleaner::clean(result);

            std::string serialized = result.dump();

            {
                std::lock_guard<std::mutex> lock(cacheMutex);
                cache[url] = result;

                rocksdb::Status s = db->Put(
                    rocksdb::WriteOptions(),
                    url,
                    serialized
                );

                if (!s.ok()) {
                    std::cerr << "RocksDB write failed: " << s.ToString() << std::endl;
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

    bool getCached(const std::string& url, nlohmann::json& out) {
        {
            std::lock_guard<std::mutex> lock(cacheMutex);
            auto it = cache.find(url);
            if (it != cache.end()) {
                out = it->second;
                return true;
            }
        }

        std::string value;
        rocksdb::Status s = db->Get(rocksdb::ReadOptions(), url, &value);

        if (!s.ok()) {
            return false;
        }

        out = nlohmann::json::parse(value);

        std::lock_guard<std::mutex> lock(cacheMutex);
        cache[url] = out;

        return true;
    }

    bool hasCached(const std::string& url) {
        std::lock_guard<std::mutex> lock(cacheMutex);
        return cache.find(url) != cache.end();
    }
};

using json = nlohmann::json;

class ScraperObserver {
public:
    struct Metrics {
        size_t fieldCount = 0;
        size_t totalStringLength = 0;
        double entropy = 0.0;
        double infoGain = 0.0;
    };

    Metrics analyze(const json& data) {
        Metrics m;

        if (data.is_null()) return m;

        computeMetrics(data, m);

        m.infoGain = computeInfoGain(m);

        return m;
    }

    void printMetrics(const Metrics& m) {
        std::cout << "\n--- SCRAPER METRICS ---\n";
        std::cout << "Fields: " << m.fieldCount << "\n";
        std::cout << "Total text size: " << m.totalStringLength << "\n";
        std::cout << "Entropy: " << m.entropy << "\n";
        std::cout << "Info Gain (heuristic): " << m.infoGain << "\n";
    }

private:

    void computeMetrics(const json& j, Metrics& m) {
        if (j.is_object()) {
            for (auto& [key, value] : j.items()) {
                m.fieldCount++;

                computeMetrics(value, m);
            }
        }
        else if (j.is_array()) {
            for (auto& v : j) {
                computeMetrics(v, m);
            }
        }
        else if (j.is_string()) {
            std::string s = j.get<std::string>();
            m.totalStringLength += s.size();

            m.entropy += shannonLikeScore(s);
        }
        else {
            m.fieldCount++;
        }
    }

    double shannonLikeScore(const std::string& s) {
        if (s.empty()) return 0.0;

        std::unordered_map<char, int> freq;

        for (char c : s) {
            freq[c]++;
        }

        double entropy = 0.0;
        double len = static_cast<double>(s.size());

        for (auto& [c, f] : freq) {
            double p = f / len;
            entropy -= p * std::log2(p);
        }

        return entropy;
    }

    double computeInfoGain(const Metrics& m) {
        double base = 1.0;
        return (m.fieldCount * 0.5 + m.entropy + std::log1p(m.totalStringLength)) / base;
    }
};