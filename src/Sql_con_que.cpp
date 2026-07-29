#include "Sql_con_que.h"

Sql_con::Sql_con(int num = 4) : con_num(num) 
{
    for (int i = 0; i < num; i++)
    {
        MYSQL* conn_ = mysql_init(nullptr);
        if (conn_ == nullptr)
        {
            logging::error("mysql init failed");

            close_sql(conn_);
            continue;
        }

        if (!connect(conn_))
        {
            close_sql(conn_);
            continue;
        }
        conns_que.push(conn_);
    }
}

Sql_con::~Sql_con()
{
    shoutdown = true;
    cv_q.notify_all();

    while (!conns_que.empty())
    {
        MYSQL* conn_ = conns_que.front();
        close_sql(conn_);
        conns_que.pop();
        logging::info("Database connection closed");
    }
}

bool Sql_con::connect(MYSQL* conn_)
{
    if (mysql_real_connect(conn_, "localhost", "root", "1234", "chat_db_2", 3306, nullptr, 0) == nullptr)
    {
        //std::cerr << "mysql_real_connect failed: " << mysql_error(conn_) << std::endl;
        logging::error("mysql_real_connect failed: " + std::string(mysql_error(conn_)));
        return false;
    }
    
    //std::cout << "Connected to MySQL successfully" << std::endl;
    logging::info("Connected to MySQL successfully");
    return true;
}

MYSQL* Sql_con::get_sql_con()
{
    std::unique_lock<std::mutex> lock_q(mtx_q);
    cv_q.wait(lock_q, [this]()
    {
        return !conns_que.empty() || shoutdown || !con_num;
    });

    if (shoutdown || !con_num)
    {
        if (shoutdown) logging::info("Sql_con shoutdown");
        if (!con_num) logging::error("con_num is 0");
        return nullptr;
    }

    MYSQL* conn_ = conns_que.front();
    conns_que.pop();
    
    if (mysql_ping(conn_) != 0)
    {
        logging::info("Database connection lost, reconnecting...");
        if (!connect(conn_))
        {
            logging::error("Reconnect failed");
            close_sql(conn_);
            return nullptr;
        }
    }
    return conn_;
}

void Sql_con::rtn_sql_con(MYSQL* conn_)
{
    if (shoutdown)
    {
        close_sql(conn_);
        return;
    }

    std::unique_lock<std::mutex> lock_q(mtx_q);
    if (conn_ == nullptr)
    {
        logging::error("conn_ is nullptr");
        return;
    }
    conns_que.push(conn_);
    cv_q.notify_one();
}

void Sql_con::close_sql(MYSQL* conn_)
{
    if (conn_ == nullptr) return;
    mysql_close(conn_);
    con_num--;
}