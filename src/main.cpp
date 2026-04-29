#include <iostream>
#include <atomic>

#include <rocksdb/db.h>
#include <rocksdb/options.h>

#include <pqxx/pqxx>

#include "flow_managment.h"
#include "scraper.h"

int main() {
    try {
        // =========================
        // 1. PostgreSQL connection
        // =========================
        std::string pgConnStr =
            "host=localhost port=5432 dbname=scraper_db user=postgres password=1234";

        pqxx::connection pgConn(pgConnStr);

        // =========================
        // 2. RocksDB init (shared)
        // =========================
        rocksdb::DB* rocksDb = nullptr;

        rocksdb::Options options;
        options.create_if_missing = true;

        rocksdb::Status status =
            rocksdb::DB::Open(options, "./scraper_db", &rocksDb);

        if (!status.ok() || !rocksDb) {
            std::cerr << "Failed to open RocksDB: "
                      << status.ToString() << "\n";
            return 1;
        }

        // =========================
        // 3. Scraper worker (RocksDB)
        // =========================
        ScraperWorker worker(rocksDb);

        // =========================
        // 4. App
        // =========================
        ScraperApp app(pgConnStr, pgConn, worker);

        std::atomic<bool> running{true};

        // =========================
        // 5. Run system
        // =========================
        app.run(running);

        // =========================
        // 6. Cleanup
        // =========================
        delete rocksDb;

        std::cout << "Shutdown cleanly.\n";

    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}