#pragma once

#include <vector>
#include <future>
#include <algorithm>
#include <iostream>
#include <string>
#include <memory>

#include <pqxx/pqxx>

#include "fetchers.h"
#include "url_fetcher.h"

class SearXNGClient {
public:
    explicit SearXNGClient(std::string baseUrl);
    std::vector<ScrapedURL> search(const std::string& query);

private:
    std::string baseUrl;
    std::string httpGet(const std::string& url);
};

class URLFetcherWorker {
private:
    SearXNGClient& client;
    PageBuffer& buffer;

public:
    URLFetcherWorker(SearXNGClient& c, PageBuffer& b)
        : client(c), buffer(b) {}

    void fetchPage(const std::string& query, int page) {
        try {
            std::string pagedQuery =
                query + "&pageno=" + std::to_string(page);

            auto urls = client.search(pagedQuery);

            buffer.setPage(page, urls);

            std::cout << "Page " << page
                      << " fetched (" << urls.size() << " results)\n";

        } catch (const std::exception& e) {
            std::cerr << "Page " << page
                      << " error: " << e.what() << "\n";
        }
    }

    std::future<void> runAsync(const std::string& query, int page) {
        return std::async(
            std::launch::async,
            &URLFetcherWorker::fetchPage,
            this,
            query,
            page
        );
    }
};

class ScraperController {
private:
    SearXNGClient client;
    PageBuffer buffer;
    URLFetcherWorker worker;

    pqxx::connection& db; 

    void initDB() {
        pqxx::work txn(db);

        txn.exec(R"(
            CREATE TABLE IF NOT EXISTS scraped_results (
                id SERIAL PRIMARY KEY,
                page INT NOT NULL,
                content TEXT NOT NULL
            );
        )");

        txn.commit();
    }

public:
    ScraperController(const std::string& baseUrl,
                      pqxx::connection& dbConn)
        : client(baseUrl),
          worker(client, buffer),
          db(dbConn)
    {
        initDB();
    }

    void saveBatchToDB(int start, int end) {
        pqxx::work txn(db);

        for (int i = start; i <= end; i++) {
            auto results = buffer.getPage(i);

            for (const auto& r : results) {
                txn.exec_params(
                    "INSERT INTO scraped_results(page, content) VALUES ($1, $2)",
                    i,
                    r.url
                );
            }
        }

        txn.commit();
    }

    void run(const std::string& query,
             int startPage,
             int numPages,
             int batchSize)
    {
        std::vector<std::future<void>> tasks;

        std::cout << "Starting concurrent scraping...\n";

        for (int i = startPage; i < startPage + numPages; i++) {
            tasks.push_back(worker.runAsync(query, i));
        }

        for (auto& t : tasks) {
            t.get();
        }

        for (int i = startPage;
             i < startPage + numPages;
             i += batchSize)
        {
            int end = std::min(i + batchSize - 1,
                               startPage + numPages - 1);

            saveBatchToDB(i, end);

            std::cout << "Saved batch "
                      << i << "-" << end
                      << " to DB\n";
        }

        std::cout << "\nDone. Results saved to PostgreSQL.\n";
    }
};