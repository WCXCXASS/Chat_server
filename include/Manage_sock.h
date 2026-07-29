#ifndef MANAGE_SOCK_H_
#define MANAGE_SOCK_H_

#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <thread>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <filesystem>
#include <string>
#include <mutex>
#include <map>

#include "Msg_queue.h"
#include "Thlog_w.h"
#include "Message.h"
#include "Sql_table.h"

class Chat_ser;

struct File_context
{
    std::string name;
    std::ifstream ptr_r;
    std::ofstream ptr_w;
};

class Chat_cli
{
public:
    Chat_cli();
    ~Chat_cli();

    void handle_message_data(Msg_que& msg_que);
    
    void handle_file_name(std::vector<char> msg);
    void handle_file_data(std::vector<char> msg);
    void handle_file_close();

    void send_file_list();
    void send_file_data();
    void open_send_file(std::vector<char> msg);

    int get_fd();
    void set_fd(int fd);

    bool is_file_open();

    void set_name(std::string str);
    const std::string& get_name();

private:
    int client_fd = -1;
    const int head_size = 5;    // Msg_type + uint32_t
    std::vector<char> buffer;
    std::vector<char> chat_data;
    File_context recv_file_buffer;
    File_context send_file_buffer;
    std::mutex mtx_f;
    bool file_open = false;
    std::vector<std::string> files_list;
    std::string name;
};

class Chat_ser
{
public:
    Chat_ser(const char* ip, uint32_t port, int maxnums);
    ~Chat_ser();

    void handle_th_start();

    void handle_accept(epoll_event *events, int maxevents);
    static void send_error_message(int des_fd, std::string msg);
    static void send_all_message(int des_fd, Message_box send_msg);

    void handle_login(int cli_fd, std::vector<char> msg);
    void handle_register(int cli_fd, std::vector<char> msg);

    void private_message(int sou_fd, Message_box msg);
    void broadcast_message(int sou_fd, Message_box msg);

    void enable_send(int fd);
    void open_client_file(int sou_fd, std::vector<char> msg);
    void handle_client_file(int sou_fd, std::vector<char> msg);
    void client_file_close(int sou_fd);

    void set_cli_cli(int sou_fd, int des_fd);

    void remove_client(int cli_fd);
    void close_server();

    int get_server_fd();

private:
    int server_fd = -1;
    int epoll_fd = -1;
    bool is_close = false;
    int head_size = 5;
    std::mutex mtx_cs;
    Msg_que msg_que{4};
    Sql_table db;
    std::vector<char> buffer;
    std::map<int, int> des_cli;
    std::map<int, std::shared_ptr<Chat_cli>> clients;
};

#endif