#include "config_parser.h"

#include <fstream>
#include <stdexcept>
#include <cstdlib>

static std::string trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

static void apply_setting(SessionConfig& cfg, const std::string& key, const std::string& value) {
    if      (key == "user_code")           cfg.user_code           = value;
    else if (key == "password")            cfg.password            = value;
    else if (key == "address")             cfg.address             = value;
    else if (key == "port")                cfg.port                = std::atoi(value.c_str());
    else if (key == "board")               cfg.board               = value;
    else if (key == "sequence_number")     cfg.sequence_number     = std::atoi(value.c_str());
    else if (key == "heartbeat_interval")  cfg.heartbeat_interval  = std::atoi(value.c_str());
}

void ConfigParser::load(const std::string& path) {
    std::ifstream file(path.c_str());
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open config file: " + path);
    }

    sessions.clear();

    std::string section = "DEFAULT";
    std::string line;

    while (std::getline(file, line)) {
        line = trim(line);

        if (line.empty() || line[0] == '#' || line[0] == ';') continue;

        if (line.front() == '[' && line.back() == ']') {
            section = trim(line.substr(1, line.size() - 2));
            if (section.empty()) section = "DEFAULT";
            continue;
        }

        size_t eq_pos = line.find('=');
        if (eq_pos == std::string::npos) continue;

        std::string key   = trim(line.substr(0, eq_pos));
        std::string value = trim(line.substr(eq_pos + 1));
        if (key.empty()) continue;

        if (section == "DEFAULT") {
            apply_setting(defaults, key, value);
        } else {
            if (sessions.find(section) == sessions.end()) {
                sessions[section] = defaults;
            }
            apply_setting(sessions[section], key, value);
        }
    }
}

SessionConfig ConfigParser::get_session(const std::string& name) const {
    std::map<std::string, SessionConfig>::const_iterator it = sessions.find(name);
    if (it == sessions.end()) {
        throw std::runtime_error("Session not found in config: " + name);
    }
    return it->second;
}
