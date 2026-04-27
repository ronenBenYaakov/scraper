#include "flow_managment.h"

class ScraperApp {
private:
    CrawlerService crawler;
    DatabaseStream db;
    WorkerPool workers;
    TaskPool<std::string> pool;

public:
    ScraperApp(const std::string& dbConn)
        : crawler("http://localhost:8888", dbConn),
          db(dbConn, 20) {}

    void run() {
        crawler.start();

        int offset = 0;

        while (true) {
            auto batch = db.fetch(offset);

            if (batch.empty()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                batch = db.fetch(offset);
                if (batch.empty()) break;
            }

            for (auto& b : batch) pool.push(b);

            auto tasks = workers.run(batch);
            workers.wait(tasks);

            offset += 20;
        }

        crawler.join();
    }
};

int main() {
    try {
        std::string dbConn =
            "host=localhost port=5432 dbname=scraper_db user=postgres password=1234";

        ScraperApp app(dbConn);
        app.run();

    } catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
    }

    return 0;
}