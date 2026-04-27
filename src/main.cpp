#include "searxng_client.h"
#include "scraper.h"
#include <pqxx/pqxx>
#include <iostream>
#include <vector>
#include <thread>

int main() {
    try {
        std::string dbConn =
            "host=localhost port=5432 dbname=scraper_db user=postgres password=1234";

        ScraperController scraper("http://localhost:8888", dbConn);

        // -------------------------------
        // 1. RUN CRAWLER AS BACKGROUND TASK
        // -------------------------------
        std::thread crawlerThread([&scraper]() {
            scraper.run("what are the benefits of ai?", 1, 100, 5);
        });

        // -------------------------------
        // 2. START SCRAPING PIPELINE IMMEDIATELY
        // -------------------------------
        pqxx::connection db(dbConn);

        const int batchSize = 20;
        int offset = 0;

        ScraperWorker worker;

        std::cout << "Streaming batch scraping starting...\n";

        while (true) {
            pqxx::work txn(db);

            pqxx::result res = txn.exec_params(
                "SELECT content FROM scraped_results ORDER BY id LIMIT $1 OFFSET $2",
                batchSize,
                offset
            );

            txn.commit();

            if (res.empty()) {
                // small wait in case crawler is still filling DB
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                
                // re-check once more
                pqxx::work retryTxn(db);
                pqxx::result retry = retryTxn.exec_params(
                    "SELECT content FROM scraped_results ORDER BY id LIMIT $1 OFFSET $2",
                    batchSize,
                    offset
                );
                retryTxn.commit();

                if (retry.empty()) {
                    break;
                }

                continue;
            }

            std::vector<std::future<void>> tasks;
            tasks.reserve(res.size());

            std::cout << "Processing batch at offset " << offset
                      << " (" << res.size() << " items)\n";

            for (const auto& row : res) {
                std::string url = row["content"].as<std::string>();
                tasks.push_back(worker.runAsync(url));
            }

            for (auto& t : tasks) {
                t.get();
            }

            offset += batchSize;
        }

        crawlerThread.join();

        std::cout << "Done.\n";

    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << "\n";
    }

    return 0;
}