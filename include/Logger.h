#ifndef _LOGGER_H_
#define _LOGGER_H_

#include <string>

namespace logging
{
    void init(const std::string& dir);

    void info(const std::string& msg);
    void error(const std::string& msg);
};

#endif