#ifndef SQL_CON_QUE_H_
#define SQL_CON_QUE_H_

#include <mutex>
#include <atomic>
#include <condition_variable>
#include <queue>
#include <string>
#include <mysql/mysql.h>

#include "Logger.h"

class Sql_con
{
public:
    Sql_con(int num);
    ~Sql_con();

    MYSQL* get_sql_con();
    void rtn_sql_con(MYSQL* conn_);

    bool connect(MYSQL* conn_);

    void close_sql(MYSQL* conn_);

private:
    std::atomic<int> con_num = 4;
    std::atomic<bool> shoutdown = false;
    std::queue<MYSQL*> conns_que;
    std::mutex mtx_q;
    std::condition_variable cv_q;
};

#endif