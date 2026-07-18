#include "Logger.h"

#include <iostream>
#include <mutex>
#include <chrono>
#include <ctime>
#include <fstream>
#include <filesystem>

namespace
{
    enum class Level
        {
            INFO,
            WARN,
            ERROR,
            DEBUG
        };

    std::string level_to_string(Level level)
    {
        switch (level)
        {
            case Level::INFO:  return "INFO";
            case Level::WARN:  return "WARN";
            case Level::ERROR: return "ERROR";
            case Level::DEBUG: return "DEBUG";
            default:           return "UNKNOWN";
        }
    }

    class Logger
    {
    public:
        static Logger& instance()
        {
            static Logger inst;
            return inst;
        }

        void init(const std::string& dir)
        {
            log_dir = dir;

            if (!std::filesystem::exists(log_dir))
            {
                std::filesystem::create_directories(log_dir);
            }

            reopen();
        }

        void log(Level level, const std::string& msg)
        {
            std::lock_guard<std::mutex> lock(mtx);
            check_and_reopen();

            std::string line = "[" + get_current_time() + "]"
                                + level_to_string(level) + " " 
                                + msg + "\n";
            if (file.is_open())
            {
                file << line;
                file.flush();
            }
            else
            {
                std::cerr << line;
            }

        }

    private:
        Logger() = default;
        ~Logger() { if (file.is_open()) file.flush(); }

        Logger(const Logger&) = delete;
        Logger& operator=(const Logger&) = delete;

        void reopen()
        {
            if (file.is_open()) file.close();
            current_data = get_current_date();
            std::string filename = log_dir + "server_" + current_data + ".log";
            file.open(filename, std::ios::app);
            if (!file.is_open())
            {
                std::cerr << "[ERROR] Failed to open log file: " << filename << std::endl;
            }
        }

        void check_and_reopen()
        {
            std::string today = get_current_date();

            if (today != current_data)
            {
                reopen();
            }
        }

        std::string get_current_date() const
        {
            auto now = std::chrono::system_clock::now();
            std::time_t t = std::chrono::system_clock::to_time_t(now);
            std::tm tm;
            localtime_r(&t, &tm);
            char buf[16];
            std::strftime(buf, sizeof(buf), "%Y-%m-%d", &tm);
            return buf;
        }

        std::string get_current_time()
        {
            auto now = std::chrono::system_clock::now();
            std::time_t t = std::chrono::system_clock::to_time_t(now);
            std::tm tm;
            localtime_r(&t, &tm);
            char buf[16];
            std::strftime(buf, sizeof(buf), "%H:%M:%S", &tm);
            return buf;
        }

        std::ofstream file;
        std::mutex mtx;
        std::string current_data;
        std::string log_dir;
    };
}

namespace loggin
{
    void init(const std::string& dir = "../logs/")
    {
        Logger::instance().init(dir);
    }

    void info(const std::string& msg)
    {
        Logger::instance().log(Level::INFO, msg);
    }

    void error(const std::string& msg)
    {
        Logger::instance().log(Level::ERROR, msg);
    }
}