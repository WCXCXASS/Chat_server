#include "Manage_sock.h"

Chat_ser::Chat_ser(const char *ip, uint32_t port)
{
    int res;
    sockaddr_in ser_addr;
    ser_addr.sin_family = AF_INET;
    ser_addr.sin_port = htons(8888);
    ser_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1)
    {
        perror("socket failed: ");
    }

    res = bind(server_fd, (sockaddr*)&ser_addr, sizeof(ser_addr));
    if (res == -1)
    {
        perror("bind failed: ");
    }

    res = listen(server_fd, 100);
    if (res == -1)
    {
        perror("listen failed: ");
    }

    epoll_fd = epoll_create(1);
    if (epoll_fd == -1)
    {
        perror("epoll_create failed: ");
    }

    epoll_event server_ev;
    server_ev.events = EPOLLIN;
    server_ev.data.fd = server_fd;
    res = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &server_ev);
    if (res == -1)
    {
        perror("epoll_ctl failed: ");
    }
}

void Chat_ser::handle_accept(epoll_event *events, int maxevents)///////// wait change
{
    int res;
    
    int events_n = epoll_wait(epoll_fd, events, maxevents, 0);
    if (events_n == -1)
    {
        perror("epoll_wait: ");
    }

    for (int i = 0; i < events_n; i++)
    {
        int fd = events[i].data.fd;
        if (fd == server_fd)
        {
            sockaddr_in cli_addr;
            socklen_t cli_size = sizeof(cli_addr);
            int new_fd = accept(server_fd, (sockaddr*)&cli_addr, &cli_size);
            if (new_fd == -1)
            {
                perror("accept failed: ");
            }
            msg_handle.handle_message_data(new_fd, clients, files_name);                          // waiting add thread
        }
        else
        {
            msg_handle.handle_message_data(fd, clients, files_name);                              // waiting add thread
        }
    }
}