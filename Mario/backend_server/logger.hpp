#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <string>

class Logger {
public:
    void info(const std::string& msg);
    void error(const std::string& msg);
};

#endif

