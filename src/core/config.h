#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>

#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;

#include "core/common.h"

class Config {
public:
    Config() {}
    Config(const std::string& filename) {
        load(filename);
    }

    void load(const std::string& filename) {
        root_dir = fs::canonical(fs::absolute(fs::path(filename.c_str()).parent_path()));
        std::cout << root_dir.string() << std::endl;

        std::ifstream reader(filename.c_str(), std::ios::in);
        if (reader.fail()) {
            FatalError("Failed to load config file: %s", filename.c_str());
        }

        std::string line;
        while (std::getline(reader, line)) {
            line = strip(line);
            if (line == "" || line[0] == '#') {
                continue;
            }

            std::string name, value;
            std::stringstream ss;
            ss << line;
            ss >> name;
            ss.ignore(line.size(), '=');
            std::getline(ss, value);

            name = strip(name);
            value = strip(value);
            data[name] = value;
        }

        reader.close();

        printf("*** Parameters ***\n");
        for (const auto& it : data) {
            printf("%15s = %s\n", it.first.c_str(), it.second.c_str());
        }
        printf("******************\n\n");
    }

    std::string getString(const std::string& name) const {
        const auto it = data.find(name);
        if (it == data.end()) {
            FatalError("No parameter \"%s\" found!", name.c_str());
        }

        return it->second;
    }

    std::string getPath(const std::string& name) const {
        const auto it = data.find(name);
        if (it == data.end()) {
            FatalError("No parameter \"%s\" found!", name.c_str());
        }

        fs::path latter(it->second.c_str());
        return fs::canonical(root_dir / latter).string();
    }

    int getInt(const std::string& name) const {
        const auto it = data.find(name);
        if (it == data.end()) {
            FatalError("No parameter \"%s\" found!", name.c_str());
        }

        int ret;
        if (sscanf(it->second.c_str(), "%d", &ret) != 1) {
            FatalError("\"%s\" cannot be parsed to float!", it->second.c_str());
        }
        return ret;
    }

    float getFloat(const std::string& name) const {
        const auto it = data.find(name);
        if (it == data.end()) {
            FatalError("No parameter \"%s\" found!", name.c_str());
        }

        float ret;
        if (sscanf(it->second.c_str(), "%f", &ret) != 1) {
            FatalError("\"%s\" cannot be parsed to float!", it->second.c_str());
        }
        return ret;
    }

    glm::vec3 getVec3D(const std::string& name) const {
        const auto it = data.find(name);
        if (it == data.end()) {
            FatalError("No parameter \"%s\" found!", name.c_str());
        }

        float x, y, z;
        if (sscanf(it->second.c_str(), "%f %f %f", &x, &y, &z) != 3) {
            FatalError("\"%s\" cannot be parsed to float!", it->second.c_str());
        }
        return glm::vec3(x, y, z);
    }

    static std::string strip(const std::string& str) {
        const int beg = str.find_first_not_of(' ');
        if (beg != std::string::npos) {
            const int end = str.find_last_not_of(' ');
            return str.substr(beg, end - beg + 1);
        }
        return str;
    }

private:
    fs::path root_dir;
    std::unordered_map<std::string, std::string> data;
};