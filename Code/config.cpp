#include "config.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <regex>

bool Config::load(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Warning: Could not open config file '" << filepath << "'. Using defaults." << std::endl;
        return false;
    }

    std::string line;
    std::string currentSection = "global";

    while (std::getline(file, line)) {
        size_t commentPos = line.find("//");
        if (commentPos != std::string::npos) {
            line = line.substr(0, commentPos);
        }

        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);

        if (line.empty()) continue;
        if (line == "{" || line == "}") continue;

        parseLine(line, currentSection);
    }
    std::cout << "Configuration loaded from " << filepath << std::endl;
    return true;
}

void Config::parseLine(std::string line, std::string& currentSection) {
    if (line.back() == '{') {
        size_t quoteStart = line.find('\"');
        size_t quoteEnd = line.find('\"', quoteStart + 1);
        if (quoteStart != std::string::npos && quoteEnd != std::string::npos) {
            currentSection = line.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
        }
        return;
    }

    size_t colonPos = line.find(':');
    if (colonPos == std::string::npos) return;

    std::string keyPart = line.substr(0, colonPos);
    std::string valPart = line.substr(colonPos + 1);

    size_t keyStart = keyPart.find('\"');
    size_t keyEnd = keyPart.rfind('\"');
    if (keyStart == std::string::npos || keyEnd == std::string::npos) return;
    std::string key = keyPart.substr(keyStart + 1, keyEnd - keyStart - 1);

    if (valPart.back() == ',') valPart.pop_back();
    
    valPart.erase(0, valPart.find_first_not_of(" \t"));
    valPart.erase(valPart.find_last_not_of(" \t") + 1);

    std::string fullKey = currentSection + "." + key;

    // Determine type
    if (valPart == "true") {
        m_data[fullKey] = true;
    } else if (valPart == "false") {
        m_data[fullKey] = false;
    } else if (valPart.find('.') != std::string::npos) {
        try {
            m_data[fullKey] = std::stod(valPart);
        } catch (...) {}
    } else {
        try {
            m_data[fullKey] = std::stoi(valPart);
        } catch (...) {}
    }
}

int Config::getInt(const std::string& key, int defaultVal) const {
    auto it = m_data.find(key);
    if (it != m_data.end()) {
        if (std::holds_alternative<int>(it->second)) return std::get<int>(it->second);
        if (std::holds_alternative<double>(it->second)) return static_cast<int>(std::get<double>(it->second));
    }
    return defaultVal;
}

double Config::getDouble(const std::string& key, double defaultVal) const {
    auto it = m_data.find(key);
    if (it != m_data.end()) {
        if (std::holds_alternative<double>(it->second)) return std::get<double>(it->second);
        if (std::holds_alternative<int>(it->second)) return static_cast<double>(std::get<int>(it->second));
    }
    return defaultVal;
}

bool Config::getBool(const std::string& key, bool defaultVal) const {
    auto it = m_data.find(key);
    if (it != m_data.end() && std::holds_alternative<bool>(it->second)) {
        return std::get<bool>(it->second);
    }
    return defaultVal;
}