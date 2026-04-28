#pragma once
#include <mutex>
#include <condition_variable>
#include "searxng_client.h"
#include "scraper.h"
#include <vector>
#include <deque>
#include <future>
#include <string>
#include <optional>
#include <thread>

template <typename T>
class TaskPool {
private:
    std::deque<T> data;
    std::mutex mtx;

public:
    void push(T value) {
        std::lock_guard<std::mutex> lock(mtx);
        data.push_back(std::move(value));
    }

    std::optional<T> pop() {
        std::lock_guard<std::mutex> lock(mtx);
        if (data.empty()) return std::nullopt;
        T v = std::move(data.front());
        data.pop_front();
        return v;
    }

    bool empty() {
        std::lock_guard<std::mutex> lock(mtx);
        return data.empty();
    }
};

class CrawlerService {
private:
    ScraperController scraper;
    std::thread th;

public:
    CrawlerService(const std::string& url, const std::string& db)
        : scraper(url, db) {}

    void start() {
        th = std::thread([this]() {
            scraper.run("what are the benefits of ai?", 1, 100, 5);
        });
    }

    void join() {
        if (th.joinable()) th.join();
    }
};

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
            "SELECT content FROM scraped_results ORDER BY id LIMIT $1 OFFSET $2",
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

class WorkerPool {
private:
    ScraperWorker worker;

public:
    std::vector<std::future<void>> run(const std::vector<std::string>& items) {
        std::vector<std::future<void>> tasks;
        for (auto& u : items) {
            tasks.push_back(worker.runAsync(u));
        }
        return tasks;
    }

    void wait(std::vector<std::future<void>>& tasks) {
        for (auto& t : tasks) t.get();
    }
};

#pragma once
#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

class ScraperApp {
private:
    CrawlerService crawler;
    DatabaseStream db;
    WorkerPool workers;
    TaskPool<std::string> pool;

    std::atomic<int> processed{0};
    std::atomic<int> batches{0};

public:
    ScraperApp(const std::string& dbConn)
        : crawler("http://localhost:8888", dbConn),
          db(dbConn, 20) {}

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