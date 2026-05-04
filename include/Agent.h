#pragma once
#include <string>

class Agent {
private:
    std::string url;
    std::string postRequest(const std::string& jsonBody);

public:
    Agent(const std::string& serverUrl);
    std::string chat(const std::string& userMessage);
};