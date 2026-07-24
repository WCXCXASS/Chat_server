#include "Manage_sock.h"
#include "Logger.h"
#include <iostream>
#include <cstring>    // for strerror

Chat_cli::Chat_cli() = default;
Chat_cli::~Chat_cli()
{
    if (recv_file_buffer.ptr_w.is_open()) {
        recv_file_buffer.ptr_w.close();
    }
    if (send_file_buffer.ptr_r.is_open()) {
        send_file_buffer.ptr_r.close();
    }
}

void Chat_cli::handle_message_data(Chat_ser* chat_ser)
{
    char tem[4096];
    int len = recv(client_fd, tem, sizeof(tem), 0);
    if (len <= 0)
    {
        if (len == 0)
        {
            logging::info(std::to_string(client_fd) + " finish");
            client_fd = -1;
            return;
        }
        logging::error("handle_message recv failed: " + std::string(strerror(errno)));
        return;
    }
    buffer.insert(buffer.end(), tem, tem + len);
    while (buffer.size() >= head_size)
    {
        Msg_type msg_type;
        uint32_t data_len;
        memcpy(&msg_type, buffer.data(), sizeof(Msg_type));
        memcpy(&data_len, buffer.data() + sizeof(Msg_type), sizeof(uint32_t));
        data_len = ntohl(data_len);

        if (buffer.size() < data_len + head_size) break;

        std::vector<char> packet(buffer.begin(), buffer.begin() + (head_size + data_len));
        buffer.erase(buffer.begin(), buffer.begin() + (head_size + data_len));
        switch (msg_type)
        {
            case Msg_type::LOGIN_DATA:
            {
                chat_ser->handle_login(client_fd, packet);
            }break;

            case Msg_type::REGISTER_DATA:
            {
                chat_ser->handle_register(client_fd, packet);
            }break;

            case Msg_type::CHAT_DATA_B:
            {
                chat_ser->broadcast_message(client_fd, packet);
            }break;

            case Msg_type::CHAT_DATA_P:
            {
                chat_ser->private_message(client_fd, packet);
            }break;

            case Msg_type::FILE_NAME:
            {
                chat_ser->open_client_file(client_fd, packet);
            }break;

            case Msg_type::FILE_DATA:
            {
                chat_ser->handle_client_file(client_fd, packet);
            }break;

            case Msg_type::FILE_END_MSG:
            {
                chat_ser->client_file_close(client_fd);
            }break;

            case Msg_type::FILE_LIST_REQ:
            {
                send_file_list();
            }break;

            case Msg_type::FILE_REQUST:
            {
                open_send_file(packet);
                chat_ser->enable_send(client_fd);
            }break;

            default:
            {
                logging::error("Msy_type invailed: " + std::string(strerror(errno))); // perror
            }break;
        }
    }
}

void Chat_cli::send_file_list()
{
    Message_box files_list_msg(Msg_type::FILE_LIST_RESP);
    uint32_t data_len;

    for (auto& file_name : files_list)
    {
        data_len = static_cast<uint32_t>(file_name.size()) + 1;
        files_list_msg.set_data(file_name.data(), data_len);

        Chat_ser::send_all_message(client_fd, files_list_msg);

        files_list_msg.clear();
    }

    Message_box list_end(Msg_type::FILE_LIST_END);
    list_end.set_data(nullptr, 0);
    Chat_ser::send_all_message(client_fd, list_end);
}

void Chat_cli::open_send_file(std::vector<char> msg)
{
    logging::info("open_send_file"); // std::cout

    std::string file_name = msg.data() + head_size;
    std::string file_path(file_name);

    send_file_buffer.ptr_r.open(file_path, std::ios::binary);
    if (!send_file_buffer.ptr_r.is_open())
    {
        logging::error("ifstream open file failed: " + std::string(strerror(errno))); // perror
        Chat_ser::send_error_message(client_fd, "not find file");
        return;
    }
    file_open = true;

    logging::info("open_send_file finish"); // std::cout
}

void Chat_cli::send_file_data()
{
    if (!file_open) return;

    logging::info("send_file_data"); // std::cout

    std::ifstream& file = send_file_buffer.ptr_r;
    const size_t CHUNK_SIZE = 4096;
    std::vector<char> buffer(CHUNK_SIZE);
    if (file.read(buffer.data(), CHUNK_SIZE) || file.gcount() > 0)
    {
        logging::info("send data: " + std::to_string(file.gcount()) + " bit"); // std::cout

        Message_box msg(Msg_type::FILE_DATA);
        msg.set_data(buffer.data(), file.gcount());
        Chat_ser::send_all_message(client_fd, msg);
        buffer.clear();
    }
    else
    {
        logging::info("send file end"); // std::cout

        Message_box msg_end(Msg_type::FILE_END_MSG);
        msg_end.set_data(nullptr, 0);
        Chat_ser::send_all_message(client_fd, msg_end);
        file.close();
        file_open = false;
    }
}

void Chat_cli::handle_file_name(std::vector<char> msg)
{
    std::string des_name = (msg.data() + head_size);
    std::string file_name(msg.data() + head_size + des_name.size() + 1);
    recv_file_buffer.name = file_name;

    std::string file_path(file_name);
    recv_file_buffer.ptr_w.open(file_name, std::ios::binary);

    if (!recv_file_buffer.ptr_w.is_open())
    {
        logging::error("sfstream open file failed: " + std::string(strerror(errno))); // perror
        return;
    }
}

void Chat_cli::handle_file_data(std::vector<char> msg)
{
    if (!recv_file_buffer.ptr_w.is_open()) return;

    uint32_t data_len;
    memcpy(&data_len, msg.data() + sizeof(uint8_t), sizeof(uint32_t));
    data_len = ntohl(data_len);
    
    recv_file_buffer.ptr_w.write(msg.data() + head_size, data_len);
}

void Chat_cli::handle_file_close()
{
    recv_file_buffer.ptr_w.close();
    files_list.push_back(recv_file_buffer.name);
}

bool Chat_cli::is_file_open()
{
    return file_open;
}

Chat_ser::Chat_ser(const char *ip, uint32_t port, int maxnums)
{
    int res;
    sockaddr_in ser_addr;
    ser_addr.sin_family = AF_INET;
    ser_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &ser_addr.sin_addr);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1)
    {
        logging::error("socket failed: " + std::string(strerror(errno))); // perror
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    res = bind(server_fd, (sockaddr*)&ser_addr, sizeof(ser_addr));
    if (res == -1)
    {
        logging::error("bind failed: " + std::string(strerror(errno))); // perror
    }

    res = listen(server_fd, maxnums);
    if (res == -1)
    {
        logging::error("listen failed: " + std::string(strerror(errno))); // perror
    }

    epoll_fd = epoll_create(1);
    if (epoll_fd == -1)
    {
        logging::error("epoll_create failed: " + std::string(strerror(errno))); // perror
    }

    epoll_event server_ev;
    server_ev.events = EPOLLIN;
    server_ev.data.fd = server_fd;
    res = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &server_ev);
    if (res == -1)
    {
        logging::error("epoll_ctl failed: " + std::string(strerror(errno))); // perror
    }
}

Chat_ser::~Chat_ser() = default;

void Chat_ser::open_client_file(int sou_fd, std::vector<char> msg)
{
    std::string des_name(msg.data() + head_size);
    int des_fd = -1;
    
    for (auto& [cli_fd, cli] : clients)
    {
        if (cli->get_name() == des_name)
        {
            des_fd = cli->get_fd();
        }
    }

    if (des_fd == -1)
    {
        send_error_message(sou_fd, "not find user");
        return;
    }
    set_cli_cli(sou_fd, des_fd);
    clients[des_cli[sou_fd]]->handle_file_name(msg);
}

void Chat_ser::handle_client_file(int sou_fd, std::vector<char> msg)
{
    clients[des_cli[sou_fd]]->handle_file_data(msg);
}

void Chat_ser::enable_send(int sou_fd)
{
    epoll_event cli_ev;
    cli_ev.data.fd = sou_fd;
    cli_ev.events = EPOLLIN | EPOLLOUT;
    int res = epoll_ctl(epoll_fd, EPOLL_CTL_MOD, sou_fd, &cli_ev);
    if (res == -1)
    {
        logging::error("epoll_ctl mod to EPOLLOUT failed: " + std::string(strerror(errno))); // perror
    }
}

void Chat_ser::client_file_close(int sou_fd)
{
    clients[des_cli[sou_fd]]->handle_file_close();
    des_cli.erase(sou_fd);
}

void Chat_ser::set_cli_cli(int sou_fd, int des_fd)
{
    des_cli[sou_fd] = des_fd;
}

void Chat_ser::remove_client(int fd)
{
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
    
    for (auto it = des_cli.begin(); it != des_cli.end();)
    {
        auto& [sou_fd, des_fd] = *it;
        if (sou_fd == fd || des_fd == fd)
        {   
            it = des_cli.erase(it);
        }
        else
        {
            it++;
        }
    }

    close(fd);
    clients.erase(fd);
}

void Chat_ser::handle_register(int cli_fd, std::vector<char> msg)   // Msg_type + data_len + username/0 + password/0
{
    logging::info("client registering fd = " + std::to_string(cli_fd)); // std::cout
    std::string username(msg.data() + head_size);
    std::string password(msg.data() + head_size + username.size() + 1);
    
    if (!db.register_user(username, password))
    {
        send_error_message(cli_fd, "register faied: name repeat, etc");
        return;
    }

    clients[cli_fd]->set_fd(cli_fd);
    clients[cli_fd]->set_name(username);
    send_error_message(cli_fd, "register success");
    logging::info("client register fd = " + std::to_string(cli_fd) + " username :" + username);
}

void Chat_ser::handle_login(int cli_fd, std::vector<char> msg)     // Msg_type + data_len + username/0
{
    std::string username(msg.data() + head_size);
    std::string password(msg.data() + head_size + username.size() + 1);

    logging::info("client logining" + username);

    if (!db.login_user(username, password))
    {
        send_error_message(cli_fd, "login failed");
        return;
    }

    clients[cli_fd]->set_fd(cli_fd);
    clients[cli_fd]->set_name(username);
    logging::info("client logined, name: " + username);
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

            logging::error("send failed: " + std::string(strerror(errno))); // perror
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

void Chat_ser::private_message(int sou_fd, Message_box msg)
{
    const char* msg_data = msg.get_data();
    std::string user_name(msg_data + head_size);
    int des_fd = -1;

    for (auto& [fd, cli] : clients)
    {
        if (cli->get_name() == user_name)
        {
            des_fd = fd;
            break;
        }
    }
    if (des_fd == -1)
    {
        send_error_message(sou_fd, "not find user");
        return;
    }

    send_all_message(des_fd, msg);
}

void Chat_ser::broadcast_message(int sou_fd, Message_box msg)
{
    for (auto& [des_fd, ptr] : clients)
    {
        send_all_message(des_fd, msg);
    }
}

void Chat_ser::handle_accept(epoll_event *events, int maxevents)
{
    int res;
    
    logging::info("epoll_wait"); // printf
    int events_n = epoll_wait(epoll_fd, events, maxevents, -1);
    if (events_n == -1)
    {
        logging::error("epoll_wait: " + std::string(strerror(errno))); // perror
    }

    for (int i = 0; i < events_n; i++)
    {
        epoll_event cli_ev;
        int fd = events[i].data.fd;
        if (fd == server_fd)
        {
            sockaddr_in cli_addr;
            socklen_t cli_size = sizeof(cli_addr);
            int new_fd = accept(server_fd, (sockaddr*)&cli_addr, &cli_size);
            if (new_fd == -1)
            {
                logging::error("accept failed: " + std::string(strerror(errno))); // perror
                continue;
            }

            logging::info("client " + std::to_string(new_fd)); // printf

            cli_ev.events = EPOLLIN;
            cli_ev.data.fd = new_fd;
            res = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_fd, &cli_ev);
            if (res == -1)
            {
                logging::error("epoll_ctl failed: " + std::string(strerror(errno))); // perror
            }

            clients[new_fd] = std::make_unique<Chat_cli>();
            clients[new_fd]->set_fd(new_fd);
        }
        else
        {
            if (events[i].events & EPOLLIN)
            {
                clients[fd]->handle_message_data(this);
            }
            
            if (events[i].events & EPOLLOUT)
            {
                clients[fd]->send_file_data();

                if (!clients[fd]->is_file_open())
                {
                    cli_ev.events = EPOLLIN;
                    cli_ev.data.fd = fd;
                    epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &cli_ev);
                }
            }

            if (clients.find(fd) != clients.end() && clients[fd]->get_fd() == -1)
            {
                remove_client(fd);
            }
        }
    }
}

int Chat_ser::get_server_fd()
{
    return server_fd;
}

int Chat_cli::get_fd()
{
    return client_fd;
}

void Chat_cli::set_fd(int fd)
{
    client_fd = fd;
}

const std::string& Chat_cli::get_name()
{
    return name;
}

void Chat_cli::set_name(std::string str)
{
    name = str;
}