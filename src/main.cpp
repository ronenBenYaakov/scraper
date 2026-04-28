#include "flow_managment.h"

int main() {
    try {
        std::string dbConn =
            "host=localhost port=5432 dbname=scraper_db user=postgres password=1234";

        ScraperApp app(dbConn);
        std::atomic<bool> running{true};
        app.run(running);

    } catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
    }

    return 0;
}