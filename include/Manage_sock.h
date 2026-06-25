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

class Chat_cli
{
public:
    Chat_cli();
    ~Chat_cli();

    int get_client_fd();

private:
    int client_fd;
};

class Chat_ser
{
public:
    Chat_ser(const char* ip, uint32_t port);
    ~Chat_ser();

    void handle_accept(epoll_event *events, int maxevents);
    void broadcast_message(std::vector<char>& chat_data);
    void private_message(std::vector<char>& chat_data);
    void close_server();

    int get_server_fd();

private:
    int server_fd = -1;
    int epoll_fd = -1;
    bool is_close = false;
    std::vector<std::string> files_name;
    Message msg_handle;
    std::map<std::string, int> clients;
};

#endif