#include <iostream>
#include <string>
#include "Agent.h"

/*int main() {
    // llama-server endpoint
    Agent agent("http://127.0.0.1:8081/v1/chat/completions");

    std::cout << "🤖 Local LLM Agent ready (type 'exit' to quit)\n";

    std::string input;

    while (true) {
        std::cout << "\nYou: ";
        std::getline(std::cin, input);

        if (input == "exit") break;

        std::string response = agent.chat(input);

        std::cout << "Agent: " << response << "\n";
    }

    return 0;
}*/

/*
#include <iostream>
#include "LinkFetcher.h"

int main() {
    LinkFetcher client("http://localhost:8080");

    auto results = client.search("C++ recursion tutorial");

    for (const auto& r : results) {
        std::cout << "Title: " << r.title << "\n";
        std::cout << "URL: " << r.url << "\n";
        std::cout << "Snippet: " << r.snippet << "\n\n";
    }

    return 0;
}*/

#include "SiteScraper.h"
#include <iostream>

int main() {
    SiteScraper scraper;

    std::string url = "https://danzig.us/cpp/recursion.html";

    std::cout << "Fetching: " << url << "\n";

    if (!scraper.fetch(url)) {
        std::cerr << "Failed to fetch page.\n";
        return 1;
    }

    auto links = scraper.getLinks();

    std::cout << "\n=== LINKS FOUND ===\n";
    for (const auto& link : links) {
        std::cout << link << "\n";
    }

    
    std::cout << "\n=== TEXT (preview) ===\n";

    std::string text = scraper.getText();

    if (text.size() > 1500) {
        std::cout << text.substr(0, 1500) << "\n... (truncated)\n";
    } else {
        std::cout << text << "\n";
    }

    return 0;
}