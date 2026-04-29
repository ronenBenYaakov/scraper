#pragma once

#include <mutex>
#include <condition_variable>
#include <vector>
#include <deque>
#include <future>
#include <string>
#include <thread>
#include <atomic>
#include <optional>
#include <iostream>
#include "scraper.h"
#include <pqxx/pqxx>
#include <rocksdb/db.h>
#include "searxng_client.h"
// =========================
// Blocking Task Queue
// =========================
template <typename T>
class TaskPool {
private:
    std::deque<T> data;
    std::mutex mtx;
    std::condition_variable cv;

public:
    void push(T value) {
        {
            std::lock_guard<std::mutex> lock(mtx);
            data.push_back(std::move(value));
        }
        cv.notify_one();
    }

    T popBlocking() {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [&] { return !data.empty(); });

        T v = std::move(data.front());
        data.pop_front();
        return v;
    }

    bool empty() {
        std::lock_guard<std::mutex> lock(mtx);
        return data.empty();
    }
};

// =========================
// Crawler Service
// =========================
class CrawlerService {
private:
    ScraperController scraper;
    std::thread th;

public:
    CrawlerService(const std::string& url,
                   pqxx::connection& pgDb)
        : scraper(url, pgDb) {}

    void start() {
        th = std::thread([this]() {
            scraper.run("what are the benefits of ai?", 1, 100, 5);
        });
    }

    void join() {
        if (th.joinable()) th.join();
    }
};

// =========================
// Postgres Stream
// =========================
class DatabaseStream {
private:
    pqxx::connection conn;
    int batchSize;

public:
    DatabaseStream(const std::string& db, int size)
        : conn(db), batchSize(size) {}

    std::vector<std::string> fetch(int offset) {
        pqxx::work txn(conn);

        pqxx::result res = txn.exec_params(
            "SELECT content FROM scraped_results "
            "ORDER BY id LIMIT $1 OFFSET $2",
            batchSize,
            offset
        );

        txn.commit();

        std::vector<std::string> out;
        for (auto const& r : res) {
            out.push_back(r["content"].as<std::string>());
        }
        return out;
    }
};

// =========================
// Worker Pool (shared worker)
// =========================
class WorkerPool {
private:
    ScraperWorker& worker;

public:
    WorkerPool(ScraperWorker& w)
        : worker(w) {}

    std::vector<std::future<void>> run(const std::vector<std::string>& items) {
        std::vector<std::future<void>> tasks;

        for (auto& u : items) {
            tasks.push_back(worker.runAsync(u));
        }

        return tasks;
    }

    void wait(std::vector<std::future<void>>& tasks) {
        for (auto& t : tasks) {
            t.get();
        }
    }
};

// =========================
// Scraper Application
// =========================
class ScraperApp {
private:
    CrawlerService crawler;
    DatabaseStream db;
    WorkerPool workers;
    TaskPool<std::string> pool;

    std::atomic<int> processed{0};
    std::atomic<int> batches{0};

public:
    // 🔥 EVERYTHING uses shared DB externally
    ScraperApp(const std::string& dbConn,
            pqxx::connection& pgDb,
            ScraperWorker& worker)
        : crawler("http://localhost:8888", pgDb),
        db(dbConn, 20),
        workers(worker) {}

    void run(std::atomic<bool>& runningFlag) {
        crawler.start();

        int offset = 0;

        while (runningFlag.load()) {
            auto batch = db.fetch(offset);

            if (batch.empty()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(200));

                batch = db.fetch(offset);
                if (batch.empty()) break;
            }

            // push into queue (optional pipeline usage)
            for (auto& b : batch) {
                pool.push(b);
            }

            auto tasks = workers.run(batch);
            workers.wait(tasks);

            processed += batch.size();
            batches++;

            offset += 20;
        }

        crawler.join();
    }

    int getProcessed() const { return processed.load(); }
    int getBatches() const { return batches.load(); }
};