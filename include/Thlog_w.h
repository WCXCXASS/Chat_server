#ifndef THLOG_W_H_
#define THLOG_W_H_

#include <iostream>
#include <string>
#include <mutex>
#include <atomic>
#include <thread>
#include <fstream>
#include <filesystem>
#include <condition_variable>
#include <queue>

enum class Level
{
    INFO,
    WARN,
    ERROR,
    DEBUG
};

class Thlog_w
{
public:
    Thlog_w(std::string dir = "../logs/");
    ~Thlog_w();

    void log(Level level, const std::string& msg);
    std::thread::id th_id();
    void stop();

private:
    void start();
    bool reopen();
    void check_and_reopen();
    std::string level_to_string(Level level);
    std::string get_current_date() const;
    std::string get_current_time();
    void go_work();
    void write();

private:
    std::atomic<bool> shutdown = false;
    std::ofstream file_w;
    std::mutex mtx_log;
    std::thread worker;
    std::condition_variable cv_;
    std::queue<std::string> msg_qu;

    std::string current_data;
    std::string log_dir;
    std::string file_path;
};

#endif