#ifndef SQL_H_
#define SQL_H_

#include <mysql/mysql.h>
#include <mutex>
#include <stdexcept>
#include <string>

#include "Sql_con_que.h"

class Sql_table
{
public:
    Sql_table();
    ~Sql_table();

    bool register_user(const std::string& username, const std::string& password);

    bool login_user(const std::string& username, const std::string& password);

private:
    //MYSQL* conn_ = nullptr;
    std::string host_;
    std::mutex mtx_sql;
    Sql_con conn_pool{4};
};

#endif