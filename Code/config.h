#ifndef B216602_CONFIG_H
#define B216602_CONFIG_H

#include <string>
#include <map>
#include <variant>
#include <iostream>
#include "vector3.h"

class Config {
public:
    static Config& Instance() {
        static Config instance;
        return instance;
    }

    // Loads the JSON file. Returns false if failed.
    bool load(const std::string& filepath);

    // Getters with default fallbacks
    int getInt(const std::string& key, int defaultVal = 0) const;
    double getDouble(const std::string& key, double defaultVal = 0.0) const;
    bool getBool(const std::string& key, bool defaultVal = false) const;

private:
    Config() = default;
    std::map<std::string, std::variant<int, double, bool, std::string>> m_data;

    void parseLine(std::string line, std::string& currentSection);
};

#endif //B216602_CONFIG_H