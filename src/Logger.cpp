#include "Logger.h"
#include "Thlog_w.h"

namespace logging
{
    static Thlog_w& getLogger()
    {
        static Thlog_w logger("./logs/");
        return logger;
    }

    void info(const std::string& msg)
    {
        getLogger().log(Level::INFO, msg);
    }

    void error(const std::string& msg)
    {
        getLogger().log(Level::ERROR, msg);
    }
};