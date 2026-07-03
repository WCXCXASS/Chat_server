#include "Manage_sock.h"

Chat_cli::Chat_cli() = default;
Chat_cli::~Chat_cli() = default;

void Chat_cli::handle_message_data(int cli_fd, Chat_ser* chat_ser, std::vector<std::string>& files_name)
{
    char tem[4096];
    int len = recv(cli_fd, tem, sizeof(tem), 0);
    if (len <= 0)
    {
        if (len == 0)
        {
            printf("%d finish\n", cli_fd);
            return;
        }
        perror("handle_message recv failed: ");
        return;
    }
    buffer.insert(buffer.end(), tem, tem + len);
    while (buffer.size() >= head_size)
    {
        Msg_type msg_type; // = //*((Msg_type*)buffer.data());
        uint32_t data_len; // = *((uint32_t*)(buffer.data() + sizeof(Msg_type)));
        memcpy(&msg_type, buffer.data(), sizeof(Msg_type));
        memcpy(&data_len, buffer.data() + sizeof(Msg_type), sizeof(uint32_t));
        data_len = ntohl(data_len);
        if (data_len + head_size > buffer.size()) break;
        std::vector<char> packet(buffer.begin(), buffer.begin() + (head_size + data_len));
        buffer.erase(buffer.begin(), buffer.begin() + (head_size + data_len));
        switch (msg_type)
        {
            case Msg_type::LOGIN_DATA:
            {
                chat_ser->handle_login(cli_fd, packet);
            }break;

            case Msg_type::REGISTER_DATA:
            {
                chat_ser->handle_register(cli_fd, packet);
            }break;

            case Msg_type::CHAT_DATA_B:
            {
                chat_ser->broadcast_message(cli_fd, packet);
            }break;

            case Msg_type::CHAT_DATA_P:
            {
                chat_ser->private_message(cli_fd, packet);
            }break;

            case Msg_type::FILE_NAME:
            {
                handle_file_name(files_name, packet);
            }break;

            case Msg_type::FILE_DATA:
            {
                handle_file_data(files_name, packet);
            }break;

            default:
            {
                perror("Msy_type invailed: ");
            }break;
        }
    }
}

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

void Chat_ser::handle_register(int cli_fd, std::vector<char> msg)
{
    
}

void Chat_ser::handle_login(int cli_fd, std::vector<char> msg)
{

}

void Chat_ser::send_all_message(int des_fd, Message_box send_msg)
{
    int send_all_len = send_msg.get_size();
    int send_len = 0;
    while (send_len < send_all_len)
    {
        int len = send(des_fd, send_msg.get_data() + send_len, send_all_len - send_len, 0);
        if (len <= 0)
        {
            if (len == 0) return;

            perror("send failed: ");
            break;
        }
        send_len += len;
    }
}

void Chat_ser::send_error_message(int des_fd, std::string msg)
{
    Message_box send_msg(Msg_type::ERROR_RESP);
    send_msg.set_data(msg.data(), msg.size());
    send_all_message(des_fd, send_msg);
}

void Chat_ser::private_message(int sou_fd, Message_box msg)     // Msg_type + data_len + des_name\0 + chat_data
{
    const char* msg_data = msg.get_data();
    std::string user_name(msg_data + head_size);

    auto it = clients.find(user_name);
    if (it == clients.end())
    {
        send_error_message(sou_fd, "not find the client");
        return;
    }
    int des_fd = it->second;

    send_all_message(des_fd, msg);
}

void Chat_ser::broadcast_message(int sou_fd, Message_box msg)
{
    for (auto& [cli_name, des_fd] : clients)
    {
        send_all_message(des_fd, msg);
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
            epoll_event cli_ev;
            sockaddr_in cli_addr;
            socklen_t cli_size = sizeof(cli_addr);
            int new_fd = accept(server_fd, (sockaddr*)&cli_addr, &cli_size);
            if (new_fd == -1)
            {
                perror("accept failed: ");
                continue;
            }

            cli_ev.events = EPOLLIN;
            cli_ev.data.fd = new_fd;
            res = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_fd, &cli_ev);
            if (res == -1)
            {
                perror("epoll_ctl failed: ");
            }

            chat_clients[new_fd] = std::make_unique<Chat_cli>();
             2. 将 new_fd 加入 epoll 后，等待事件触发
            // 3. 当 epoll 返回该 fd 可读时，在 else 分支中调用 handle_message_data
            //chat_clients[new_fd]->handle_message_data(new_fd, this, files_name);
            //msg_handle.handle_message_data(new_fd, clients, files_name);                          // waiting add thread
        }
        else
        {
            chat_clients[fd]->handle_message_data(fd, this, files_name);
            //msg_handle.handle_message_data(fd, clients, files_name);                              // waiting add thread
        }
    }
}

int Chat_ser::get_server_fd()
{
    return server_fd;
}