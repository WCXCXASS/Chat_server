#include "Thlog_w.h"

Thlog_w::Thlog_w(std::string dir = "../logs/")
{
    log_dir = dir;
    if (!reopen())
    {
        throw std::runtime_error("Failed to open log file");
    }
    start();
}

Thlog_w::~Thlog_w()
{
    stop();
    worker.join();
}

std::string Thlog_w::level_to_string(Level level)
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

void Thlog_w::start()
{
    if (!file_w.is_open())
    {
        std::cerr << "file not open";
        return;
    }
    
    worker = std::thread(&Thlog_w::write, this);
}

std::thread::id Thlog_w::th_id()
{
    return worker.get_id();
}

void Thlog_w::log(Level level, const std::string& msg)
{
    if (shutdown)
    {
        std::cerr << "log shutdown";
        return;
    }

    std::string line = "[" + level_to_string(level) + "]" + "[" + get_current_time() + "]" + ": " + msg + "\n"; 
    {
        std::lock_guard<std::mutex> lock_p(mtx_log);
        msg_qu.push(line);
    }

    go_work();
}

void Thlog_w::stop()
{
    shutdown = true;
    cv_.notify_all();
}

bool Thlog_w::reopen()
{
    current_data = get_current_date();
    file_path = log_dir + "server_" + current_data + ".log";

    std::filesystem::path p(file_path);
    std::string dir = p.parent_path().string();
    if (!dir.empty() && !std::filesystem::exists(dir))
    {
        std::filesystem::create_directories(dir);
    }

    if (file_w.is_open()) file_w.close();
    
    file_w.open(file_path, std::ios::app);
    if (!file_w.is_open())
    {
        std::cerr << "[ERROR] Failed to open log file: " << file_path << std::endl;
        return false;
    }
    return true;
}

void Thlog_w::go_work()
{
    cv_.notify_one();
}

void Thlog_w::write()
{
    while (true)
    {
        std::unique_lock<std::mutex> lock_log(mtx_log);
        cv_.wait(lock_log, [this](){ return !msg_qu.empty() || shutdown; });

        std::queue<std::string> log;

        if (shutdown)
        {
            msg_qu.swap(log);
            lock_log.unlock();

            while (!log.empty())
            {
                file_w << log.front() << "\n";
                log.pop();
            }

            file_w.flush();
            std::cerr << "log write close";
            break;
        }

        msg_qu.swap(log);
        lock_log.unlock();

        check_and_reopen();

        while (!log.empty())
        {
            file_w << log.front() << "\n";
            log.pop();
        }
        file_w.flush();
    }
}

std::string Thlog_w::get_current_date() const
{
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
    localtime_r(&t, &tm);
    char buf[16];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d", &tm);
    return buf;
}

std::string Thlog_w::get_current_time()
{
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
    localtime_r(&t, &tm);
    char buf[16];
    std::strftime(buf, sizeof(buf), "%H:%M:%S", &tm);
    return buf;
}

void Thlog_w::check_and_reopen()
{
    std::string today = get_current_date();
    if (today != current_data)
    {
        if (!reopen())
        {
            throw std::runtime_error("Failed to open log file");
        }
    }
}