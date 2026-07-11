#ifndef _MANAGE_SOCK_H_
#define _MANAGE_SOCK_H_

#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <thread>
#include <cstdint>
#include <cstring>

#include <functional>
#include <string>
#include <map>

#include "Message.h"

class Chat_ser;

class Chat_cli
{
public:
    Chat_cli();
    ~Chat_cli();

    void handle_message_data(Chat_ser* chat_ser);
    
    void handle_file_data(std::vector<std::string>& files_name, std::vector<char> msg);
    void handle_file_name(std::vector<std::string>& files_name, std::vector<char> msg);

    int get_fd();
    void set_fd(int fd);

    void set_name(std::string str);
    const std::string& get_name();

private:
    int client_fd = -1;
    const int head_size = 5;    // Msg_type + uint32_t
    std::vector<char> buffer;
    std::vector<char> chat_data;
    std::string name;
};

class Chat_ser
{
public:
    Chat_ser(const char* ip, uint32_t port, int maxnums);
    ~Chat_ser();

    void handle_accept(epoll_event *events, int maxevents);
    void send_error_message(int des_fd, std::string msg);
    void send_all_message(int des_fd, Message_box send_msg);

    void handle_login(int cli_fd, std::vector<char> msg);
    void handle_register(int cli_fd, std::vector<char> msg);

    void private_message(int sou_fd, Message_box msg);
    void broadcast_message(int sou_fd, Message_box msg);

    void remove_client(int cli_fd);
    void close_server();

    int get_server_fd();

private:
    int server_fd = -1;
    int epoll_fd = -1;
    bool is_close = false;
    int head_size = 5;
    std::vector<char> buffer;
    std::vector<std::string> files_name;
    std::map<int, std::unique_ptr<Chat_cli>> clients;
};

#endif