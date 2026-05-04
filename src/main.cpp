#include <iostream>
#include <string>
#include "Agent.h"

/*int main() {
    // llama-server endpoint
    Agent agent("http://127.0.0.1:8080/v1/chat/completions");

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

#include <iostream>
#include "LinkFetcher.h"

int main() {
    LinkFetcher client("http://localhost:8888");

    auto results = client.search("C++ recursion tutorial");

    for (const auto& r : results) {
        std::cout << "Title: " << r.title << "\n";
        std::cout << "URL: " << r.url << "\n";
        std::cout << "Snippet: " << r.snippet << "\n\n";
    }

    return 0;
}