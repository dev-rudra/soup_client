#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H

#include <string>
#include <map>

struct SessionConfig {
    std::string user_code;
    std::string password;
    std::string address;
    int         port;
    std::string board;
    int         sequence_number;
    int         heartbeat_interval;

    SessionConfig()
        : port(0)
        , sequence_number(0)
        , heartbeat_interval(1)
    {}
};

class ConfigParser {
public:
    void load(const std::string& path);

    SessionConfig get_session(const std::string& name) const;

private:
    SessionConfig                        defaults;
    std::map<std::string, SessionConfig> sessions;
};

#endif
