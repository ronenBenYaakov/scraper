#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>

#include "flow_managment.h"

using namespace std;
using namespace std::chrono;

static const int STALL_THRESHOLD_MS = 3000;   // no progress = suspicious
static const int SAMPLE_INTERVAL_MS = 500;

void run_stress_monitor() {
    std::cout << "=== SCRAPER APP STRESS + DEADLOCK MONITOR ===\n";

    const std::string dbConn =
        "host=localhost port=5432 dbname=scraper_db user=postgres password=1234";

    ScraperApp app(dbConn);

    std::atomic<bool> running{true};

    // -----------------------------
    // RUN APP IN BACKGROUND THREAD
    // -----------------------------
    std::thread worker([&]() {
        app.run(running);
        running = false;
    });

    auto start = high_resolution_clock::now();

    int lastProcessed = 0;
    auto lastProgressTime = start;

    bool warnedStall = false;

    // -----------------------------
    // LIVE MONITOR LOOP
    // -----------------------------
    while (running.load()) {
        std::this_thread::sleep_for(milliseconds(SAMPLE_INTERVAL_MS));

        auto now = high_resolution_clock::now();
        auto ms = duration_cast<milliseconds>(now - start).count();

        int processed = app.getProcessed();
        int batches = app.getBatches();

        int delta = processed - lastProcessed;
        double instantRate = delta / (SAMPLE_INTERVAL_MS / 1000.0);
        double avgRate = processed / (ms / 1000.0);

        // ----------------------------------------
        // DEADLOCK / STALL DETECTION LOGIC
        // ----------------------------------------
        if (delta > 0) {
            lastProgressTime = now;
            warnedStall = false;
        } else {
            auto stallTime = duration_cast<milliseconds>(now - lastProgressTime).count();

            if (stallTime > STALL_THRESHOLD_MS && !warnedStall) {
                std::cout << "\n⚠️ WARNING: Possible deadlock or stall detected!\n";
                std::cout << "No progress for " << stallTime << " ms\n";
                std::cout << "Processed: " << processed << "\n";
                warnedStall = true;
            }
        }

        // ----------------------------------------
        // LIVE DISPLAY
        // ----------------------------------------
        std::cout << "\r"
                  << "Time: " << ms << " ms | "
                  << "Processed: " << processed << " | "
                  << "Batches: " << batches << " | "
                  << "Instant: " << instantRate << " items/s | "
                  << "Avg: " << avgRate << " items/s"
                  << "        " // clear leftovers
                  << std::flush;

        lastProcessed = processed;
    }

    worker.join();

    // -----------------------------
    // FINAL REPORT
    // -----------------------------
    auto end = high_resolution_clock::now();
    auto totalMs = duration_cast<milliseconds>(end - start).count();

    std::cout << "\n\n=== FINAL REPORT ===\n";
    std::cout << "Total time: " << totalMs << " ms\n";
    std::cout << "Total processed: " << app.getProcessed() << "\n";
    std::cout << "Total batches: " << app.getBatches() << "\n";

    double finalRate = app.getProcessed() / (totalMs / 1000.0);
    std::cout << "Avg throughput: " << finalRate << " items/sec\n";

    std::cout << "=====================\n";
}

int main() {
    run_stress_monitor();
    return 0;
}