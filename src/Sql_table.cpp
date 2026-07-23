#include "Sql_table.h"
#include "Logger.h"

Sql_table::Sql_table()
{
    conn_ = mysql_init(nullptr);
    if (conn_ == nullptr)
    {
        perror("mysql init failed");
    }
}

Sql_table::~Sql_table()
{
    if (conn_ != nullptr)
    {
        mysql_close(conn_);
        conn_ = nullptr;
        std::cout << "Database connection closed\n";
    }   
}

bool Sql_table::connect()
{
    if (mysql_real_connect(conn_, "localhost", "root", "1234", "chat_db_2", 3306, nullptr, 0) == nullptr)
    {
        std::cerr << "mysql_real_connect failed: " << mysql_error(conn_) << std::endl;
        return false;
    }
    
    std::cout << "Connected to MySQL successfully" << std::endl;
    return true;
}

bool Sql_table::register_user(const std::string& username, const std::string& password)
{
    if (conn_ == nullptr)
    {
        logging::error("register_user: conn_ is nullptr");
        return false;
    }

    std::vector<char> escaped_username(username.size() * 2 + 1);
    std::vector<char> escaped_password(password.size() * 2 + 1);

    unsigned long username_len = mysql_real_escape_string(
        conn_,
        escaped_username.data(),
        username.c_str(),
        username.size()
    );

    unsigned long password_len = mysql_real_escape_string(
        conn_,
        escaped_password.data(),
        password.c_str(),
        password.size()
    );

    std::string sql = "INSERT INTO users (username, password_hash) VALUES ('"
                    + std::string(escaped_username.data(), username_len) + "', '"
                    + std::string(escaped_password.data(), password_len) + "')";

    if (mysql_query(conn_, sql.c_str()) != 0)
    {
        int err = mysql_errno(conn_);
        if (err == 1062)
        {
            logging::error("Register failed: username '" + username + "' already exists");
        } 
        else 
        {
            logging::error("Register failed: " + std::string(mysql_error(conn_)) 
                           + " (errno: " + std::to_string(err) + ")");
        }
        return false;
    }

    my_ulonglong affected = mysql_affected_rows(conn_);
    if (affected == 0)
    {
        logging::error("Register failed: affected rows = 0, username: " + username);
        return false;
    }

    logging::info("User registered successfully: " + username);
    return true;
}

bool Sql_table::login_user(const std::string& username, const std::string& password)
{
    if (conn_ == nullptr)
    {
        logging::error("login_user: conn_ is nullptr");
        return false;
    }

    std::vector<char> escaped_username(username.size() * 2 + 1);
    unsigned long username_len = mysql_real_escape_string(
        conn_,
        escaped_username.data(),
        username.c_str(),
        username.size()
    );

    std::string sql = "SELECT password_hash FROM users WHERE username = '"
                    + std::string(escaped_username.data(), username_len) + "'";

    if (mysql_query(conn_, sql.c_str()) != 0)
    {
        logging::error("Login query failed: " + std::string(mysql_error(conn_)));
        return false;
    }

    MYSQL_RES* res = mysql_store_result(conn_);
    if (res == nullptr)
    {
        logging::error("mysql_store_result failed: " + std::string(mysql_error(conn_)));
        return false;
    }

    MYSQL_ROW row = mysql_fetch_row(res);
    if (row == nullptr)
    {
        mysql_free_result(res);
        return false;
    }

    std::string db_password = row[0] ? row[0] : "";

    mysql_free_result(res);

    if (db_password == password)
    {
        return true;
    }
    else
    {
        return false;
    }
}