#ifndef _LOGGER_H_
#define _LOGGER_H_

#include <Thlog_w.h>
#include <string>

namespace logging
{
    static Thlog_w& getLogger();

    void info(const std::string& msg);
    void error(const std::string& msg);
};

#endif