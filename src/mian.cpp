#include "Manage_sock.h"

int main()
{
    Chat_ser server("0.0.0.0", 8888, 100);
    epoll_event events[100];

    while (true)
    {
        server.handle_accept(events, 100);
    }
}